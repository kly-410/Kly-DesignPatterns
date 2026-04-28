# 02_Bridge — 桥接模式

## 问题引入：维度爆炸

假设你做一个嵌入式 GUI 库，要支持：
- **3 种屏幕**：OLED、LCD、TFT
- **3 种控制芯片**：STM32、NXP、ESP32

用传统继承：
```
                    ┌──────────┐
                    │ Display  │
                    └────┬─────┘
            ┌───────────┼───────────┐
        ┌───┴───┐   ┌───┴───┐   ┌───┴───┐
        │ OLED  │   │  LCD  │   │  TFT  │      ← 3 种屏幕
       /┴──┬──┴\ /┴──┬──┴\ /┴──┬──┴\
      /    │    X    │    X    │    \
   STM32  NXP  ESP32 STM32 NXP  ESP32  ...
```

**9 个类**！每加一种屏幕或芯片，指数级增长。这就是"维度爆炸"。

---

## 桥接模式核心思想

> **拆解成两个独立维度，用组合代替继承。**

把"抽象"和"实现"分离，各自独立变化，中间用"桥"连接。

### 维度拆解

| 维度 | 描述 | 变化频率 |
|:---|:---|:---|
| **抽象（Abstraction）** | GUI 控件、绘制逻辑 | 较快 |
| **实现（Implementation）** | 底层屏幕驱动、SPI/I2C 传输 | 较慢 |

### 类图

```
┌──────────────────┐       ┌─────────────────────┐
│   Abstraction    │──────>│ Implementation      │
│  (高层抽象类)     │ «桥»  │ (底层实现接口)        │
└────────┬─────────┘       └──────────┬──────────┘
         │                             │
         │ 精细化抽象                    │ 具体实现
         ▼                             ▼
┌──────────────────┐       ┌─────────────────────┐
│ RefinedAbstraction│     │ ConcreteImplA       │
│                  │─────>│                     │
└──────────────────┘       └─────────────────────┘
```

---

## 模式定义

**Bridge Pattern**：将抽象部分与实现部分分离，使它们可以独立变化。

**核心原则**：优先使用对象组合，而不是类继承。

---

## C语言实现：平台无关的驱动模型

### 场景：嵌入式显示驱动双平台抽象

```c
// ============================================================
// 场景：嵌入式系统，需要同时支持 TFT 和 OLED 屏幕，
//       运行在 STM32 和 ESP32 两个平台
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------- 实现维度：底层像素传输接口 ---------
typedef struct _PixelTransfer PixelTransfer;
struct _PixelTransfer {
    void (*sendPixels)(PixelTransfer*, const void* pixels, size_t len);
    void (*setWindow)(PixelTransfer*, int x, int y, int w, int h);
};

// 每个具体平台实现一套传输方式
static void stm32_send_pixels(PixelTransfer* t, const void* px, size_t len)
{
    printf("  [STM32 SPI] send %zu bytes via DMA\n", len);
}

static void stm32_set_window(PixelTransfer* t, int x, int y, int w, int h)
{
    printf("  [STM32 SPI] set window (%d,%d) %dx%d\n", x, y, w, h);
}

static PixelTransfer* new_STM32PixelTransfer(void)
{
    PixelTransfer* t = calloc(1, sizeof(PixelTransfer));
    t->sendPixels = stm32_send_pixels;
    t->setWindow  = stm32_set_window;
    return t;
}

static void esp32_send_pixels(PixelTransfer* t, const void* px, size_t len)
{
    printf("  [ESP32 SPI] send %zu bytes via RMT\n", len);
}

static void esp32_set_window(PixelTransfer* t, int x, int y, int w, int h)
{
    printf("  [ESP32 SPI] set window (%d,%d) %dx%d\n", x, y, w, h);
}

static PixelTransfer* new_ESP32PixelTransfer(void)
{
    PixelTransfer* t = calloc(1, sizeof(PixelTransfer));
    t->sendPixels = esp32_send_pixels;
    t->setWindow  = esp32_set_window;
    return t;
}

// --------- 抽象维度：GUI 图形设备 ---------
typedef struct _GraphicsDevice GraphicsDevice;
struct _GraphicsDevice {
    PixelTransfer* transfer;     // 桥：持有实现接口
    int width;
    int height;
    
    // 抽象操作（与具体传输方式无关）
    void (*drawRect)(GraphicsDevice*, int x, int y, int w, int h, int color);
    void (*drawLine)(GraphicsDevice*, int x1, int y1, int x2, int y2, int color);
    void (*fillScreen)(GraphicsDevice*, int color);
};

// 抽象实现：把逻辑和具体传输解耦
static void gd_drawRect(GraphicsDevice* gd, int x, int y, int w, int h, int color)
{
    gd->transfer->setWindow(gd->transfer, x, y, w, h);
    // 模拟像素数据
    char dummy[16] = {0};
    gd->transfer->sendPixels(gd->transfer, dummy, w * h * 2);
    printf("  [Graphics] drew rect %dx%d at (%d,%d) color=0x%X\n",
           w, h, x, y, color);
}

static void gd_drawLine(GraphicsDevice* gd, int x1, int y1, int x2, int y2, int color)
{
    // Bresenham 算法画线...这里简化
    printf("  [Graphics] drew line (%d,%d)->(%d,%d)\n", x1, y1, x2, y2);
}

static void gd_fillScreen(GraphicsDevice* gd, int color)
{
    gd->transfer->setWindow(gd->transfer, 0, 0, gd->width, gd->height);
    printf("  [Graphics] fill screen with 0x%X\n", color);
}

GraphicsDevice* new_GraphicsDevice(PixelTransfer* t, int w, int h)
{
    GraphicsDevice* gd = calloc(1, sizeof(GraphicsDevice));
    gd->transfer = t;
    gd->width = w;
    gd->height = h;
    gd->drawRect   = gd_drawRect;
    gd->drawLine   = gd_drawLine;
    gd->fillScreen = gd_fillScreen;
    return gd;
}

// ============================================================
int main()
{
    printf("========== 桥接模式演示 ==========\n\n");

    // 组合1：TFT + STM32
    printf("--- 组合1: STM32 + TFT ---\n");
    PixelTransfer* stm32 = new_STM32PixelTransfer();
    GraphicsDevice* tft_stm32 = new_GraphicsDevice(stm32, 320, 240);
    tft_stm32->fillScreen(tft_stm32, 0xFFFF);
    tft_stm32->drawRect(tft_stm32, 10, 10, 50, 30, 0xF800);

    // 组合2：OLED + ESP32
    printf("\n--- 组合2: ESP32 + OLED ---\n");
    PixelTransfer* esp32 = new_ESP32PixelTransfer();
    GraphicsDevice* oled_esp32 = new_GraphicsDevice(esp32, 128, 64);
    oled_esp32->fillScreen(oled_esp32, 0x0000);
    oled_esp32->drawRect(oled_esp32, 20, 10, 40, 20, 0xFF);

    // 组合3：TFT + ESP32（加一个新组合完全不用改代码）
    printf("\n--- 组合3: ESP32 + TFT ---\n");
    GraphicsDevice* tft_esp32 = new_GraphicsDevice(esp32, 320, 240);
    tft_esp32->fillScreen(tft_esp32, 0x001F);
    tft_esp32->drawLine(tft_esp32, 0, 0, 100, 50, 0xFFFF);

    free(stm32); free(esp32);
    free(tft_stm32); free(oled_esp32); free(tft_esp32);
    return 0;
}
```

**输出：**
```
========== 桥接模式演示 ==========

--- 组合1: STM32 + TFT ---
  [STM32 SPI] set window (0,0) 320x240
  [Graphics] fill screen with 0xFFFF
  [STM32 SPI] set window (10,10) 50x30
  [Graphics] drew rect 50x30 at (10,10) color=0xF800

--- 组合2: ESP32 + OLED ---
  [ESP32 SPI] set window (0,0) 128x64
  [Graphics] fill screen with 0x0000
  [ESP32 SPI] set window (20,10) 40x20
  [Graphics] drew rect 40x20 at (20,10) color=0xFF

--- 组合3: ESP32 + TFT ---
  [ESP32 SPI] set window (0,0) 320x240
  [Graphics] fill screen with 0x001F
  [Graphics] drew line (0,0)->(100,50)
```

---

## Linux 内核实例：DMA 引擎抽象

Linux DMA 引擎（`drivers/dma/）`完美展示了桥接模式：

```c
// 实现维度：不同厂商的 DMA 控制器
//      st-dma.c (STMicro)
//      mv-dma.c (Marvell)
//      fsl-dma.c (Freescale)

// 抽象维度：通用 DMA API (dmaengine.h)
//      dmaengine_prep_*
//      dmaengine_submit
//      dma_async_issue_pending

// 桥接层：
//      struct dma_device — 抽象接口
//      每家 DMA 驱动实现自己的 .device_prep_* 函数
//      注册到统一的 DMA 引擎框架
```

```c
// 嵌入式驱动程序员只需要：
struct dma_async_tx_descriptor* desc;
desc = dmaengine_prep_slave_sg(chan, sg, 3, DMA_DEV_TO_MEM, 0);
// 完全不用管是 STM32 DMA 还是 Freescale eDMA
```

---

## 关键类比

| 概念 | 继承方案 | 桥接方案 |
|:---|:---|:---|
| 3 屏幕 + 3 平台 | 9 个类 | 3 + 3 = 6 个类 |
| 加新屏幕 | +3 个类 | +1 个类 |
| 加新平台 | +3 个类 | +1 个类 |
| **本质区别** | 组合爆炸 | **维度正交分解** |

---

## 练习

1. **基础练习**：为"声音输出"实现桥接模式，`AudioOutput`（抽象）× `PulseAudio`/`ALSA`（实现）。

2. **进阶练习**：在桥接的基础上，用**装饰器模式**给 `GraphicsDevice` 添加缓存层。

3. **内核练习**：阅读 Linux 内核 `drivers/dma/dw/core.c`，分析 DMA 引擎如何用桥接统一不同厂商的 DMA 控制器。

---

## 关键收获

- **桥接解决的是"多维度变化"导致的类爆炸**
- **核心**：分离抽象和实现，两者之间用组合（桥）连接
- **对比**：继承是"is-a"，组合是"has-a"，桥接是"通过...实现"
- **Linux 内核**：几乎所有子系统（DMA、I2C、SPI、Framebuffer）都用桥接模式
