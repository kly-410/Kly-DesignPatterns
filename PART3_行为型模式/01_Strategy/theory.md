# 策略模式（Strategy Pattern）

## 本章目标

1. 理解策略模式解决的问题：**行为变化的部分如何封装**
2. 掌握策略模式在 C 语言中的实现方式（函数指针）
3. 理解策略模式和"变化部分封装"原则的关系
4. 能识别 PCIe 驱动和 Linux 内核中的策略模式实例

---

## 1. 从一个问题开始

设计一个排序模块，需求是：

```
初始版本：只需要整数数组升序排序
后来：需要支持降序排序
再后来：需要支持按绝对值排序、按字符串长度排序...
```

**糟糕的设计**：每次加新排序方式，修改排序核心函数。

**好的设计**：排序核心函数不变，具体排序方式作为"策略"传入。

---

## 2. 策略模式的定义

> **策略模式**：定义了算法族，分别封装起来，让它们之间可以互相替换，此模式让算法的变化独立于使用它的客户。

三个关键词：
- **算法族**：一组相关的行为（升序、降序、绝对值）
- **封装**：每个算法独立，互不干扰
- **可替换**：运行时切换，不需要改客户代码

---

## 3. C 语言实现：排序策略

### 3.1 定义策略接口

```c
// 策略接口：用函数指针类型定义"比较行为"
typedef int (*CompareFunc)(const void *a, const void *b);

// qsort的标准比较器签名：接收两个const void*，返回int
// C语言里void*可以接受任意指针类型，这是最灵活的"接口"
```

### 3.2 具体策略实现

```c
// 策略A：升序（从小到大）
int compare_asc(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// 策略B：降序（从大到小）
int compare_desc(const void *a, const void *b) {
    return (*(int*)b - *(int*)a);
}

// 策略C：按绝对值排序
int compare_abs(const void *a, const void *b) {
    int aa = abs(*(int*)a);
    int bb = abs(*(int*)b);
    return aa - bb;
}
```

### 3.3 上下文（使用策略的函数）

```c
// 上下文：排序函数
// 它本身不包含任何排序逻辑，只负责"比较"和"交换"
void sort(void *arr, size_t n, size_t size, CompareFunc cmp) {
    char *base = (char*)arr;
    for (size_t i = 0; i < n - 1; i++) {
        for (size_t j = i + 1; j < n; j++) {
            if (cmp(base + i * size, base + j * size) > 0) {
                // 交换两个元素（泛型交换，用memcpy）
                char tmp[size];
                memcpy(tmp, base + i * size, size);
                memcpy(base + i * size, base + j * size, size);
                memcpy(base + j * size, tmp, size);
            }
        }
    }
}
```

### 3.4 客户代码（运行时切换策略）

```c
int main(void) {
    int arr[] = {3, 1, 4, 1, 5};

    sort(arr, 5, sizeof(int), compare_asc);   // 升序
    sort(arr, 5, sizeof(int), compare_desc);  // 降序，sort函数本身一行没改
}
```

---

## 4. 策略模式的类图（C语言视角）

```
┌──────────────────────────────────────────────────────┐
│                  上下文（Context）                    │
│              void sort(..., CompareFunc cmp)        │
│                        │                             │
│                        │ 持有并调用策略               │
└────────────────────────┼────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────┐
│              CompareFunc（策略接口）                 │
│         typedef int (*)(const void*, const void*)  │
└──────────────────────────────────────────────────────┘
                         ▲
            ┌───────────┴───────────┐
            │                       │
    ┌───────┴───────┐       ┌───────┴───────┐
    │ compare_asc  │       │ compare_desc │
    └──────────────┘       └──────────────┘
```

**关键**：上下文不关心策略的具体实现，只负责调用。

---

## 5. 一个完整的 C 语言实例：出行策略

### 场景：旅行 app，支持多种出行方式

```c
// 抽象策略接口：出行方式
typedef struct {
    void (*go)(const char *destination);
} TravelStrategy;

// 具体策略：飞机
static void travel_by_plane(const char *dest) {
    printf("✈️  乘坐飞机去%s，3小时后到达\n", dest);
}

static const TravelStrategy PlaneStrategy = {
    .go = travel_by_plane,
};

// 具体策略：火车
static void travel_by_train(const char *dest) {
    printf("🚂  乘坐火车去%s，8小时后到达\n", dest);
}

static const TravelStrategy TrainStrategy = {
    .go = travel_by_train,
};

// 具体策略：步行（没钱的时候）
static void travel_by_walk(const char *dest) {
    printf("🚶  步行去%s，算了别去了太远了\n", dest);
}

static const TravelStrategy WalkStrategy = {
    .go = travel_by_walk,
};

// 上下文：旅程规划器
// 它持有当前使用的出行策略，可以运行时切换
typedef struct {
    const TravelStrategy *strategy;
} TravelPlanner;

// 设置出行策略
void planner_set_strategy(TravelPlanner *p, const TravelStrategy *s) {
    p->strategy = s;
}

// 规划行程：调用策略
void planner_travel(TravelPlanner *p, const char *dest) {
    p->strategy->go(dest);
}

int main(void) {
    TravelPlanner planner = {.strategy = &TrainStrategy};

    planner_travel(&planner, "北京");  // 坐火车去北京
    // output: 🚂 乘坐火车去北京，8小时后到达

    planner_set_strategy(&planner, &PlaneStrategy);  // 换成飞机
    planner_travel(&planner, "上海");  // 坐飞机去上海
    // output: ✈️ 乘坐飞机去上海，3小时后到达
}
```

---

## 6. Linux 内核里的策略模式

### 6.1 DMA 回调策略

```c
// DMA传输请求结构体
struct dma_async_tx_descriptor {
    dma_addr_t device_addr;
    void *addr;                       // 数据地址
    size_t len;                       // 数据长度
    void (*callback)(void *param);   // 传输完成后的回调策略
    void *param;                      // 回调参数
};
```

每个驱动可以实现自己的 callback，DMA 核心框架负责调用。

### 6.2 PWM 策略

```c
struct pwm_ops {
    int (*apply)(struct pwm_device *pwm, struct pwm_state *state);
    int (*get_state)(struct pwm_device *pwm, struct pwm_state *state);
    // ...
};
```

每个 PWM 芯片驱动实现自己的 apply/get_state，框架统一调用。

---

## 7. 策略模式的适用场景

**应该用：**
- 一个功能有多种实现方式，需要运行时选择
- 不希望用 `if (type == A) ... else if (type == B)` 一堆分支
- 不同实现之间没有共享状态，彼此独立

**不应该用：**
- 策略只有一种，不会变
- 策略非常简单，只有几行代码
- 策略之间需要共享大量状态（用享元模式或中介者模式更合适）

---

## 8. 本章小结

| 要素 | C 语言实现 |
|:---|:---|
| 策略接口 | `typedef` + 函数指针 |
| 具体策略 | 实现了函数指针签名的具体函数 |
| 上下文 | 持有策略指针，提供统一调用入口 |
| 运行时切换 | 修改变量指向不同的策略函数 |

**策略模式的核心**：把"会变化的部分"封装成策略，让"不变的部分"调用策略。

---

## 9. 练习

1. 实现一个计算器，支持 + - * / 四种运算策略
2. 实现一个日志模块，支持 UART / Memory / USB 三种输出策略
3. 读 Linux 内核 `include/linux/pci.h`，找出哪些地方用了策略模式

---

## 代码

`code.c` — 本章所有代码的完整可编译版本
`Makefile` — 全注释编译脚本
