// render.h
// -----------------------------------------------------------
// 현재 스테이지와 플레이어 상태를 터미널 화면에 그려주는 렌더링 함수 제공.
// - 맵, 플레이어 위치, 장애물, 타이머, 스테이지 번호 등을 한 번에 출력.
// - 게임의 "화면 출력"을 한 곳에 모아둔 모듈.

#ifndef RENDER_H
#define RENDER_H

#include "game.h"   // Stage, Player 구조체 정의 사용

// 전체 게임 화면을 그려주는 함수.
// - 인자 stage: 현재 스테이지 상태 (맵, 장애물 위치 등)
// - 인자 player: 현재 플레이어 상태 (위치, alive 여부)
// - 인자 elapsed_time: 현재까지 경과 시간 (초 단위)
// - 인자 current_stage: 현재 스테이지 번호(예: 1, 2, 3 ...)
// - 인자 total_stages: 전체 스테이지 수 (예: 5 스테이지 중 몇 번째인지 표시).
// - 내부에서 할 일 예시:
//   1) 화면 clear (ncurses라면 clear(), 일반 터미널이면 system("clear") 등).
//   2) stage->map을 한 줄씩 출력.
//   3) 그 위에 player와 장애물을 좌표에 맞게 덮어 그리기.
//   4) 아래쪽 혹은 상단에 "시간: xx.xx초, Stage x / y" 같은 정보 표시.
// - 이 함수 하나로 "화면 업데이트"를 담당하게 하여, 메인 루프에서 단순히 render(...)만 반복 호출하면 됨.
void render(const Stage *stage, const Player *player, double elapsed_time, int current_stage, int total_stages);

#endif // RENDER_H
