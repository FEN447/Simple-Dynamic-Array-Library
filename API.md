# dnmc_arr API 参考

## 目录

- [常量](#常量)
- [生命周期](#生命周期)
- [容量操作](#容量操作)
- [元素操作](#元素操作)
- [状态查询](#状态查询)
- [状态码详解](#状态码详解)
- [错误处理](#错误处理)
- [完整示例](#完整示例)

---

## 常量

### DA_INIT_ALLOC

```c
#ifndef DA_INIT_ALLOC
#define DA_INIT_ALLOC 64
#endif
```

**说明**

初始分配字节数。可在包含头文件前定义以自定义。

**约束**

- 必须 ≥ 64
- 必须是 2 的幂
- 编译期检查，不满足则编译失败

**示例**

```c
#define DA_INIT_ALLOC 128
#include "dnmc_arr.h"
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
| **返回** `NULL` | `esize == 0`，创建失败 |
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
| **返回** `0` | 释放成功 |
| **返回** `-2` | `darr` 为无效指针 |

```c
int *arr = darr_init(sizeof(int));
int err = darr_free(arr);

// 错误：重复释放或无效指针
int x = 10;
darr_free(&x);  // 返回 -2
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

**返回值与状态码的关系**

| 返回值 | 与输入 `darr` 比较 | `darr_stat()` | 含义 |
|--------|-------------------|---------------|------|
| `NULL` | - | - | `darr` 无效 |
| 非 `NULL` | 地址相同 | `-1` | 失败（内存不足），数组未变 |
| 非 `NULL` | 地址相同 | `0` | 成功，未重新分配内存 |
| 非 `NULL` | 地址不同 | `1` | 成功，已重新分配内存 |

**注意**

- 返回值是否等于原指针**不是**判断成败的标准
- 必须通过 `darr_stat()` 确认操作结果
- 扩容时新元素未初始化
- 缩容时数据截断

```c
int *arr = darr_init(sizeof(int));
int *old = arr;

arr = darr_setlen(arr, 100);

switch (darr_stat(arr)) {
    case -1: 
        // 失败，arr == old，长度未变
        break;
    case 0:  
        // 成功未移动，arr == old
        break;
    case 1:  
        // 成功已移动，arr != old，old 已失效
        break;
}

// 初始化新元素（扩容后未初始化）
for (int i = 0; i < 100; i++) arr[i] = i;
```

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
| **溢出** | 若 `len > SIZE_MAX - darr_len(arr)`，状态码 `-1` |

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

**状态码**

| 状态码 | 含义 |
|--------|------|
| `-1` | `realloc` 失败（极罕见） |
| `0` | 成功，未移动 |
| `1` | 成功，已移动 |

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

**状态码**

继承 `darr_clear` 的结果。

**典型用途**：复用内存作为不同类型数组。

```c
int *iarr = darr_init(sizeof(int));
// ... 使用 int 数组 ...

// 复用内存作为 char 数组
char *carr = darr_reset(iarr, sizeof(char));
// 注意：iarr 不再有效，必须使用 carr

carr = darr_push(carr, "A");
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

**状态码**

| 状态码 | 含义 |
|--------|------|
| `-1` | 内存不足 |
| `0` | 成功，未重新分配 |
| `1` | 成功，已重新分配 |

**警告**

- `data` 为 `NULL` 导致未定义行为（未检查）
- 仅复制 `esize` 字节，不存储 `data` 指针

```c
int *arr = darr_init(sizeof(int));

int val = 42;
arr = darr_push(arr, &val);  // arr[0] = 42

val = 100;
arr = darr_push(arr, &val);  // arr[1] = 100

// 错误：data 为 NULL
arr = darr_push(arr, NULL);  // 未定义行为！
```

---

## 状态查询

### darr_stat

```c
int darr_stat(void* darr);
```

查询上一次修改操作的状态。

| 返回值 | 含义 |
|--------|------|
| `-2` | `darr` 无效 |
| `-1` | 上一次操作失败 |
| `0` | 上一次操作成功，未移动内存 |
| `1` | 上一次操作成功，已移动内存 |

**影响状态的函数**

- `darr_setlen`
- `darr_addlen`
- `darr_clear`
- `darr_push`

**不影响状态的函数**

- `darr_init`（刚创建，无操作）
- `darr_free`（已释放，无法查询）
- `darr_reset`（继承 `darr_clear` 的状态）

```c
arr = darr_push(arr, &val);
if (darr_stat(arr) == -1) {
    // push 失败，数组未变
}
```

---

### darr_len / darr_tot / darr_rem

```c
long long darr_len(void* darr);  // 已用元素个数
long long darr_tot(void* darr);  // 总容量（元素个数）
long long darr_rem(void* darr);  // 剩余容量（元素个数）
```

| 返回值 | 含义 |
|--------|------|
| `-2` | `darr` 无效 |
| `≥0` | 查询结果 |

**关系**

```c
darr_tot(arr) == darr_len(arr) + darr_rem(arr)
```

**注意**

- 返回 `long long` 以容纳错误码 `-2`
- 正常结果可安全转换为 `size_t`

```c
printf("used: %lld, total: %lld, free: %lld\n",
       darr_len(arr), darr_tot(arr), darr_rem(arr));
```

---

## 状态码详解

### 状态码汇总表

| 状态码 | 含义 | 触发场景 |
|--------|------|----------|
| `-2` | 无效指针 | 任何查询/操作函数收到无效 `darr` |
| `-1` | 操作失败 | 内存不足、溢出、realloc 失败 |
| `0` | 成功（原地） | 操作成功，未重新分配内存 |
| `1` | 成功（移动） | 操作成功，已重新分配内存，指针可能改变 |

### 各函数状态码行为

| 函数 | 可能返回的状态 | 说明 |
|------|----------------|------|
| `darr_init` | 无 | 通过返回值判断（NULL = 失败） |
| `darr_free` | `-2` | 无效指针检测 |
| `darr_setlen` | `-1, 0, 1` | 扩容/缩容都可能触发 realloc |
| `darr_addlen` | `-1, 0, 1` | 委托给 `darr_setlen` |
| `darr_clear` | `-1, 0, 1` | 缩容可能触发 realloc |
| `darr_reset` | 继承 `clear` | 内部调用 `darr_clear` |
| `darr_push` | `-1, 0, 1` | 容量不足时触发扩容 |
| `darr_stat` | `-2, -1, 0, 1` | 查询状态本身也可能失败 |
| `darr_len/tot/rem` | `-2, ≥0` | 仅检测无效指针 |

### 返回值与状态码关系总结

对于可能修改数组的函数（`setlen`, `addlen`, `clear`, `push`, `reset`）：

| 返回值 | 必须更新指针 | 判断结果方式 |
|--------|-------------|-------------|
| `NULL` | 否（操作未发生） | `darr` 无效 |
| 非 `NULL` | **是** | 通过 `darr_stat()` 判断成功/失败及是否移动 |

**关键原则**：

1. 返回 `NULL` = 无效输入，原指针仍有效（但可能已损坏）
2. 返回非 `NULL` = 必须用返回值替换原指针，无论地址是否相同
3. 地址是否变化**不表示**成败，只表示是否发生 realloc
4. 唯一判断成败的方式是 `darr_stat()`

---

## 错误处理

### 策略一：立即检查（推荐用于关键操作）

```c
arr = darr_setlen(arr, 10000);
if (darr_stat(arr) == -1) {
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

if (darr_stat(arr) == -1) {
    // 最后一次操作失败，前面可能已成功
    // 需要更细粒度检查才能确定哪步失败
}
```

### 策略三：断言（调试阶段）

```c
#include <assert.h>

arr = darr_setlen(arr, 100);
assert(darr_stat(arr) != -1);  // 失败直接终止，便于调试
```

### 策略四：忽略（确定不会失败时）

```c
// 确定容量充足时
if (darr_rem(arr) > 0) {
    arr = darr_push(arr, &val);  // 必定成功
}
```

---

## 注意事项

1. **始终使用返回值**：所有可能 realloc 的函数（`setlen`, `addlen`, `clear`, `push`, `reset`）都必须用返回值更新原指针

2. **检查状态码**：生产代码应检查 `darr_stat` 返回值，特别是大内存分配

3. **无效指针检测**：库会检测明显的无效指针（NULL、未对齐、损坏的 header），但无法捕获所有野指针

4. **线程安全**：无内置同步，多线程访问需外部加锁

5. **变长数据**：不支持变长元素（如柔性数组、指针指向的外部数据），仅支持固定大小类型

6. **返回值不等于成败**：返回的指针地址是否变化只表示是否发生 realloc，不代表操作成功或失败
