/**
 * ===========================================================================
 * Template Method Pattern — 模板方法模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Caffeine Beverage
 * ============================================================================*/

typedef struct _CaffeineBeverage {
    void (*prepare)(struct _CaffeineBeverage*);
    void (*brew)(struct _CaffeineBeverage*);
    void (*addCondiments)(struct _CaffeineBeverage*);
    void (*boilWater)(struct _CaffeineBeverage*);
    void (*pourInCup)(struct _CaffeineBeverage*);
} CaffeineBeverage;

static void CaffeineBeverage_boilWater(CaffeineBeverage* bev) {
    printf("  [Template] Boiling water...\n");
}

static void CaffeineBeverage_pourInCup(CaffeineBeverage* bev) {
    printf("  [Template] Pouring into cup...\n");
}

static void CaffeineBeverage_prepare(CaffeineBeverage* bev) {
    bev->boilWater(bev);
    bev->brew(bev);
    bev->pourInCup(bev);
    bev->addCondiments(bev);
}

typedef struct _Coffee {
    CaffeineBeverage base;
} Coffee;

static void Coffee_brew(CaffeineBeverage* bev) {
    printf("  [Coffee] Dripping coffee through filter...\n");
}

static void Coffee_addCondiments(CaffeineBeverage* bev) {
    printf("  [Coffee] Adding sugar and milk...\n");
}

static Coffee* Coffee_new(void) {
    Coffee* c = calloc(1, sizeof(Coffee));
    c->base.prepare = CaffeineBeverage_prepare;
    c->base.brew = Coffee_brew;
    c->base.addCondiments = Coffee_addCondiments;
    c->base.boilWater = CaffeineBeverage_boilWater;
    c->base.pourInCup = CaffeineBeverage_pourInCup;
    return c;
}

typedef struct _Tea {
    CaffeineBeverage base;
} Tea;

static void Tea_brew(CaffeineBeverage* bev) {
    printf("  [Tea] Steeping the tea...\n");
}

static void Tea_addCondiments(CaffeineBeverage* bev) {
    printf("  [Tea] Adding lemon...\n");
}

static Tea* Tea_new(void) {
    Tea* t = calloc(1, sizeof(Tea));
    t->base.prepare = CaffeineBeverage_prepare;
    t->base.brew = Tea_brew;
    t->base.addCondiments = Tea_addCondiments;
    t->base.boilWater = CaffeineBeverage_boilWater;
    t->base.pourInCup = CaffeineBeverage_pourInCup;
    return t;
}

/* ============================================================================
 * 第二部分：PCIe Driver Probe Template
 * ============================================================================*/

typedef enum { PROBE_OK = 0, PROBE_ERR_ENABLE = -1 } ProbeResult;

typedef struct _PCIeDevice {
    uint32_t bar0;
    uint32_t bar1;
    int irq;
} PCIeDevice;

static ProbeResult PCIeDriver_probe_template(void) {
    printf("  [Probe] Step 1: pci_enable_device()\n");
    printf("  [Probe] Step 2: pci_request_regions() BAR0=0xFEBC0000\n");
    printf("  [Probe] Step 3: pci_ioremap(BAR0)\n");
    printf("  [Probe] Step 4: request_irq(IRQ 17)\n");
    printf("  [Probe] Step 5: device-specific init\n");
    return PROBE_OK;
}

/* ============================================================================
 * 第三部分：Firmware Init Sequence
 * ============================================================================*/

typedef int (*InitStep)(void);

static int init_clock(void) { printf("  [Firmware] Clock subsystem initialized\n"); return 0; }
static int init_ddr(void) { printf("  [Firmware] DDR memory initialized\n"); return 0; }
static int init_pcie(void) { printf("  [Firmware] PCIe subsystem initialized\n"); return 0; }
static int init_network(void) { printf("  [Firmware] Network subsystem initialized\n"); return 0; }

static InitStep init_steps[] = { init_clock, init_ddr, init_pcie, init_network };
static const char* init_names[] = { "Clock Init", "DDR Init", "PCIe Init", "Network Init" };

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                  Template Method Pattern — 模板方法模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Caffeine Beverage */
    printf("【示例1】Caffeine Beverage — 咖啡因饮料冲泡\n");
    printf("-----------------------------------------------------------------\n");
    printf("\n  Making Coffee:\n");
    Coffee* coffee = Coffee_new();
    coffee->base.prepare(&coffee->base);
    printf("\n  Making Tea:\n");
    Tea* tea = Tea_new();
    tea->base.prepare(&tea->base);
    free(coffee); free(tea);

    /* 示例2：PCIe Driver Probe */
    printf("\n【示例2】Linux PCIe Driver Probe — PCIe 驱动探测模板\n");
    printf("-----------------------------------------------------------------\n");
    PCIeDriver_probe_template();

    /* 示例3：Firmware Init Sequence */
    printf("\n【示例3】Firmware Init Sequence — 固件初始化序列\n");
    printf("-----------------------------------------------------------------\n");
    int num_steps = sizeof(init_steps) / sizeof(init_steps[0]);
    printf("  [Firmware] Starting boot sequence...\n");
    for (int i = 0; i < num_steps; i++) {
        printf("  [Firmware] Step %d: %s\n", i + 1, init_names[i]);
        init_steps[i]();
    }
    printf("  [Firmware] Boot sequence completed!\n");

    printf("\n=================================================================\n");
    printf(" Template Method 模式核心：算法骨架 + 抽象步骤函数指针\n");
    printf("=================================================================\n");
    return 0;
}
