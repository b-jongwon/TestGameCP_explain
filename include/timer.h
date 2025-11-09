#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

double get_elapsed_time(struct timeval start, struct timeval end);

#endif
