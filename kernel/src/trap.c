#include "asm/x86.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "traps.h"
#include "spinlock.h"
#include "lab2_ag.h"
#include <stddef.h>


// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  //START COLE CODE
  case T_PGFLT:
  //report page fault
    lab2_report_pagefault(tf);

    //faulting page is put in cr2
    uint va = PGROUNDDOWN(rcr2());
    
    pde_t* pgdir = myproc()->pgdir;


    pde_t pde = pgdir[PDX(va)];
    
  //if the page isnt present, this is a true page fault and on xv6 we arent dealing with switching so we kill the process
    if(!(pde&PTE_P)){
      myproc()->killed=1;
      exit();
    }
    //Manually find the pte pointer
    pte_t* pte = (pte_t*)P2V(PTE_ADDR(pde))+PTX(va);
    // cprintf("PTE Entry PTE: %x, LA: %d COW: %d\n", *pte, *pte&PTE_LA,*pte&PTE_COW);

    char* mem;

    //If the fault came from a write issue, its either a COW or LA issue

    if(tf->err & 2){

      //if its a copy on write issue
      if(*pte&PTE_COW){
        acquire(&ref_lock);
        
        //if its not a shared reference page, then we can just set its write bit back
        if(ref_counts[(PTE_ADDR(*pte)>>12)]<=1){      
          release(&ref_lock);  
          *pte |=PTE_W;
          //if no additional refs clear COWs
          *pte &=~PTE_COW;
          invlpg((char*)va);
          break;

        }
        release(&ref_lock);
        //otherwise we need to copy the page

        //kalloc a new page
        if ((mem = kalloc())==0){
          //out of memeory kill the process; xv6 does not handle swapping about memory from disk
          myproc()->killed=1;
          exit();
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
        break;
      }


      //if the page fault is because of a lazy allocation
      if(*pte&PTE_LA)
      {
        char* mem = kalloc();
        if(mem==NULL){
          //out of memeory kill the process; xv6 does not handle swapping about memory from disk
          myproc()->killed=1;
          exit();
        }

        lab2_pgzero(mem, va);
        //replace the zero page PPn with an actually allocated and zerod page, remove the LA flag
        *pte = PTE_ADDR(V2P(mem)) | PTE_FLAGS(*pte)|PTE_W;
        *pte &=~PTE_LA;
        invlpg((char*)va);

        break;


      }
    }
    
    myproc()->killed=1;
    exit();
    //END COLE CODE
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  // NOTE(lab2): Disabled preemptive yield for testing.
  /*
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();
  */

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
