/**
 * 策略模式（Strategy Pattern）— C语言实现
 * 
 * 本章对应《Head First 设计模式》第一章
 * 
 * 策略模式核心思想：
 * 把"会变化的行为"封装成独立的策略，上下文持有策略接口，
 * 运行时可以切换不同策略，而上下文本身保持不变。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/*====================================================================
 * 示例1：排序策略 — 最贴近实战的例子
 * 
 * 场景：固件里需要排序功能，最初只需要升序，后面需要降序、
 *       按绝对值排序等多种策略。用策略模式避免重复写排序逻辑。
 *====================================================================*/

// 策略接口：比较函数指针
// 这是C语言实现策略模式的核心：把"算法"当成指针传递
// 对应《Head First》里的FlyBehavior接口
typedef int (*CompareFunc)(const void *a, const void *b);

// 上下文：泛型排序
// 它本身不包含排序逻辑，只负责比较和交换
// 具体用哪种比较策略，由调用方传入的CompareFunc决定
void sort_generic(void *arr, size_t n, size_t elem_size, CompareFunc cmp) {
    char *base = (char*)arr;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            if (cmp(base + i * elem_size, base + j * elem_size) > 0) {
                char tmp[elem_size];
                memcpy(tmp, base + i * elem_size, elem_size);
                memcpy(base + i * elem_size, base + j * elem_size, elem_size);
                memcpy(base + j * elem_size, tmp, elem_size);
            }
        }
    }
}

// 具体策略1：整数升序
int compare_int_asc(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return ia - ib;
}

// 具体策略2：整数降序
int compare_int_desc(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return ib - ia;
}

// 具体策略3：按绝对值排序
int compare_int_abs(const void *a, const void *b) {
    int ia = abs(*(const int*)a);
    int ib = abs(*(const int*)b);
    return ia - ib;
}

// 具体策略4：按字符串长度排序
int compare_str_len(const void *a, const void *b) {
    const char *sa = *(const char**)a;
    const char *sb = *(const char**)b;
    return (int)strlen(sa) - (int)strlen(sb);
}

void print_int_array(const int *arr, size_t n, const char *label) {
    printf("  %s: ", label);
    for (size_t i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");
}

void demo_sort_strategy(void) {
    printf("=== 示例1：排序策略 ===\n");

    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    sort_generic(arr, n, sizeof(int), compare_int_asc);
    print_int_array(arr, n, "升序");

    sort_generic(arr, n, sizeof(int), compare_int_desc);
    print_int_array(arr, n, "降序");

    int arr2[] = {-3, 1, -4, 1, -5, 2};
    sort_generic(arr2, 6, sizeof(int), compare_int_abs);
    print_int_array(arr2, 6, "绝对值排序");

    const char *strs[] = {"hello", "a", "world!", "hi"};
    sort_generic(strs, 4, sizeof(char*), compare_str_len);
    printf("  字符串长度排序: ");
    for (int i = 0; i < 4; i++) printf("\"%s\" ", strs[i]);
    printf("\n\n");
}


/*====================================================================
 * 示例2：出行策略 — 模拟《Head First》鸭子的例子
 *====================================================================*/

// 抽象策略接口：出行方式
typedef void (*TravelFn)(const char *dest);

// 策略实现：飞机
static void travel_by_plane(const char *dest) {
    printf("✈️  乘坐飞机前往 %s，3小时后到达\n", dest);
}

// 策略实现：火车
static void travel_by_train(const char *dest) {
    printf("🚂  乘坐火车前往 %s，8小时后到达\n", dest);
}

// 策略实现：骑车
static void travel_by_bike(const char *dest) {
    printf("🚲  骑自行车前往 %s，2小时后到达\n", dest);
}

// 上下文：旅行规划器
typedef struct {
    TravelFn go;
    const char *planner_name;
} TravelPlanner;

void planner_set_strategy(TravelPlanner *p, TravelFn fn) {
    p->go = fn;
}

void planner_travel(TravelPlanner *p, const char *dest) {
    p->go(dest);
}

void demo_travel_strategy(void) {
    printf("=== 示例2：出行策略 ===\n");

    TravelPlanner planner = {
        .go = travel_by_train,
        .planner_name = "我的旅行规划"
    };

    printf("使用%s出行：\n", planner.planner_name);
    printf("  去北京：");
    planner.go("北京");

    planner.go = travel_by_plane;
    printf("  去上海：");
    planner.go("上海");

    planner.go = travel_by_bike;
    printf("  去郊区：");
    planner.go("郊区");
    printf("\n");
}


/*====================================================================
 * 示例3：LED 亮度控制策略 — 嵌入式实战
 *====================================================================*/

typedef int BrightnessLevel;  // 0-1000

typedef BrightnessLevel (*BrightnessMapFn)(BrightnessLevel target);

static BrightnessLevel brightness_linear(BrightnessLevel target) {
    return target;
}

static BrightnessLevel brightness_exp(BrightnessLevel target) {
    float t = (float)target / 1000.0f;
    float result = t * t;
    return (BrightnessLevel)(result * 1000.0f);
}

static BrightnessLevel brightness_s_curve(BrightnessLevel target) {
    float t = (float)target / 1000.0f;
    float s = 1.0f / (1.0f + exp(-10.0f * (t - 0.5f)));
    return (BrightnessLevel)(s * 1000.0f);
}

typedef struct {
    BrightnessMapFn map_brightness;
    BrightnessLevel current;
    const char *led_name;
} LEDController;

void led_set_target(LEDController *led, BrightnessLevel target) {
    if (led->map_brightness) {
        led->current = led->map_brightness(target);
    } else {
        led->current = target;
    }
    printf("  %s: 目标=%d -> 实际=%d\n",
           led->led_name, target, led->current);
}

void demo_led_brightness(void) {
    printf("=== 示例3：LED亮度控制策略（嵌入式实战） ===\n");

    LEDController led = {
        .map_brightness = brightness_linear,
        .current = 0,
        .led_name = "LED1"
    };

    printf("线性亮度策略：\n");
    led_set_target(&led, 500);

    printf("\n指数亮度策略（人眼感知）：\n");
    led.map_brightness = brightness_exp;
    led_set_target(&led, 500);

    printf("\nS曲线亮度策略（氛围灯）：\n");
    led.map_brightness = brightness_s_curve;
    led_set_target(&led, 500);
    printf("\n");
}


/*====================================================================
 * 示例4：PCIe DMA 回调策略 — Linux内核实战
 *====================================================================*/

typedef enum {
    DMA_STATUS_PENDING,
    DMA_STATUS_COMPLETE,
    DMA_STATUS_ERROR
} DmaStatus;

// 模拟PCIe设备结构体
typedef struct {
    const char *name;
    int irq_wake;
} PciDev;

// 模拟FIFO
typedef struct {
    int head;
    int tail;
} Fifo;

// DMA传输请求
typedef struct {
    void *src;
    void *dst;
    size_t len;
    DmaStatus status;
    void (*callback)(void *param, DmaStatus status);
    void *callback_param;
} DmaRequest;

void dma_submit(DmaRequest *req) {
    req->status = DMA_STATUS_PENDING;
    printf("  [DMA] 提交传输: %zu bytes\n", req->len);
}

void dma_complete(DmaRequest *req, DmaStatus status) {
    req->status = status;
    if (req->callback) {
        req->callback(req->callback_param, status);
    }
}

// 策略A：发中断
static void dma_callback_interrupt(void *param, DmaStatus status) {
    (void)status;
    PciDev *dev = (PciDev *)param;
    printf("  [策略A] DMA完成，发中断给%s\n", dev->name);
}

// 策略B：填充FIFO
static void dma_callback_fifo(void *param, DmaStatus status) {
    (void)param; (void)status;
    printf("  [策略B] DMA完成，数据填充FIFO\n");
}

// 策略C：日志
static void dma_callback_log(void *param, DmaStatus status) {
    (void)param;
    printf("  [策略C] DMA状态: %s\n",
           status == DMA_STATUS_COMPLETE ? "成功" : "失败");
}

void demo_pcie_dma(void) {
    printf("=== 示例4：PCIe DMA回调策略（Linux内核实战） ===\n");

    PciDev dev = {.name = "0000:01:00.0", .irq_wake = 0};
    Fifo fifo = {.head = 0, .tail = 0};
    (void)fifo;

    DmaRequest req = {
        .src = (void*)0x100000,
        .dst = (void*)0x200000,
        .len = 4096,
    };

    req.callback = dma_callback_interrupt;
    req.callback_param = &dev;
    dma_submit(&req);
    dma_complete(&req, DMA_STATUS_COMPLETE);

    printf("\n");
    req.callback = dma_callback_fifo;
    req.callback_param = &fifo;
    dma_submit(&req);
    dma_complete(&req, DMA_STATUS_COMPLETE);

    printf("\n");
    req.callback = dma_callback_log;
    req.callback_param = NULL;
    dma_submit(&req);
    dma_complete(&req, DMA_STATUS_ERROR);
    printf("\n");
}


/*====================================================================
 * 主函数
 *====================================================================*/

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║   策略模式（Strategy Pattern）— C语言实现               ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    demo_sort_strategy();
    demo_travel_strategy();
    demo_led_brightness();
    demo_pcie_dma();

    printf("=== 策略模式核心：把会变的行为封装成策略，上下文保持不变 ===\n");
    return 0;
}
