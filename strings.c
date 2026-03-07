#include "../dnmc_arr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
 * 示例：动态字符串数组（二维动态数组）
 * 展示 dnmc_arr 的嵌套使用和复杂场景
 */

int main(void) {
    // 1. 创建字符串指针数组（存储 char*）
    char **strs = darr_init(sizeof(char*));
    if (!strs) {
        fprintf(stderr, "创建失败\n");
        return 1;
    }

    // 2. 动态添加字符串（需要手动管理字符串内存）
    const char *samples[] = {"Hello", "World", "dnmc_arr", "Dynamic", "Array"};
    int n = sizeof(samples) / sizeof(samples[0]);

    for (int i = 0; i < n; i++) {
        // 为每个字符串分配内存
        char *copy = malloc(strlen(samples[i]) + 1);
        if (!copy) {
            fprintf(stderr, "字符串内存不足\n");
            break;
        }
        strcpy(copy, samples[i]);

        // 将字符串指针加入数组
        strs = darr_push(strs, &copy);
        if (darr_stat(strs) == -1) {
            fprintf(stderr, "数组内存不足\n");
            free(copy);
            break;
        }
    }

    printf("添加 %lld 个字符串\n", darr_len(strs));

    // 3. 遍历并打印
    for (int i = 0; i < darr_len(strs); i++) {
        printf("  strs[%d] = \"%s\"\n", i, strs[i]);
    }

    // 4. 在中间插入（通过 setlen + 手动移动）
    // 注意：dnmc_arr 不提供 insert，用户自行实现策略
    printf("\n在索引 2 处插入新元素...\n");

    // 先扩容
    strs = darr_setlen(strs, darr_len(strs) + 1);
    if (darr_stat(strs) != -1) {
        // 从后往前移动元素
        for (int i = darr_len(strs) - 1; i > 2; i--) {
            strs[i] = strs[i - 1];
        }
        // 插入新元素
        char *new_str = malloc(strlen("INSERTED") + 1);
        strcpy(new_str, "INSERTED");
        strs[2] = new_str;
    }

    // 5. 打印结果
    printf("插入后：\n");
    for (int i = 0; i < darr_len(strs); i++) {
        printf("  strs[%d] = \"%s\"\n", i, strs[i]);
    }

    // 6. 删除元素（手动移动 + setlen）
    printf("\n删除索引 3...\n");
    free(strs[3]);  // 先释放字符串内存
    for (int i = 3; i < darr_len(strs) - 1; i++) {
        strs[i] = strs[i + 1];
    }
    strs = darr_setlen(strs, darr_len(strs) - 1);

    printf("删除后（共 %lld 个）：\n", darr_len(strs));
    for (int i = 0; i < darr_len(strs); i++) {
        printf("  strs[%d] = \"%s\"\n", i, strs[i]);
    }

    // 7. 清理：先释放所有字符串，再释放数组
    printf("\n清理内存...\n");
    for (int i = 0; i < darr_len(strs); i++) {
        free(strs[i]);
    }
    darr_free(strs);

    printf("完成\n");
    return 0;
}
