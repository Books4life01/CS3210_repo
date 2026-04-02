#ifndef INCLUDE_ETH_h
#define INCLUDE_ETH_h

#include "types.h"

struct __attribute__((packed)) ethframe {
	uint8_t dest_mac[6];
	uint8_t src_mac[6];
	uint16_t length;
	char payload[256];
};

int eth_send(char *, uint);
int eth_receive(char *, uint);

#endif