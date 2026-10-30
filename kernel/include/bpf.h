#ifndef _BPF_H
#define _BPF_H

#include "types.h"
#include "spinlock.h"

#define NUMREGS         8
#define MEMSIZE         512
#define NUMINSNS        128
#define MAPSIZE         16

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
    struct {
        uint32_t index;
        uint8_t value;
    } map;
};

struct bpf_vm {
    int loaded;
    uint8_t ip;
    uint8_t reg[NUMREGS];
    uint8_t mem[MEMSIZE];
    uint8_t map[MAPSIZE]; 
    struct bpf_insn insns[NUMINSNS];
};

extern struct bpf_vm vm;
extern struct spinlock vm_lock;

void bpf_init(void);
int bpf_load(const union bpf_attr *attr);
int bpf_run(char *pkt, uint pktsize);

#endif // _BPF_H
