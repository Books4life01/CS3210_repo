struct stat;
struct rtcdate;

// bpf
#define LD_OPCODE       0x0
#define LDI_OPCODE      0x1
#define JE_OPCODE       0x2
#define JGT_OPCODE      0x3
#define JGE_OPCODE      0x4
#define RET_PASS_OPCODE 0x5
#define RET_DROP_OPCODE 0x6
#define ADD_OPCODE      0x7
#define SUB_OPCODE      0x8
#define CALL_OPCODE     0x9

#define BPF_PROG_LOAD        0
#define BPF_MAP_LOOKUP_ELEM  1
#define BPF_MAP_UPDATE_ELEM  2

#define BPF_FUNC_map_lookup_elem 1
#define BPF_FUNC_map_update_elem 2

union bpf_attr {
    struct {
        struct bpf_insn *insns;
        uint32_t insn_cnt;
    } prog;
    struct {
        uint32_t index;
        uint8_t value;
    } map;
};

struct bpf_insn {
    uint8_t opcode;

    uint16_t src_addr;

    int16_t imm;
    int8_t offset;

    uint8_t reg0;
    uint8_t reg1;
    uint8_t dst_reg;
};

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int bpf(int cmd, union bpf_attr *attr);
int eth_send(char *, uint);
int eth_receive(char *, uint);
int lspci(void);
int whatsmymac(unsigned char *);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);