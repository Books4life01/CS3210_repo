#include "asm/x86.h"
#include "param.h"
#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"
#include "lab2_ag.h"
#include "spinlock.h"
#include <stddef.h>

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()

//using kmem lock for ref counts
struct spinlock ref_lock;
int ref_counts[PHYSTOP/PGSIZE];

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    // NOTE(lab2): We dont call lab2_pgzero() here, as these are page table
    //   pages, not data pages.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  // NOTE(lab2): We don't call lab2_pgzero here, as these are kernel page-table
  // (e.g. the page table for the kernel address space) entries, and not
  // lazily initialized
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void*)DEVSPACE)
    panic("PHYSTOP too high");
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
  lcr3(V2P(kpgdir));   // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
  if(p == 0)
    panic("switchuvm: no process");
  if(p->kstack == 0)
    panic("switchuvm: no kstack");
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts)-1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iomb = (ushort) 0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir));  // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  // NOTE(lab2): This is only used to set up the initial user virtual address
  // space.  We don't require you zero-initialize or copy-on-write this page.
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
  // NOTE(lab2): This is only used to set up the initial user virtual address
  // space.  We don't require you zero-initialize or copy-on-write this page.
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i,  n;
  pte_t *pte;
  char* mem;

  if((uint) addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for(i = 0; i < sz; i += PGSIZE){

    //readi is executed in run by the system and wont trigger a page fault messing up lazy allocation
    //thus we need to create pages for the program segment

    

    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
      panic("loaduvm: address should exist");

    if((mem = kalloc())==0)
      return -1;
    lab2_pgzero(mem, (uint)addr+i);
    // pa = PTE_ADDR(*pte);
    *pte = PTE_ADDR(V2P(mem))|PTE_P|PTE_U|PTE_W;
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, mem, offset+i, n) != n)
      return -1;
  }
  return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  uint a;
 // char* mem;

  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){


    // mem = kalloc();
    // if(mem == 0){
    //   cprintf("allocuvm out of memory\n");
    //   deallocuvm(pgdir, newsz, oldsz);
    //   return 0;
    // }
    // lab2_pgzero(mem, a);
    // if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
    //   cprintf("allocuvm out of memory (2)\n");
    //   deallocuvm(pgdir, newsz, oldsz);
    //   kfree(mem);
    //   return 0;
    // }

    //instead of allocating a page, we change the pte_t to point to the zero page and set LA bit to 1
    if (mappages(pgdir, (char*)a, PGSIZE, (uint)phys_zero_page, PTE_LA|PTE_U)<0){
      deallocuvm(pgdir, newsz, oldsz);

      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a  < oldsz; a += PGSIZE){
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if((*pte & PTE_P) != 0){
      pa = PTE_ADDR(*pte);
      if(pa == 0)
        panic("kfree");
      char *v = P2V(pa);
      kfree(v);
      *pte = 0;
    }
  }
  return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
  uint i;

  if(pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for(i = 0; i < NPDENTRIES; i++){
    if(pgdir[i] & PTE_P){
      char * v = P2V(PTE_ADDR(pgdir[i]));
      kfree(v);
    }
  }
  kfree((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if(pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
   
    //COLE's CODE

    //If  the page has W=1, we set COW=1; otherwise we jsut leave as be
    //when a trap is handling a write fault, if COW is set it will set W=1 after copying, otherwise it wont because that page is read only
    //regardless we can just copy instead of allocating a new page

    //additionaly if the page has LA=1, then we dont have to set COW, because its pointing to the shared zero page and has its one handler
    if(!(*pte&PTE_LA)&& (*pte&PTE_W||*pte&PTE_COW)){
      //set copy on write flags for both pte's
      flags |= PTE_COW;
      *pte |= PTE_COW;
      //disable write for both pte's
      flags &= ~PTE_W;
      *pte &= ~PTE_W;
      invlpg((char*)i);
    }

     //make the new page table point to the old pages
    if(mappages(d, (void*)i, PGSIZE, pa, flags) < 0) {
      goto bad;
    }
    acquire(&ref_lock);  
    //update ref counts
    ref_counts[pa>>12]+=1;
    release(&ref_lock); 
       
    //END COLE CODE

  }
  return d;

bad:
  freevm(d);
  return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char*
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if((*pte & PTE_P) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  return (char*)P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char*)p;
  while(len > 0){
    va0 = (uint)PGROUNDDOWN(va);
    normalize_page(pgdir, va0);

    pa0 = uva2ka(pgdir, (char*)va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if(n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}


//take a page that is either lazily allocated or copy on write and allocate memory for it so it can be used normally
void normalize_page(pde_t *pgdir, uint va){
    
    //Procure the page table entry
    pte_t* pte = walkpgdir(pgdir, (char*)va, 0);

    char* mem;

      //if the page has a copy on write flag
      if(*pte&PTE_COW){
        acquire(&ref_lock);
        
        //if its not a shared reference page, then we can just set its write bit back
        if(ref_counts[(PTE_ADDR(*pte)>>12)]<=1){      
          release(&ref_lock);  
          *pte |=PTE_W;
          //if no additional refs clear COWs
          *pte &=~PTE_COW;
          invlpg((char*)va);
          return;        
        }
        release(&ref_lock);
        //otherwise we need to copy the page

        //kalloc a new page
        if ((mem = kalloc())==0){
          return;
        }
        //copy the new mage into mem
        lab2_pgcopy(mem, (char*)P2V(PTE_ADDR(*pte)), va);

        //save the old physical adress so we can change its ref count
        uint pa = PTE_ADDR(*pte);


        acquire(&ref_lock);

        //change PPN of the pte
        *pte = PTE_ADDR(V2P(mem)) | PTE_FLAGS(*pte);


        //decrease ref count for the old_page
        ref_counts[(PTE_ADDR(pa)>>12)]-=1;
        ref_counts[(PTE_ADDR(V2P(mem))>>12)]=1;//set new page ref count to 1
        *pte |=PTE_W;
        *pte &=~PTE_COW;
        release(&ref_lock);

        
        invlpg((char*)va);
        return;
      }


      //if the page has a lazy allocation flag
      if(*pte&PTE_LA)
      {
        char* mem = kalloc();
        if(mem==NULL){
          return ;
        }
        lab2_pgzero(mem, va);
        //replace the zero page PPn with an actually allocated and zerod page, remove the LA flag
        *pte = PTE_ADDR(V2P(mem)) | PTE_FLAGS(*pte)|PTE_W;
        *pte &=~PTE_LA;
        invlpg((char*)va);
        return;


      }
    


}

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

