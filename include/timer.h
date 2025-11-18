// timer.h
// -----------------------------------------------------------
// 시간 측정을 위한 간단한 유틸리티 함수 제공.
// - get_elapsed_time: start, end 시각 사이의 경과 시간을 초 단위 double로 계산.
// - struct timeval: gettimeofday 등을 통해 얻는 초/마이크로초 단위 시간 구조체.

#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>   // struct timeval, gettimeofday 사용

// 두 시각(start, end) 사이의 경과 시간을 초 단위(double)로 반환하는 함수.
// - 인자 start: 측정 시작 시각.
// - 인자 end: 측정 종료 시각.
// - 구현 예시:
//   double get_elapsed_time(struct timeval start, struct timeval end) {
//       double diff_sec  = (double)(end.tv_sec  - start.tv_sec);
//       double diff_usec = (double)(end.tv_usec - start.tv_usec) / 1e6;
//       return diff_sec + diff_usec;
//   }
// - 게임에서는:
//   * 스테이지 시작할 때 gettimeofday(&start),
//   * 클리어 시점에 gettimeofday(&end),
//   * 그 차이를 이 함수로 계산해서 클리어 시간으로 사용.
//   * 그 값을 fileio 모듈에 넘겨 기록 갱신에 활용 가능.
double get_elapsed_time(struct timeval start, struct timeval end);

#endif // TIMER_H
