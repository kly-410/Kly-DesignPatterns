# 05_Facade — 外观模式

## 问题引入：复杂性爆炸

启动一个嵌入式设备，需要按顺序初始化：
```
1. 时钟控制器 (Clock)
2. 电源管理 (PMIC)
3. DDR 内存初始化
4. NAND Flash 驱动
5. 显示驱动
6. 触摸屏驱动
7. 网络 PHY 初始化
8. WiFi 固件加载
9. 外设时钟门控
10. 安全引擎初始化
```

**问题**：
- 调用方要记住 10 个初始化顺序，太复杂
- 某天换了 Flash 芯片，调用方全要改
- 想要"一键启动"，不要 expose 细节

---

## 外观模式核心思想

> **为复杂的子系统提供一个统一的高层接口，让子系统更易用。**

外观（Facade）给客户端提供一个简单的入口，把细节封装在内部。

### 类图

```
┌──────────┐                    ┌─────────────────┐
│  Client  │───────────────────>│    Facade       │
│ (调用方)  │                    │  init_system()  │
└──────────┘                    │  start_network()│
                                └───────┬─────────┘
                                        │
       ┌───────────────────────────────┼───────────────┐
       │             子系统复杂细节       │               │
       ▼                ▼                ▼               ▼
   ┌────────┐      ┌────────┐       ┌────────┐     ┌────────┐
   │Subsys A│      │Subsys B│       │Subsys C│     │Subsys D│
   └────────┘      └────────┘       └────────┘     └────────┘
```

### 对比

| 无外观 | 有外观 |
|:---|:---|
| `clk_init(); pmic_init(); ddr_init(); nand_init(); display_init(); tp_init(); net_init(); wifi_load();` | `system_init();` |
| 调用方需要知道10个API | 调用方只知道1个API |
| 顺序错误 → 系统崩溃 | 顺序由外观内部保证 |

---

## 模式定义

**Facade Pattern**：为一组复杂的子系统接口提供一个统一的接口。外观定义了一个高层接口，让子系统更易使用。

---

## C语言实现：嵌入式固件初始化序列

### 场景：物联网关设备初始化

```c
// ============================================================
// 场景：物联网关设备，需要初始化多个复杂子系统
// 外观模式把10个初始化函数封装成3个简单接口
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------- 子系统A：时钟控制器 ---------
static int clock_inited = 0;
int clock_init(void)
{
    printf("[CLK] Initializing PLL and dividers...\n");
    printf("[CLK]   Set ARM: 1GHz, AXI: 400MHz, APB: 100MHz\n");
    clock_inited = 1;
    return 0;
}
int clock_enable(int peripheral_id)
{
    if (!clock_inited) return -1;
    printf("[CLK]   enable peripheral #%d clock\n", peripheral_id);
    return 0;
}

// --------- 子系统B：电源管理 ---------
static int pmic_inited = 0;
int pmic_init(void)
{
    printf("[PMIC] Initializing power management IC...\n");
    printf("[PMIC]   Set core voltage: 1.1V, IO voltage: 3.3V\n");
    printf("[PMIC]   Enable power rails: VDD_ARM, VDD_MEM, VDD_IO\n");
    pmic_inited = 1;
    return 0;
}
int pmic_set_voltage(int rail_id, int voltage_mv)
{
    if (!pmic_inited) return -1;
    printf("[PMIC]   Rail #%d -> %d mV\n", rail_id, voltage_mv);
    return 0;
}

// --------- 子系统C：DDR 内存 ---------
static int ddr_inited = 0;
int ddr_init(void)
{
    printf("[DDR] Initializing DDR3 controller...\n");
    printf("[DDR]   Training memory lanes (calibration)...\n");
    printf("[DDR]   Size: 512MB @ 1600MT/s\n");
    ddr_inited = 1;
    return 0;
}
void* ddr_alloc(size_t size)
{
    if (!ddr_inited) return NULL;
    return malloc(size);
}

// --------- 子系统D：显示驱动 ---------
static int display_inited = 0;
int display_init(void)
{
    printf("[DISP] Initializing MIPI DSI display...\n");
    printf("[DISP]   Resolution: 1920x1080 @ 60Hz\n");
    printf("[DISP]   Backlight: PWM dimming\n");
    display_inited = 1;
    return 0;
}

// --------- 子系统E：网络子系统 ---------
static int net_inited = 0;
int net_init(void)
{
    printf("[NET] Initializing network subsystem...\n");
    printf("[NET]   PHY: Gigabit Ethernet (RGMII)\n");
    printf("[NET]   WiFi: 802.11ac (SDIO)\n");
    net_inited = 1;
    return 0;
}
int net_send_packet(const char* data, size_t len)
{
    if (!net_inited) return -1;
    printf("[NET]   TX packet: %zu bytes\n", len);
    return 0;
}

// ============================================================
// 外观（Facade）：设备初始化管理器
// ============================================================
typedef struct _DeviceManager DeviceManager;
struct _DeviceManager {
    int initialized;
    
    // 高级接口：一个函数搞定所有初始化
    int (*boot)(DeviceManager*);
    int (*shutdown)(DeviceManager*);
    
    // 子系统状态（内部使用，不暴露给调用方）
    int subsystem_mask;
};
// 0x01=CLK, 0x02=PMIC, 0x04=DDR, 0x08=DISP, 0x10=NET

static int device_boot(DeviceManager* dm)
{
    printf("\n========== 设备启动序列 ==========\n");
    printf("[BOOT] Starting firmware initialization...\n\n");
    
    // 严格按顺序初始化（依赖关系）
    if (clock_init() != 0) { printf("[BOOT] FAIL: clock\n"); return -1; }
    dm->subsystem_mask |= 0x01;
    clock_enable(1); clock_enable(2); clock_enable(3);
    
    if (pmic_init() != 0) { printf("[BOOT] FAIL: pmic\n"); return -1; }
    dm->subsystem_mask |= 0x02;
    pmic_set_voltage(0, 1100);  // VDD_ARM
    pmic_set_voltage(1, 3300);  // VDD_IO
    
    if (ddr_init() != 0) { printf("[BOOT] FAIL: ddr\n"); return -1; }
    dm->subsystem_mask |= 0x04;
    
    if (display_init() != 0) { printf("[BOOT] FAIL: display\n"); return -1; }
    dm->subsystem_mask |= 0x08;
    clock_enable(10);  // 显示相关时钟
    
    if (net_init() != 0) { printf("[BOOT] FAIL: network\n"); return -1; }
    dm->subsystem_mask |= 0x10;
    clock_enable(20);  // 网卡相关时钟
    
    dm->initialized = 1;
    printf("\n[BOOT] ========== 启动完成 ==========\n");
    return 0;
}

static int device_shutdown(DeviceManager* dm)
{
    if (!dm->initialized) return 0;
    printf("\n[SHUTDOWN] ========== 关机序列 ==========\n");
    printf("[SHUTDOWN] Stopping network...\n");
    printf("[SHUTDOWN] Powering off display backlight...\n");
    printf("[SHUTDOWN] Releasing DDR...\n");
    printf("[SHUTDOWN] Disabling power rails...\n");
    dm->initialized = 0;
    dm->subsystem_mask = 0;
    printf("[SHUTDOWN] ========== 关机完成 ==========\n");
    return 0;
}

DeviceManager* new_DeviceManager(void)
{
    DeviceManager* dm = calloc(1, sizeof(DeviceManager));
    dm->boot     = device_boot;
    dm->shutdown = device_shutdown;
    return dm;
}

// ============================================================
int main()
{
    printf("========== 外观模式演示 ==========\n\n");

    // 调用方只需要和一个对象打交道
    DeviceManager* dev = new_DeviceManager();
    
    // 一键启动：内部按正确顺序初始化了所有子系统
    dev->boot(dev);
    
    // 此时可以开始使用设备了
    printf("\n[APP] Device ready, sending test packet...\n");
    net_send_packet("Hello IoT!", 9);
    
    // 一键关机：自动按正确顺序关闭
    dev->shutdown(dev);
    
    free(dev);
    return 0;
}
```

**输出：**
```
========== 外观模式演示 ==========

========== 设备启动序列 ==========
[BOOT] Starting firmware initialization...

[CLK] Initializing PLL and dividers...
[CLK]   Set ARM: 1GHz, AXI: 400MHz, APB: 100MHz
[CLK]   enable peripheral #1 clock
[CLK]   enable peripheral #2 clock
[CLK]   enable peripheral #3 clock
[PMIC] Initializing power management IC...
[PMIC]   Set core voltage: 1.1V, IO voltage: 3.3V
[PMIC]   Enable power rails: VDD_ARM, VDD_MEM, VDD_IO
[PMIC]   Rail #0 -> 1100 mV
[PMIC]   Rail #1 -> 3300 mV
[DDR] Initializing DDR3 controller...
[DDR]   Training memory lanes (calibration)...
[DDR]   Size: 512MB @ 1600MT/s
[DISP] Initializing MIPI DSI display...
[DISP]   Resolution: 1920x1080 @ 60Hz
[DISP]   Backlight: PWM dimming
[NET] Initializing network subsystem...
[NET]   PHY: Gigabit Ethernet (RGMII)
[NET]   WiFi: 802.11ac (SDIO)
[CLK]   enable peripheral #10 clock
[CLK]   enable peripheral #20 clock

[BOOT] ========== 启动完成 ==========

[APP] Device ready, sending test packet...
[NET]   TX packet: 9 bytes

[SHUTDOWN] ========== 关机序列 ==========
[SHUTDOWN] Stopping network...
[SHUTDOWN] Powering off display backlight...
[SHUTDOWN] Releasing DDR...
[SHUTDOWN] Disabling power rails...
[SHUTDOWN] ========== 关机完成 ==========
```

---

## Linux 内核实例：块设备请求队列

Linux 块设备层用外观模式封装了复杂的请求调度：

```c
// 复杂的底层：多种调度算法（CFQ、Deadline、MQ）
//              多种块设备（SSD、HDD、NVM）
//              复杂的硬件队列管理

// 提供给文件系统的统一接口：
 blk_mq_submit_request()
   ↓
 外观层隐藏了：
   - scheduler selection
   - request merging
   - block layer plug
   - hardware queue mapping
```

应用程序只需要 `write(fd, buf, len)`，背后由 VFS + block layer 这个外观处理一切。

---

## 外观模式 vs 适配器 vs 装饰器

| 模式 | 目的 | 侧重点 |
|:---|:---|:---|
| **外观** | 简化接口，封装复杂度 | **减少调用方的复杂度** |
| **适配器** | 接口转换，让不兼容变兼容 | **转换接口** |
| **装饰器** | 动态添加功能 | **增强功能** |

---

## 练习

1. **基础练习**：给 `DeviceManager` 添加 `is_ready()` 接口，检查所有子系统是否就绪。

2. **进阶练习**：实现子系统依赖检查——如果某个子系统启动失败，要能回滚已经启动的部分。

3. **内核练习**：阅读 Linux 内核 `block/blk-core.c`，找找 `blk_mq_submit_request()` 背后封装了哪些复杂度。

---

## 关键收获

- **外观模式不改变子系统接口**，只是把复杂调用封装成一个简单入口
- **调用方和子系统解耦**：子系统内部变化不影响调用方
- **外观可以只暴露部分功能**，隐藏不需要关心的接口
- **Linux 内核**：几乎所有子系统都有外观，`blk_mq_submit_request()`、`device_add()`、`netif_rx()` 都是例子
