#include "../dnmc_arr.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x, y;
} Point;

int main(void) {
    Point *pts = darr_init(sizeof(Point));
    if (!pts) {
        fprintf(stderr, "创建失败\n");
        return 1;
    }

    for (int i = 0; i < 100; i++) {
        Point p = {i, i * 2};
        pts = darr_push(pts, &p);
        // 新版：检查返回值是否为 NULL，或用 darr_stat_clear + darr_len 组合
        if (!pts) {
            fprintf(stderr, "内存不足\n");
            return 1;
        }
    }

    // 查询状态：新版 darr_stat 需要缓冲区
    char stat_buf[128];
    printf("点数: %lld, 容量: %lld, 剩余: %lld, 状态: %s\n",
           darr_len(pts), darr_tot(pts), darr_rem(pts),
           darr_stat(pts, stat_buf, sizeof(stat_buf)));

    printf("pts[50] = (%d, %d)\n", pts[50].x, pts[50].y);

    pts = darr_setlen(pts, 10);
    printf("缩容后: %lld\n", darr_len(pts));

    int *nums = darr_reset(pts, sizeof(int));
    for (int i = 0; i < 5; i++) {
        nums = darr_push(nums, &i);
    }

    darr_free(nums);
    return 0;
}
