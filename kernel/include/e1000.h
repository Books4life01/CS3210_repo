#ifndef _E1000_H
#define _E1000_H

void e1000init(void);
void e1000intr(void);
int e1000tx(char *, uint);
int e1000rx(char *, uint);

#endif // _E1000_H