#include "timer.h"   // struct timeval, 함수 선언 포함


// ----------------------------------------------------------
// get_elapsed_time()
// ----------------------------------------------------------
// 두 시각(start, end) 사이의 경과 시간을 초 단위(double)로 계산.
// - struct timeval:
//     tv_sec  : 초 단위
//     tv_usec : 마이크로초(1/1,000,000초) 단위
// - 계산식:
//     (end.tv_sec - start.tv_sec)               → 정수 초 차이
//   + (end.tv_usec - start.tv_usec) / 1,000,000 → 소수점 이하 초
double get_elapsed_time(struct timeval start, struct timeval end) {
    return (end.tv_sec  - start.tv_sec) +
           (end.tv_usec - start.tv_usec) / 1000000.0;
}
