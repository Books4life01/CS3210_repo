#include "lab4_net_ag.h"

#include "defs.h"

void lab4_net_report_send(char *data, uint size) {
    cprintf("================\n");
    cprintf("Sending bytes:\n");
    for (int i = 0; i < size; i++) {
        cprintf("0x%x\n", (uchar)data[i]);
    }
    cprintf("================\n");
}

void lab4_net_report_recv(char *data, uint size) {
    cprintf("================\n");
    cprintf("Received bytes:\n");
    for (int i = 0; i < size; i++) {
        cprintf("0x%x\n", (uchar)data[i]);
    }
    cprintf("================\n");
}