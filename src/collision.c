#include "../include/collision.h"

int is_world_position_blocked(const Stage *stage, int world_x, int world_y, CollisionInfo *info)
{
    if (!stage)
    {
        return 1;
    }

    const int tile_size = SUBPIXELS_PER_TILE;
    const int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    const int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    const int world_limit_x = stage_width * tile_size;
    const int world_limit_y = stage_height * tile_size;

    if (info)
    {
        info->overlap_top = 0;
        info->overlap_bottom = 0;
        info->overlap_left = 0;
        info->overlap_right = 0;
    }

    if (world_x < 0 || world_y < 0)
    {
        if (info)
        {
            if (world_x < 0)
                info->overlap_left = tile_size;
            if (world_y < 0)
                info->overlap_top = tile_size;
        }
        return 1;
    }

    if (world_x + tile_size > world_limit_x ||
        world_y + tile_size > world_limit_y)
    {
        if (info)
        {
            if (world_x + tile_size > world_limit_x)
                info->overlap_right = tile_size;
            if (world_y + tile_size > world_limit_y)
                info->overlap_bottom = tile_size;
        }
        return 1;
    }

    int min_tile_x = world_x / tile_size;
    int min_tile_y = world_y / tile_size;
    int max_tile_x = (world_x + tile_size - 1) / tile_size;
    int max_tile_y = (world_y + tile_size - 1) / tile_size;

    int blocked = 0;
    for (int ty = min_tile_y; ty <= max_tile_y; ++ty)
    {
        if (ty < 0 || ty >= stage_height)
        {
            return 1;
        }
        for (int tx = min_tile_x; tx <= max_tile_x; ++tx)
        {
            if (tx < 0 || tx >= stage_width)
            {
                return 1;
            }

            if (stage->map[ty][tx] == '#' || stage->map[ty][tx] == '@')
            {
                if (!info)
                {
                    return 1;
                }

                blocked = 1;
                int tile_left = tx * tile_size;
                int tile_right = tile_left + tile_size;
                int tile_top = ty * tile_size;
                int tile_bottom = tile_top + tile_size;

                int overlap_w = (world_x + tile_size < tile_right ? world_x + tile_size : tile_right) -
                                (world_x > tile_left ? world_x : tile_left);
                int overlap_h = (world_y + tile_size < tile_bottom ? world_y + tile_size : tile_bottom) -
                                (world_y > tile_top ? world_y : tile_top);

                if (overlap_w <= 0 || overlap_h <= 0)
                {
                    continue;
                }

                if (tile_right > world_x && tile_left <= world_x)
                {
                    if (overlap_w > info->overlap_left)
                        info->overlap_left = overlap_w;
                }
                if (tile_left < world_x + tile_size && tile_right >= world_x + tile_size)
                {
                    if (overlap_w > info->overlap_right)
                        info->overlap_right = overlap_w;
                }
                if (tile_bottom > world_y && tile_top <= world_y)
                {
                    if (overlap_h > info->overlap_top)
                        info->overlap_top = overlap_h;
                }
                if (tile_top < world_y + tile_size && tile_bottom >= world_y + tile_size)
                {
                    if (overlap_h > info->overlap_bottom)
                        info->overlap_bottom = overlap_h;
                }
            }
        }
    }

    return blocked;
}
