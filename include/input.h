#ifndef INPUT_H
#define INPUT_H

void init_input(void);
void restore_input(void);
int poll_input(void); // returns -1 if no key, otherwise char

#endif
