#ifndef _BPF_H
#define _BPF_H

#include "types.h"
#include "spinlock.h"

#define NUMREGS         8
#define MEMSIZE         512
#define NUMINSNS        128

#define LD_OPCODE       0x0
#define LDI_OPCODE      0x1
#define JE_OPCODE       0x2
#define JGT_OPCODE      0x3
#define JGE_OPCODE      0x4
#define RET_PASS_OPCODE 0x5
#define RET_DROP_OPCODE 0x6
#define ADD_OPCODE      0x7
#define SUB_OPCODE      0x8

#define BPF_PROG_LOAD   0

struct bpf_insn {
    uint8_t opcode;

    uint16_t src_addr;

    int16_t imm;
    int8_t offset;

    uint8_t reg0;
    uint8_t reg1;
    uint8_t dst_reg;
};

union bpf_attr {
    struct {
        struct bpf_insn *insns;
        uint32_t insn_cnt;
    } prog;
};

struct bpf_vm {
    int loaded;
    uint8_t ip;
    uint8_t reg[NUMREGS];
    uint8_t mem[MEMSIZE];
    struct bpf_insn insns[NUMINSNS];
};

extern struct bpf_vm vm;
extern struct spinlock vm_lock;

void bpf_init(void);
int bpf_load(const union bpf_attr *attr);
int bpf_run(char *pkt, uint pktsize);

#endif // _BPF_H