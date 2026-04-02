#ifndef _E1000_H
#define _E1000_H

void e1000_init(void);
int e1000_transmit(char *, uint);
int e1000_receive(char *, uint);

#endif // _E1000_H