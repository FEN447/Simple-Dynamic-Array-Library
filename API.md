# dnmc_arr API 参考

## 目录

- [常量](#常量)
- [生命周期](#生命周期)
- [容量操作](#容量操作)
- [元素操作](#元素操作)
- [状态查询](#状态查询)
- [状态码详解](#状态码详解)
- [错误处理](#错误处理)
- [编译配置](#编译配置)
- [完整示例](#完整示例)

---

## 常量

### 状态码定义

```c
#define DARR_STAT_EMPTY 0  // 空状态/初始状态
#define DARR_STAT_OK    1  // 操作成功（未移动）
#define DARR_STAT_MOVED 2  // 操作成功（内存已移动）
#define DARR_STAT_FAIL  3  // 操作失败
```

### 功能码定义

```c
#define DARR_FUNC_OK   0   // 函数执行成功
#define DARR_FUNC_FAIL -1  // 函数执行失败
```

---

## 生命周期

### darr_init

```c
void* darr_init(size_t esize);
```

创建动态数组。

| 项目 | 说明 |
|------|------|
| **参数** `esize` | 单个元素的字节大小，必须 > 0 |
| **返回** `NULL` | `esize == 0` 或 `esize` 超过限制，创建失败 |
| **返回** 非NULL | 数组指针，指向首个元素位置 |

```c
int    *iarr = darr_init(sizeof(int));
double *darr = darr_init(sizeof(double));

// 错误：esize = 0
void *bad = darr_init(0);  // 返回 NULL
```

---

### darr_free

```c
int darr_free(void* darr);
```

释放动态数组内存。

| 项目 | 说明 |
|------|------|
| **参数** `darr` | 由 `darr_init` 返回的指针 |
| **返回** `DARR_FUNC_OK` (0) | 释放成功 |
| **返回** `DARR_FUNC_FAIL` (-1) | `darr` 为无效指针 |

```c
int *arr = darr_init(sizeof(int));
int err = darr_free(arr);  // 返回 0

// 错误：无效指针
int x = 10;
darr_free(&x);  // 返回 -1
```

---

## 容量操作

### darr_setlen

```c
void* darr_setlen(void* darr, size_t len);
```

设置数组元素个数为指定值。

| 项目 | 说明 |
|------|------|
| **参数** `darr` | 数组指针 |
| **参数** `len` | 目标元素个数，可为 0 |
| **返回** `NULL` | `darr` 无效 |
| **返回** 非 `NULL` | 操作完成，**必须**用返回值更新原指针 |

**重要警告**

```c
// 严重错误：忽略返回值
darr_setlen(arr, 100);  // 若内存移动，arr 成悬空指针
arr[0] = 1;             // 未定义行为！

// 正确做法
arr = darr_setlen(arr, 100);
```

**注意**

- 返回值是否等于原指针**不是**判断成败的标准
- 扩容时新元素未初始化
- 缩容时数据截断

---

### darr_addlen

```c
void* darr_addlen(void* darr, size_t len);
```

增加数组元素个数。

**等价于**

```c
darr_setlen(arr, darr_len(arr) + len)
```

| 项目 | 说明 |
|------|------|
| **参数** `len` | 新增元素个数，可为 0 |
| **返回** | 同 `darr_setlen` |

```c
int *arr = darr_init(sizeof(int));
arr = darr_addlen(arr, 10);   // len = 10
arr = darr_addlen(arr, 20);   // len = 30
arr = darr_addlen(arr, 0);    // len = 30（无变化）
```

---

### darr_clear

```c
void* darr_clear(void* darr);
```

清空数组：长度置 0，容量重置为初始值。

| 项目 | 说明 |
|------|------|
| **返回** `NULL` | `darr` 无效 |
| **返回** 非 `NULL` | 操作完成，**必须**用返回值更新原指针 |

**注意**

- 数据逻辑清空，物理内存可能未归零
- 失败极罕见（缩容通常成功）

```c
int *arr = darr_init(sizeof(int));
arr = darr_setlen(arr, 1000);
// ... 使用 ...

arr = darr_clear(arr);  // len = 0，容量回到初始值
```

---

### darr_reset

```c
void* darr_reset(void* darr, size_t esize);
```

清空数组并更改元素类型大小。

| 项目 | 说明 |
|------|------|
| **参数** `esize` | 新的元素大小，必须 > 0 |
| **返回** `NULL` | `darr` 无效或 `esize == 0` |
| **返回** 非 `NULL` | 成功，**必须**用返回值更新原指针 |

**典型用途**：复用内存作为不同类型数组。

```c
int *iarr = darr_init(sizeof(int));
// ... 使用 int 数组 ...

// 复用内存作为 char 数组
char *carr = darr_reset(iarr, sizeof(char));
// 注意：iarr 不再有效，必须使用 carr

int val = 65;
carr = darr_push(carr, &val);
```

---

## 元素操作

### darr_push

```c
void* darr_push(void* darr, void* data);
```

在数组尾部添加一个元素。

| 项目 | 说明 |
|------|------|
| **参数** `darr` | 数组指针 |
| **参数** `data` | 指向待复制数据的指针，**必须非 NULL** |
| **返回** `NULL` | `darr` 无效 |
| **返回** 非 `NULL` | 操作完成，**必须**用返回值更新原指针 |

**警告**

- `data` 为 `NULL` 导致未定义行为（未检查）
- 仅复制 `esize` 字节，不存储 `data` 指针

```c
int *arr = darr_init(sizeof(int));
char buf[33];

int val = 42;
arr = darr_push(arr, &val);  // arr[0] = 42

val = 100;
arr = darr_push(arr, &val);  // arr[1] = 100

// 查看状态
printf("状态: %s\n", darr_stat(arr, buf, sizeof(buf)));

// 错误：data 为 NULL
arr = darr_push(arr, NULL);  // 未定义行为！
```

---

## 状态查询

### darr_stat

```c
char* darr_stat(void* darr, char* buf, size_t len);
```

将状态历史转换为字符串存入缓冲区。

| 参数 | 说明 |
|------|------|
| `darr` | 数组指针 |
| `buf` | 字符缓冲区，用于接收状态字符串 |
| `len` | 缓冲区长度，必须 ≥ 2 |

| 返回值 | 含义 |
|--------|------|
| `NULL` | `darr` 无效，或 `buf` 为 NULL，或 `len` < 2 |
| 非 `NULL` | 返回 `buf`，内容为状态字符串 |

**状态字符串格式**

- 字符 `'0'` = `DARR_STAT_EMPTY` (0)
- 字符 `'1'` = `DARR_STAT_OK` (1)  
- 字符 `'2'` = `DARR_STAT_MOVED` (2)
- 字符 `'3'` = `DARR_STAT_FAIL` (3)

**字符串方向**：从高位到低位（从左到右），**最右侧字符为最新状态**。

```c
int *arr = darr_init(sizeof(int));
char buf[33];

arr = darr_push(arr, &(int){1});
arr = darr_push(arr, &(int){2});
arr = darr_setlen(arr, 100);  // 可能触发扩容

// 假设输出 "000000000000000000000000000000112"
// 解析：'0'(初始) -> '1'(第一次push成功) -> '1'(第二次push成功) -> '2'(setlen移动内存)
printf("状态历史: %s\n", darr_stat(arr, buf, sizeof(buf)));

// 只看最新状态（最右边字符）
char latest = buf[strlen(buf) - 1];
switch (latest) {
    case '0': printf("空状态\n"); break;
    case '1': printf("成功（未移动）\n"); break;
    case '2': printf("成功（已移动）\n"); break;
    case '3': printf("失败\n"); break;
}
```

**缓冲区长度建议**

```c
// 最小需求：能容纳最多 DARR_BITS_MAX_CALC/2 个状态字符 + '\0'
// 64位系统：64/2 = 32 字符 + 1 = 33 字节
// 32位系统：32/2 = 16 字符 + 1 = 17 字节
// 16位系统：16/2 = 8  字符 + 1 = 9  字节
char buf[33];  // 64位系统最小需求
char buf[17];  // 32位系统最小需求
char buf[9];   // 16位系统最小需求
```

---

### darr_stat_clear

```c
int darr_stat_clear(void* darr);
```

清空状态历史。

| 返回值 | 含义 |
|--------|------|
| `DARR_FUNC_OK` (0) | 成功 |
| `DARR_FUNC_FAIL` (-1) | `darr` 无效 |

```c
// 重置状态，开始新的记录周期
darr_stat_clear(arr);
// 现在 darr_stat 只显示从此刻开始的状态
```

---

### darr_len / darr_tot / darr_rem

```c
int64_t darr_len(void* darr);  // 已用元素个数
int64_t darr_tot(void* darr);  // 总容量（元素个数）
int64_t darr_rem(void* darr);  // 剩余容量（元素个数）
```

| 返回值 | 含义 |
|--------|------|
| `DARR_FUNC_FAIL` (-1) | `darr` 无效 |
| `≥0` | 查询结果 |

**关系**

```c
darr_tot(arr) == darr_len(arr) + darr_rem(arr)
```

**注意**

- 返回 `int64_t` 以容纳错误码 `-1`
- 正常结果可安全转换为 `size_t`（如果确定有效）

```c
printf("used: %lld, total: %lld, free: %lld\n",
       darr_len(arr), darr_tot(arr), darr_rem(arr));
```

---

## 状态码详解

### 状态码汇总表

| 状态码 | 字符 | 含义 | 触发场景 |
|--------|------|------|----------|
| `0` | `'0'` | 空状态 | 初始状态，或 `darr_stat_clear` 后 |
| `1` | `'1'` | 操作成功（原地） | 操作成功，未重新分配内存 |
| `2` | `'2'` | 操作成功（移动） | 操作成功，已重新分配内存，指针可能改变 |
| `3` | `'3'` | 操作失败 | 内存不足、溢出、realloc 失败 |

### 各函数状态码行为

| 函数 | 可能记录的状态 | 说明 |
|------|----------------|------|
| `darr_init` | `0` | 初始状态为空 |
| `darr_free` | 无 | 释放后无法查询 |
| `darr_setlen` | `1, 2, 3` | 扩容/缩容都可能触发 realloc |
| `darr_addlen` | `1, 2, 3` | 同上 |
| `darr_clear` | `1, 2, 3` | 缩容可能触发 realloc |
| `darr_reset` | `1, 2, 3` | 同上 |
| `darr_push` | `1, 2, 3` | 容量不足时触发扩容 |
| `darr_stat` | 无 | 查询不修改状态 |
| `darr_stat_clear` | 重置为 `0` | 清空历史 |

### 返回值与状态码关系总结

对于可能修改数组的函数（`setlen`, `addlen`, `clear`, `push`, `reset`）：

| 返回值 | 必须更新指针 | 判断结果方式 |
|--------|-------------|-------------|
| `NULL` | 否（操作未发生） | `darr` 无效 |
| 非 `NULL` | **是** | 通过 `darr_stat()` 字符串最右字符判断 |

**关键原则**：

1. 返回 `NULL` = 无效输入，原指针仍有效（但可能已损坏）
2. 返回非 `NULL` = 必须用返回值替换原指针，无论地址是否相同
3. 地址是否变化**不表示**成败，只表示是否发生 realloc
4. 判断成败需查看 `darr_stat()` 返回字符串的最右字符

---

## 错误处理

### 策略一：立即检查（推荐用于关键操作）

```c
int *arr = darr_init(sizeof(int));
char buf[33];

arr = darr_setlen(arr, 10000);
char *stat = darr_stat(arr, buf, sizeof(buf));
if (stat && stat[strlen(stat)-1] == '3') {  // '3' = FAIL
    fprintf(stderr, "内存不足\n");
    return ERROR;
}
// 继续安全使用
```

### 策略二：批量后检查（用于连续操作）

```c
arr = darr_setlen(arr, 100);
arr = darr_push(arr, &a);
arr = darr_push(arr, &b);
arr = darr_push(arr, &c);

char *stat = darr_stat(arr, buf, sizeof(buf));
if (stat[strlen(stat)-1] == '3') {
    // 最后一次操作失败，前面可能已成功
}
```

### 策略三：断言（调试阶段）

```c
#include <assert.h>

arr = darr_setlen(arr, 100);
char *stat = darr_stat(arr, buf, sizeof(buf));
assert(stat[strlen(stat)-1] != '3');  // 失败直接终止
```

### 策略四：忽略（确定不会失败时）

```c
// 确定容量充足时
if (darr_rem(arr) > 0) {
    arr = darr_push(arr, &val);  // 必定成功
}
```

---

## 编译配置

### 限制最大容量

在包含头文件**之前**定义以下宏**之一**，可限制数组最大容量：

| 宏 | 效果 | 最大容量 |
|----|------|---------|
| `DARR_SIZE_MAX_16` | 限制为 16 位 | `UINT16_MAX` (65535) |
| `DARR_SIZE_MAX_32` | 限制为 32 位 | `UINT32_MAX` (约 42 亿) |
| `DARR_SIZE_MAX_64` | 限制为 64 位 | `UINT64_MAX` (约 1.8e19) |

**默认行为**：不定义任何宏时，使用 `SIZE_MAX`（与平台 `size_t` 相同）。

**约束**：同时定义多个 `DARR_SIZE_MAX_XX` 宏会导致编译错误。

```c
// 限制为 32 位（节省内存，适合嵌入式）
#define DARR_SIZE_MAX_32
#include "dnmc_arr.h"

// 错误：同时定义多个
#define DARR_SIZE_MAX_32
#define DARR_SIZE_MAX_64
#include "dnmc_arr.h"  // 编译错误："Multiple DARR_SIZE_MAX macros defined."
```

### 启用快速模式

定义 `DARR_FAST` 宏可跳过部分运行时验证，提升性能：

```c
#define DARR_FAST
#include "dnmc_arr.h"
```

**警告**：快速模式下不检查指针有效性，错误的指针会导致未定义行为。仅用于性能关键且已验证正确的代码。

---

## 完整示例

### 基础用法

```c
#include "dnmc_arr.h"
#include <stdio.h>

int main(void) {
    // 创建
    int *arr = darr_init(sizeof(int));
    if (!arr) {
        fprintf(stderr, "创建失败\n");
        return 1;
    }

    char stat_buf[33];  // 状态缓冲区

    // 添加元素
    for (int i = 0; i < 10; i++) {
        arr = darr_push(arr, &i);
        if (!arr) {
            fprintf(stderr, "push失败\n");
            return 1;
        }
    }

    // 查看状态
    printf("状态历史: %s\n", darr_stat(arr, stat_buf, sizeof(stat_buf)));
    printf("最新状态: %c\n", stat_buf[strlen(stat_buf)-1]);

    // 查询容量
    printf("长度: %lld, 容量: %lld, 剩余: %lld\n",
           darr_len(arr), darr_tot(arr), darr_rem(arr));

    // 使用
    for (int i = 0; i < darr_len(arr); i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    // 释放
    darr_free(arr);
    return 0;
}
```

### 类型复用

```c
#include "dnmc_arr.h"
#include <stdio.h>

int main(void) {
    // 先作为 int 数组使用
    int *iarr = darr_init(sizeof(int));
    for (int i = 0; i < 5; i++) {
        iarr = darr_push(iarr, &i);
    }

    // 清空并改为 double 数组
    double *darr = darr_reset(iarr, sizeof(double));
    // 注意：iarr 指针已失效，必须使用 darr

    double val = 3.14;
    darr = darr_push(darr, &val);

    printf("double: %f\n", darr[0]);

    darr_free(darr);
    return 0;
}
```

---

## 注意事项

1. **始终使用返回值**：所有可能 realloc 的函数（`setlen`, `addlen`, `clear`, `push`, `reset`）都必须用返回值更新原指针

2. **预分配状态缓冲区**：使用 `darr_stat` 前确保提供足够大的缓冲区（建议 ≥64 字节）

3. **状态字符串方向**：字符串从左到右为从旧到新，**最右字符为最新状态**

4. **清空状态历史**：使用 `darr_stat_clear` 重置记录，避免历史干扰

5. **线程安全**：无内置同步，多线程访问需外部加锁

6. **变长数据**：不支持变长元素（如柔性数组），仅支持固定大小类型

7. **编译配置**：使用 `DARR_SIZE_MAX_16/32/64` 限制最大容量（三选一），使用 `DARR_FAST` 启用快速模式
