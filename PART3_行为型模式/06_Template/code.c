/**
 * ===========================================================================
 * 模板方法模式 — Template Method Pattern
 * ===========================================================================
 *
 * 核心思想：在一个方法里定义算法的骨架，把某些步骤延迟到子类实现
 *
 * 本代码演示（嵌入式视角）：
 * 1. 固件启动流程：硬件检查 → 内存初始化 → 外设初始化 → 加载应用
 * 2. 测试框架：setup() → run() → teardown()，流程固定，细节不同
 * 3. PCIe 设备初始化：电源 → BAR → 中断 → DMA，顺序固定
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：测试框架（最经典的模板方法例子）
 *
 * 嵌入式固件测试的典型结构：
 *   每个测试用例都走：setup → run → teardown
 *   但具体做什么由各测试自己决定
 * ============================================================================*/

/** 测试用例基类：定义模板方法和三个步骤接口 */
typedef struct _TestCase {
    /** 模板方法：固定执行顺序，定义为函数指针让子类可替换 */
    void (*execute)(struct _TestCase*);

    /** 三个步骤：抽象方法，子类必须实现 */
    void (*setup)(struct _TestCase*);      /**< 测试前准备：初始化硬件/模拟环境 */
    void (*run)(struct _TestCase*);         /**< 测试执行：实际运行测试逻辑 */
    void (*teardown)(struct _TestCase*);    /**< 测试后清理：释放资源、恢复状态 */

    /** 共享数据区 */
    const char* name;                       /**< 测试名称 */
    int passed;                             /**< 是否通过 */
    char log[256];                          /**< 测试日志 */
} TestCase;

/**
 * 模板方法实现：固定流程
 * setup() → run() → teardown()
 *
 * 这就是"模板"：算法骨架是 TestCase 的方法，具体步骤是子类实现的方法
 */
static void TestCase_execute(TestCase* tc) {
    printf("  [%s] START\n", tc->name);
    if (tc->setup) tc->setup(tc);
    if (tc->run) tc->run(tc);
    if (tc->teardown) tc->teardown(tc);
    printf("  [%s] RESULT: %s\n\n", tc->name, tc->passed ? "✅ PASS" : "❌ FAIL");
}

/* ============================================================================
 * 第二部分：具体测试用例：PCIe BAR 配置测试
 * ============================================================================*/

typedef struct {
    TestCase base;     /**< 继承 TestCase，必须是第一个成员 */
    uint32_t bar_addr; /**< 测试的 BAR 地址 */
    uint32_t test_pattern;  /**< 写入的测试数据 */
    uint32_t read_back;      /**< 读回的值 */
} PCIeBARTest;

static void PCIeBARTest_setup(TestCase* base) {
    PCIeBARTest* t = (PCIeBARTest*)base;
    printf("  [Setup] 模拟 ioremap BAR @ 0x%08X\n", t->bar_addr);
    printf("  [Setup] 使能 PCIe 内存空间\n");
    t->read_back = 0;  /**< 初始值：表示还没读 */
}

static void PCIeBARTest_run(TestCase* base) {
    PCIeBARTest* t = (PCIeBARTest*)base;
    printf("  [Run] 写入测试数据: 0x%08X\n", t->test_pattern);
    printf("  [Run] 读取数据...\n");
    /**< 模拟：假设读回来和写入一致 */
    t->read_back = t->test_pattern;
    printf("  [Run] 读回: 0x%08X\n", t->read_back);
}

static void PCIeBARTest_teardown(TestCase* base) {
    PCIeBARTest* t = (PCIeBARTest*)base;
    printf("  [Teardown] 验证写入/读回一致性\n");
    t->base.passed = (t->read_back == t->test_pattern);
    printf("  [Teardown] 禁用 BAR 内存空间\n");
}

static PCIeBARTest* PCIeBARTest_new(const char* name, uint32_t bar_addr, uint32_t pattern) {
    PCIeBARTest* t = calloc(1, sizeof(PCIeBARTest));
    t->base.name = name;
    t->base.execute = TestCase_execute;
    t->base.setup = PCIeBARTest_setup;
    t->base.run = PCIeBARTest_run;
    t->base.teardown = PCIeBARTest_teardown;
    t->bar_addr = bar_addr;
    t->test_pattern = pattern;
    return t;
}

/* ============================================================================
 * 第三部分：具体测试用例：DMA 传输测试
 * ============================================================================*/

typedef struct {
    TestCase base;
    uint32_t src_addr;
    uint32_t dst_addr;
    int size;
    int success;
} DMATransferTest;

static void DMATransferTest_setup(TestCase* base) {
    DMATransferTest* t = (DMATransferTest*)base;
    printf("  [Setup] 初始化 DMA 控制器\n");
    printf("  [Setup] 源地址: 0x%08X, 目标地址: 0x%08X, 大小: %d bytes\n",
           t->src_addr, t->dst_addr, t->size);
}

static void DMATransferTest_run(TestCase* base) {
    DMATransferTest* t = (DMATransferTest*)base;
    printf("  [Run] 启动 DMA 传输...\n");
    printf("  [Run] 等待传输完成（模拟轮询）\n");
    /**< 模拟：90% 概率成功 */
    t->success = 1;  /**< 假设成功 */
    printf("  [Run] DMA 传输完成，状态: %s\n", t->success ? "OK" : "ERROR");
}

static void DMATransferTest_teardown(TestCase* base) {
    DMATransferTest* t = (DMATransferTest*)base;
    t->base.passed = (t->success == 1);
    printf("  [Teardown] 清理 DMA 通道\n");
}

static DMATransferTest* DMATransferTest_new(const char* name,
        uint32_t src, uint32_t dst, int size) {
    DMATransferTest* t = calloc(1, sizeof(DMATransferTest));
    t->base.name = name;
    t->base.execute = TestCase_execute;
    t->base.setup = DMATransferTest_setup;
    t->base.run = DMATransferTest_run;
    t->base.teardown = DMATransferTest_teardown;
    t->src_addr = src;
    t->dst_addr = dst;
    t->size = size;
    return t;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("               Template Method Pattern — 模板方法模式\n");
    printf("=================================================================\n\n");

    printf("【示例】嵌入式固件测试框架\n");
    printf("-----------------------------------------------------------------\n");
    printf("执行顺序：setup() → run() → teardown()（流程固定，内容各异）\n\n");

    /**< 创建并执行测试用例 */
    PCIeBARTest* test1 = PCIeBARTest_new("PCIe BAR0 读写测试", 0xFE400000, 0xDEADBEEF);
    test1->base.execute((TestCase*)test1);

    DMATransferTest* test2 = DMATransferTest_new("DDR→PCIe DMA 传输测试",
            0x20000000, 0xFE400010, 1024);
    test2->base.execute((TestCase*)test2);

    free(test1);
    free(test2);

    printf("=================================================================\n");
    printf(" 模板方法核心：算法骨架在基类，具体步骤在子类\n");
    printf(" 嵌入式场景：固件启动流程、测试框架、外设初始化序列\n");
    printf("=================================================================\n");
    return 0;
}
