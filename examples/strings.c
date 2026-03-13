#include "../dnmc_arr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char **strs = darr_init(sizeof(char*));
    if (!strs) {
        fprintf(stderr, "创建失败\n");
        return 1;
    }

    const char *samples[] = {"Hello", "World", "dnmc_arr", "Dynamic", "Array"};
    int n = sizeof(samples) / sizeof(samples[0]);

    for (int i = 0; i < n; i++) {
        char *copy = malloc(strlen(samples[i]) + 1);
        if (!copy) {
            fprintf(stderr, "字符串内存不足\n");
            break;
        }
        strcpy(copy, samples[i]);

        strs = darr_push(strs, &copy);
        // 新版：检查返回值
        if (!strs) {
            fprintf(stderr, "数组内存不足\n");
            free(copy);
            break;
        }
    }

    printf("添加 %lld 个字符串\n", darr_len(strs));

    for (int i = 0; i < darr_len(strs); i++) {
        printf("  strs[%d] = \"%s\"\n", i, strs[i]);
    }

    printf("\n在索引 2 处插入新元素...\n");
    strs = darr_setlen(strs, darr_len(strs) + 1);
    
    // 新版：用 darr_len 检查是否成功，或用状态缓冲区
    if (darr_len(strs) > 0) {
        for (int i = darr_len(strs) - 1; i > 2; i--) {
            strs[i] = strs[i - 1];
        }
        char *new_str = malloc(strlen("INSERTED") + 1);
        strcpy(new_str, "INSERTED");
        strs[2] = new_str;
    }

    printf("插入后：\n");
    for (int i = 0; i < darr_len(strs); i++) {
        printf("  strs[%d] = \"%s\"\n", i, strs[i]);
    }

    printf("\n删除索引 3...\n");
    free(strs[3]);
    for (int i = 3; i < darr_len(strs) - 1; i++) {
        strs[i] = strs[i + 1];
    }
    strs = darr_setlen(strs, darr_len(strs) - 1);

    printf("删除后（共 %lld 个）：\n", darr_len(strs));
    for (int i = 0; i < darr_len(strs); i++) {
        printf("  strs[%d] = \"%s\"\n", i, strs[i]);
    }

    printf("\n清理内存...\n");
    for (int i = 0; i < darr_len(strs); i++) {
        free(strs[i]);
    }
    darr_free(strs);

    printf("完成\n");
    return 0;
}
