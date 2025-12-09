// render.h
// -----------------------------------------------------------
// 현재 스테이지와 플레이어 상태를 터미널 화면에 그려주는 렌더링 함수 제공.
// - 맵, 플레이어 위치, 장애물, 타이머, 스테이지 번호 등을 한 번에 출력.
// - 게임의 "화면 출력"을 한 곳에 모아둔 모듈.

#ifndef RENDER_H
#define RENDER_H

#include "../include/game.h"   // Stage, Player 구조체 정의 사용

// SDL 기반 렌더러 초기화/해제 함수.
// - init_renderer: SDL, 텍스처 로드, 윈도우 생성
// - shutdown_renderer: 사용 중인 리소스 정리
int init_renderer(void);
void shutdown_renderer(void);

// 전체 게임 화면을 그려주는 함수.
// - 인자 stage: 현재 스테이지 상태 (맵, 장애물 위치 등)
// - 인자 player: 현재 플레이어 상태 (위치, alive 여부)
// - 인자 elapsed_time: 현재까지 경과 시간 (초 단위)
// - 인자 current_stage: 현재 스테이지 번호(예: 1, 2, 3 ...)
// - 인자 total_stages: 전체 스테이지 수 (예: 5 스테이지 중 몇 번째인지 표시).

void render(const Stage *stage, const Player *player, double elapsed_time, int current_stage, int total_stages);

// 비플레이 상태 화면 렌더러.
// - 시작 화면: 메뉴 선택 상태를 받아 오른쪽 패널에 하이라이트를 표시.
// - 기록 화면: 최고 기록을 전달받아 텍스트 UI 렌더링.
// - 게임오버 화면: 실패 상태에서 사용자 입력 대기 중 표시.
void render_title_screen(int selected_index);
void render_records_screen(double best_time);
void render_game_over_screen(void);

#endif // RENDER_H
