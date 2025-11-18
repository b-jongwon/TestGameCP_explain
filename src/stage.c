#include <stdio.h>    // fopen, fgets, perror, FILE
#include <string.h>   // memset, strlen, snprintf

#include "stage.h"    // Stage, MAX_X, MAX_Y, MAX_OBSTACLES, Obstacle


// ----------------------------------------------------------
// load_stage()
// ----------------------------------------------------------
// 주어진 stage_id에 해당하는 스테이지 파일을 읽어
// Stage 구조체에 정보를 채워 넣는 함수.
// - 파일 포맷 예시 (assets/stage1.map):
//     #######...
//     # S   X...
//     #   G   ...
//   여기서
//     'S' = 시작 위치
//     'G' = 골 위치
//     'X' = 장애물 초기 위치
//     '@' = 벽
//     ' ' = 빈 공간
// - stage->map에는 실제 플레이용 맵이 저장되고,
//   장애물/시작/골 정보는 따로 Stage 필드에 기록됨.
int load_stage(Stage *stage, int stage_id) {

    // Stage 구조체 전체를 0으로 초기화
    memset(stage, 0, sizeof(Stage));

    // ID 설정
    stage->id = stage_id;

    // 스테이지 이름 설정
    if (stage_id == 1) {
        snprintf(stage->name, sizeof(stage->name), "Training Hall");
    } else if (stage_id == 2) {
        snprintf(stage->name, sizeof(stage->name), "Guarded Corridor");
    } else {
        snprintf(stage->name, sizeof(stage->name), "Final Escape");
    }

    // 읽어올 맵 파일 이름 구성: "assets/stage%d.map"
    char filename[64];
    snprintf(filename, sizeof(filename), "assets/stage%d.map", stage_id);

    // 맵 파일을 텍스트 읽기 모드로 연다
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        // 파일 열기 실패 시 에러 메시지 출력 후 -1 반환
        perror("fopen");
        return -1;
    }

    char line[256];  // 한 줄씩 읽어올 버퍼
    int y = 0;       // 현재 맵의 y 인덱스(행 번호)

    // 파일에서 한 줄씩 읽어와서 맵 데이터로 변환
    while (y < MAX_Y && fgets(line, sizeof(line), fp)) {

        int len = (int)strlen(line);

        // 줄 끝의 개행 문자 제거('\n' 또는 '\r')
        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        // x 좌표 0 ~ MAX_X-1까지 채우기
        for (int x = 0; x < MAX_X; x++) {

            // 현재 줄 길이보다 크면 빈칸으로 취급
            char c = (x < len) ? line[x] : ' ';

            if (c == 'S') {
                // 시작 위치 기록
                stage->start_x = x;
                stage->start_y = y;

                // 맵에는 실제로는 빈 공간으로 저장 (플레이어는 따로 표시)
                stage->map[y][x] = ' ';
            } 
            else if (c == 'G') {
                // 골 위치 기록
                stage->goal_x = x;
                stage->goal_y = y;

                // 맵에는 'G'로 남겨두어 렌더링 시 골 위치 표시
                stage->map[y][x] = 'G';
            } 
            else if (c == 'X') {
                // 장애물 초기 위치
                if (stage->num_obstacles < MAX_OBSTACLES) {

                    // 새로운 장애물 하나를 추가하고 포인터를 얻음
                    Obstacle *o = &stage->obstacles[stage->num_obstacles++];

                    o->x = x;
                    o->y = y;

                    // 초기 이동 방향은 오른쪽/아래 방향(+1)으로 시작
                    o->dir = 1;

                    // type은 스테이지/좌표의 조합으로 수평/수직을 결정
                    // (간단한 의사 랜덤 느낌)
                    o->type = (stage_id + x + y) % 2;
                }

                // 맵에는 'X' 대신 빈 공간으로 저장
                // 장애물은 렌더링 단계에서 따로 'X'로 그릴 것.
                stage->map[y][x] = ' ';
            } 
            else {
                // 그 밖의 문자('@', ' ', '#', 등)은 그대로 맵에 저장
                stage->map[y][x] = c;
            }
        }

        // 문자열 끝에 널 문자
        stage->map[y][MAX_X] = '\0';

        // 다음 줄(y + 1)로 이동
        y++;
    }

    // 파일에 줄이 부족하면 나머지 줄은 빈 공간으로 채움
    for (; y < MAX_Y; y++) {
        for (int x = 0; x < MAX_X; x++) {
            stage->map[y][x] = ' ';
        }
        stage->map[y][MAX_X] = '\0';
    }

    // 파일 닫기
    fclose(fp);

    // 정상 종료
    return 0;
}
