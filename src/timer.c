#include "timer.h"

double get_elapsed_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) +
           (end.tv_usec - start.tv_usec) / 1000000.0;
}
