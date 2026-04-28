/*
 * 05_Facade - 外观模式
 * 
 * 外观模式核心：为一组复杂的子系统提供统一的高层接口
 * 
 * 本代码演示：
 * 1. 嵌入式固件初始化序列
 * 2. 子系统间有严格依赖关系（时钟 -> 电源 -> DDR -> 外设）
 * 3. 外观封装复杂性，提供简单的启动/关机接口
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// 子系统A：时钟控制器
// ============================================================
static int clock_initialized = 0;

void clock_init(void)
{
    printf("  [CLK] Initializing PLL...\n");
    printf("  [CLK]   ARM:  1GHz\n");
    printf("  [CLK]   AXI:  400MHz\n");
    printf("  [CLK]   APB:  100MHz\n");
    clock_initialized = 1;
}

void clock_enable(int peripheral_id)
{
    if (!clock_initialized) {
        printf("  [CLK] ERROR: clock not initialized!\n");
        return;
    }
    printf("  [CLK] Enable peripheral #%d clock\n", peripheral_id);
}

void clock_disable(int peripheral_id)
{
    printf("  [CLK] Disable peripheral #%d clock\n", peripheral_id);
}

// ============================================================
// 子系统B：电源管理（PMIC）
// ============================================================
static int pmic_initialized = 0;

void pmic_init(void)
{
    printf("  [PMIC] Initializing power management IC...\n");
    printf("  [PMIC]   Core voltage: 1.1V\n");
    printf("  [PMIC]   IO voltage:   3.3V\n");
    pmic_initialized = 1;
}

void pmic_set_voltage(int rail_id, int voltage_mv)
{
    if (!pmic_initialized) {
        printf("  [PMIC] ERROR: PMIC not initialized!\n");
        return;
    }
    printf("  [PMIC]   Rail #%d: %dmV\n", rail_id, voltage_mv);
}

void pmic_shutdown(void)
{
    printf("  [PMIC] Shutdown all power rails\n");
    pmic_initialized = 0;
}

// ============================================================
// 子系统C：DDR 内存
// ============================================================
static int ddr_initialized = 0;
static size_t ddr_capacity = 0;

void ddr_init(void)
{
    printf("  [DDR] Initializing DDR3 controller...\n");
    printf("  [DDR]   Training memory lanes...\n");
    printf("  [DDR]   Capacity: 512MB @ 1600MT/s\n");
    ddr_capacity = 512 * 1024 * 1024;
    ddr_initialized = 1;
}

void* ddr_alloc(size_t size)
{
    if (!ddr_initialized) {
        printf("  [DDR] ERROR: DDR not initialized!\n");
        return NULL;
    }
    return malloc(size);
}

void ddr_free(void* ptr)
{
    free(ptr);
}

void ddr_shutdown(void)
{
    printf("  [DDR] Release DDR controller\n");
    ddr_initialized = 0;
}

// ============================================================
// 子系统D：显示驱动
// ============================================================
static int display_initialized = 0;

void display_init(void)
{
    printf("  [DISP] Initializing MIPI DSI display...\n");
    printf("  [DISP]   Resolution: 1920x1080 @ 60Hz\n");
    display_initialized = 1;
}

void display_set_brightness(int level)
{
    if (!display_initialized) {
        printf("  [DISP] ERROR: Display not initialized!\n");
        return;
    }
    printf("  [DISP]   Backlight: %d%%\n", level);
}

void display_shutdown(void)
{
    printf("  [DISP] Power off display\n");
    display_initialized = 0;
}

// ============================================================
// 子系统E：网络子系统
// ============================================================
static int net_initialized = 0;

void net_init(void)
{
    printf("  [NET] Initializing network subsystem...\n");
    printf("  [NET]   PHY: Gigabit Ethernet (RGMII)\n");
    printf("  [NET]   WiFi: 802.11ac (SDIO)\n");
    net_initialized = 1;
}

void net_send(const char* data, size_t len)
{
    if (!net_initialized) {
        printf("  [NET] ERROR: Network not initialized!\n");
        return;
    }
    printf("  [NET]   TX: %zu bytes\n", len);
}

void net_shutdown(void)
{
    printf("  [NET] Stop network subsystem\n");
    net_initialized = 0;
}

// ============================================================
// Facade：设备初始化管理器
// ============================================================
typedef struct _DeviceManager DeviceManager;
struct _DeviceManager {
    int initialized;
    int subsystem_mask;
    int (*boot)(DeviceManager*);
    int (*shutdown)(DeviceManager*);
};

#define MASK_CLK   0x01
#define MASK_PMIC  0x02
#define MASK_DDR   0x04
#define MASK_DISP  0x08
#define MASK_NET   0x10

static int device_boot(DeviceManager* dm)
{
    printf("\n========================================\n");
    printf("          设备启动序列\n");
    printf("========================================\n");
    
    clock_init();
    dm->subsystem_mask |= MASK_CLK;
    
    pmic_init();
    dm->subsystem_mask |= MASK_PMIC;
    
    ddr_init();
    dm->subsystem_mask |= MASK_DDR;
    
    clock_enable(10);
    display_init();
    dm->subsystem_mask |= MASK_DISP;
    
    clock_enable(20);
    pmic_set_voltage(2, 3300);
    net_init();
    dm->subsystem_mask |= MASK_NET;
    
    dm->initialized = 1;
    
    printf("\n========================================\n");
    printf("          启动完成！\n");
    printf("========================================\n\n");
    return 0;
}

static int device_shutdown(DeviceManager* dm)
{
    if (!dm->initialized) return 0;
    
    printf("\n========================================\n");
    printf("          设备关机序列\n");
    printf("========================================\n");
    
    net_shutdown();
    clock_disable(20);
    
    display_shutdown();
    clock_disable(10);
    
    ddr_shutdown();
    
    pmic_shutdown();
    clock_initialized = 0;
    
    dm->initialized = 0;
    dm->subsystem_mask = 0;
    
    printf("\n========================================\n");
    printf("          关机完成\n");
    printf("========================================\n");
    return 0;
}

DeviceManager* new_DeviceManager(void)
{
    DeviceManager* dm = calloc(1, sizeof(DeviceManager));
    dm->initialized = 0;
    dm->subsystem_mask = 0;
    dm->boot = device_boot;
    dm->shutdown = device_shutdown;
    return dm;
}

int device_is_ready(DeviceManager* dm)
{
    return dm->initialized;
}

// ============================================================
int main()
{
    printf("========== 外观模式演示 ==========\n\n");
    printf("场景：物联网网关设备启动\n");
    printf("10个复杂的初始化步骤 → 1个 device_boot() 调用\n\n");

    DeviceManager* dev = new_DeviceManager();
    
    dev->boot(dev);
    
    printf("设备就绪: %s\n", device_is_ready(dev) ? "YES" : "NO");
    
    printf("\n[应用程序] 设备已就绪，开始工作...\n\n");
    
    char test_data[] = "Hello IoT Gateway!";
    net_send(test_data, strlen(test_data));
    display_set_brightness(80);
    
    void* buf = ddr_alloc(1024);
    if (buf) {
        printf("\n[应用程序] DDR allocation: OK (1024 bytes)\n");
        ddr_free(buf);
    }
    
    dev->shutdown(dev);
    
    free(dev);
    
    printf("\n========== 对比：外观 vs 无外观 ==========\n");
    printf("无外观：app 需要调用 10 个初始化函数，还需记住顺序\n");
    printf("有外观：app 只需调用 device_boot() / device_shutdown()\n");
    
    return 0;
}
