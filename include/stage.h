// stage.h
// ----------------------------------------------------------
// 스테이지 데이터를 외부 파일(예: 텍스트 파일)에서 읽어와
// Stage 구조체에 채워 넣는 기능을 제공하는 헤더.
// - 스테이지 파일 포맷:
//   * ID, 이름, 맵, 시작/골 위치, 장애물 정보 등을 특정 형식으로 저장해놓고,
//   * load_stage 함수가 그걸 파싱해서 Stage에 채워 넣는 식.

#ifndef STAGE_H
#define STAGE_H

#include "../include/game.h"   // Stage 구조체 정의 사용

// 특정 스테이지 ID에 해당하는 스테이지를 로드하는 함수.
// - 인자 stage: 로드 결과를 저장할 Stage 구조체 포인터.
// - 인자 stage_id: 몇 번째 스테이지를 가져올지 지정 (1, 2, 3 ...).
// - 반환값 예시:
//   * 0: 성공
//   * 음수: 파일 없음, 파싱 실패 등 오류
// - 메인 게임 로직에서는 보통:
//   for (int id = 1; id <= total_stages; id++) {
//       load_stage(&stage, id);
//       ... 플레이 ...
//   }
//   이런 식으로 사용.
int load_stage(Stage *stage, int stage_id);
int get_stage_count(void);

#endif // STAGE_H
