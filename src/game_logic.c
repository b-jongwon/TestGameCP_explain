#include "game.h"   
// game.h에는 Stage, Player 구조체 정의가 들어있다.
// 이 파일은 Stage와 Player의 좌표를 비교하는 기능만 필요하므로 
// game.h만 include하면 충분하다.
// (불필요한 의존성 최소화: 좋은 설계)


/* --------------------------------------------------------------
 * is_goal_reached()
 * --------------------------------------------------------------
 * 플레이어가 현재 스테이지의 골(goal) 위치에 도달했는지를 검사하는 함수.
 *
 * 역할:
 *   - 플레이어 이동 이후, 메인 게임 루프에서 "클리어 조건"을 확인할 때 사용된다.
 *   - goal_x, goal_y는 stage 로딩할 때 지정되며, G 위치.
 *   - 플레이어 현재 좌표(p->x, p->y)가 목표와 정확히 일치하면 클리어.
 *
 * 리턴값:
 *   - 1: 목표 도달 (스테이지 클리어)
 *   - 0: 아직 도달하지 못함
 */
int is_goal_reached(const Stage *stage, const Player *player) {
    // 좌표 비교: 동일하면 도착
    return (player->x == stage->goal_x && 
            player->y == stage->goal_y);
}



/* --------------------------------------------------------------
 * check_collision()
 * --------------------------------------------------------------
 * 플레이어가 장애물(obstacle)과 충돌했는지 확인하는 함수.
 *
 * 역할:
 *   - 플레이어가 한 칸 이동한 후 메인 루프에서 매 프레임 체크한다.
 *   - 장애물은 계속 움직이고 있으므로 실시간 충돌 체크 필요.
 *   - obstacle 스레드가 갱신하는 stage->obstacles 배열을 순회한다.
 *
 * 리턴값:
 *   - 1: 장애물과 충돌하여 사망
 *   - 0: 충돌 없음
 *
 * 중요:
 *   - 멀티스레드 환경에서 stage->obstacles 접근 시 mutex가 있어야 한다.
 *     (실제 메인에서 mutex lock 후 호출함)
 */
int check_collision(const Stage *stage, const Player *player) {

    // 모든 장애물을 순회하며 플레이어 위치와 비교
    for (int i = 0; i < stage->num_obstacles; i++) {
        
        const Obstacle *o = &stage->obstacles[i];   // i번째 장애물 참조

        // x, y 좌표가 동일하면 충돌
        if (o->x == player->x && 
            o->y == player->y) {
            return 1;   // 충돌
        }
    }

    return 0;   // 충돌 없음
}
