#include "../dnmc_arr.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * 示例：基础用法演示
 * 
 * 展示 dnmc_arr 的核心功能：
 * - 创建/释放动态数组
 * - 添加元素（自动扩容）
 * - 查询数组状态（长度/容量/剩余空间）
 * - 调整数组大小（扩容/缩容）
 * - 清空复用（改变元素类型）
 */

typedef struct {
    int x, y;
} Point;

int main(void) {
    // 1. 创建数组
    Point *pts = darr_init(sizeof(Point));
    if (!pts) {
        fprintf(stderr, "创建失败\n");
        return 1;
    }

    // 2. 添加元素
    for (int i = 0; i < 100; i++) {
        Point p = {i, i * 2};
        pts = darr_push(pts, &p);
        if (darr_stat(pts) == -1) {
            fprintf(stderr, "内存不足\n");
            darr_free(pts);
            return 1;
        }
    }

    // 3. 查询状态
    printf("点数: %lld, 容量: %lld, 剩余: %lld\n",
           darr_len(pts), darr_tot(pts), darr_rem(pts));

    // 4. 访问元素（直接像原生数组一样使用）
    printf("pts[50] = (%d, %d)\n", pts[50].x, pts[50].y);

    // 5. 调整大小
    pts = darr_setlen(pts, 10);  // 缩容到 10 个元素
    printf("缩容后: %lld\n", darr_len(pts));

    // 6. 清空复用
    int *nums = darr_reset(pts, sizeof(int));
    for (int i = 0; i < 5; i++) {
        nums = darr_push(nums, &i);
    }

    // 7. 释放
    darr_free(nums);

    return 0;
}