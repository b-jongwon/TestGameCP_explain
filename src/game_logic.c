#include <stdio.h>
#include "../include/game.h"
#include "../include/player.h"   
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
 *   - 플레이어 현재 좌표(world_x/world_y)가 목표 타일 중심을 침범하면 클리어.
 *
 * 리턴값:
 *   - 1: 목표 도달 (스테이지 클리어)
 *   - 0: 아직 도달하지 못함
 */
int is_goal_reached(const Stage *stage, const Player *player) {
    if (!stage || !player) return 0;
    if (!player->has_backpack) return 0;

    return is_tile_center_inside_player(player, stage->start_x, stage->start_y);
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
int check_collision(Stage *stage, Player *player) {

     for (int i = 0; i < stage->num_obstacles; i++)
    {
        Obstacle *o = &stage->obstacles[i];
        if (!o->active) continue;

        if (o->kind == OBSTACLE_KIND_BREAKABLE_WALL) continue; // 깨지는 벽.

        int center_x = o->world_x + SUBPIXELS_PER_TILE / 2;
        int center_y = o->world_y + SUBPIXELS_PER_TILE / 2;
        if (is_world_point_inside_player(player, center_x, center_y))
        {
            // 보호막이 있으면 충돌 무효화
            if (player->shield_count > 0)
            {
                player->shield_count--;
                o->active = 0;   // 장애물 제거
                printf("Shield used! Remaining: %d\n", player->shield_count);
                return 0;        // 죽지 않음
            }

            // 보호막 없으면 진짜 충돌 → 죽음
            return 1;
        }
    }
    return 0;
}
