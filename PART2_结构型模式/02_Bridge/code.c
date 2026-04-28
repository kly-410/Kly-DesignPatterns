/*
 * 02_Bridge - 桥接模式
 * 
 * 桥接模式核心：将抽象部分与实现部分分离，使它们可以独立变化
 * 
 * 本代码演示：
 * 1. 嵌入式显示驱动的双平台抽象
 * 2. 抽象维度（GraphicsDevice）× 实现维度（PixelTransfer）
 * 3. 组合代替继承，避免类爆炸
 * 
 * 维度拆解：GUI 控件（抽象）× 底层驱动（实现）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// 实现维度：底层像素传输接口（桥的另一端）
// ============================================================
typedef struct _PixelTransfer PixelTransfer;
struct _PixelTransfer {
    const char* name;
    void (*sendPixels)(PixelTransfer*, const void* pixels, size_t len);
    void (*setWindow)(PixelTransfer*, int x, int y, int w, int h);
    void (*flush)(PixelTransfer*);
    void (*close)(PixelTransfer*);
};

// STM32 SPI 传输实现
static void stm32_send_pixels(PixelTransfer* t, const void* px, size_t len)
{
    printf("    [STM32 SPI] DMA tx: %zu bytes\n", len);
}

static void stm32_set_window(PixelTransfer* t, int x, int y, int w, int h)
{
    printf("    [STM32 SPI] setWindow(%d,%d %dx%d)\n", x, y, w, h);
}

static void stm32_flush(PixelTransfer* t)
{
    printf("    [STM32 SPI] flush\n");
}

static void stm32_close(PixelTransfer* t)
{
    printf("    [STM32 SPI] closed\n");
    free(t);
}

PixelTransfer* new_STM32PixelTransfer(void)
{
    PixelTransfer* t = calloc(1, sizeof(PixelTransfer));
    t->name = "STM32 SPI";
    t->sendPixels = stm32_send_pixels;
    t->setWindow  = stm32_set_window;
    t->flush      = stm32_flush;
    t->close      = stm32_close;
    return t;
}

// ESP32 SPI 传输实现
static void esp32_send_pixels(PixelTransfer* t, const void* px, size_t len)
{
    printf("    [ESP32 SPI] RMT tx: %zu bytes\n", len);
}

static void esp32_set_window(PixelTransfer* t, int x, int y, int w, int h)
{
    printf("    [ESP32 SPI] setWindow(%d,%d %dx%d)\n", x, y, w, h);
}

static void esp32_flush(PixelTransfer* t)
{
    printf("    [ESP32 SPI] flush\n");
}

static void esp32_close(PixelTransfer* t)
{
    printf("    [ESP32 SPI] closed\n");
    free(t);
}

PixelTransfer* new_ESP32PixelTransfer(void)
{
    PixelTransfer* t = calloc(1, sizeof(PixelTransfer));
    t->name = "ESP32 SPI";
    t->sendPixels = esp32_send_pixels;
    t->setWindow  = esp32_set_window;
    t->flush      = esp32_flush;
    t->close      = esp32_close;
    return t;
}

// ============================================================
// 抽象维度：GUI 图形设备（桥的这一端）
// 不依赖具体传输实现，只依赖 PixelTransfer 接口
// ============================================================
typedef struct _GraphicsDevice GraphicsDevice;
struct _GraphicsDevice {
    const char* name;
    PixelTransfer* transfer;  // 桥：用组合连接实现
    int width;
    int height;
    
    // 抽象操作（与具体传输方式完全解耦）
    void (*drawRect)(GraphicsDevice*, int x, int y, int w, int h, int color);
    void (*drawLine)(GraphicsDevice*, int x1, int y1, int x2, int y2, int color);
    void (*fillScreen)(GraphicsDevice*, int color);
    void (*close)(GraphicsDevice*);
};

// 内部实现：画矩形
static void gd_drawRect(GraphicsDevice* gd, int x, int y, int w, int h, int color)
{
    printf("  [GraphicsDevice %s] drawRect(%d,%d %dx%d) color=0x%X\n",
           gd->name, x, y, w, h, color);
    
    // 设置窗口后发送像素
    gd->transfer->setWindow(gd->transfer, x, y, w, h);
    char dummy[64] = {0};
    gd->transfer->sendPixels(gd->transfer, dummy, w * h * 2);
    gd->transfer->flush(gd->transfer);
}

// 内部实现：画线（Bresenham简化版）
static void gd_drawLine(GraphicsDevice* gd, int x1, int y1, int x2, int y2, int color)
{
    printf("  [GraphicsDevice %s] drawLine(%d,%d)->(%d,%d) color=0x%X\n",
           gd->name, x1, y1, x2, y2, color);
    // 简化：这里不实际画像素，只是打印信息
}

// 内部实现：填充全屏
static void gd_fillScreen(GraphicsDevice* gd, int color)
{
    printf("  [GraphicsDevice %s] fillScreen(0x%X)\n", gd->name, color);
    gd->transfer->setWindow(gd->transfer, 0, 0, gd->width, gd->height);
    gd->transfer->flush(gd->transfer);
}

static void gd_close(GraphicsDevice* gd)
{
    printf("  [GraphicsDevice %s] closed\n", gd->name);
    gd->transfer->close(gd->transfer);
    free(gd);
}

// 构造函数：把抽象和实现连接起来（架桥）
GraphicsDevice* new_GraphicsDevice(PixelTransfer* t, int w, int h, const char* name)
{
    GraphicsDevice* gd = calloc(1, sizeof(GraphicsDevice));
    gd->name = name;
    gd->transfer = t;
    gd->width = w;
    gd->height = h;
    gd->drawRect   = gd_drawRect;
    gd->drawLine   = gd_drawLine;
    gd->fillScreen = gd_fillScreen;
    gd->close      = gd_close;
    return gd;
}

// ============================================================
// 场景演示：OLED + STM32, TFT + ESP32 等多种组合
// ============================================================
void demo_screen(const char* screen_type, GraphicsDevice* gd)
{
    printf("\n=== %s屏幕演示 ===\n", screen_type);
    gd->fillScreen(gd, 0x0000);       // 黑色背景
    gd->drawRect(gd, 10, 10, 50, 20, 0xFFFF);  // 白色方块
    gd->drawLine(gd, 0, 0, 100, 50, 0xF800);   // 红色线
}

int main()
{
    printf("========== 桥接模式演示 ==========\n\n");
    printf("目标：用组合代替继承，避免屏幕×平台组合爆炸\n");
    printf("方案：分离抽象(GraphicsDevice)和实现(PixelTransfer)\n\n");

    // 组合1：TFT + STM32（最常用）
    printf("--- 组合1: STM32 + TFT ---\n");
    PixelTransfer* stm32 = new_STM32PixelTransfer();
    GraphicsDevice* tft_stm32 = new_GraphicsDevice(stm32, 320, 240, "TFT");
    demo_screen("TFT", tft_stm32);
    tft_stm32->close(tft_stm32);
    
    // 组合2：OLED + ESP32（IoT常用）
    printf("\n--- 组合2: ESP32 + OLED ---\n");
    PixelTransfer* esp32 = new_ESP32PixelTransfer();
    GraphicsDevice* oled_esp32 = new_GraphicsDevice(esp32, 128, 64, "OLED");
    demo_screen("OLED", oled_esp32);
    oled_esp32->close(oled_esp32);
    
    // 组合3：TFT + ESP32（加一个组合完全不用改代码）
    printf("\n--- 组合3: ESP32 + TFT ---\n");
    GraphicsDevice* tft_esp32 = new_GraphicsDevice(esp32, 320, 240, "TFT");
    demo_screen("TFT", tft_esp32);
    tft_esp32->close(tft_esp32);
    
    // 组合4：再加一个组合
    printf("\n--- 组合4: STM32 + OLED ---\n");
    PixelTransfer* stm32_b = new_STM32PixelTransfer();
    GraphicsDevice* oled_stm32 = new_GraphicsDevice(stm32_b, 128, 64, "OLED");
    demo_screen("OLED", oled_stm32);
    oled_stm32->close(oled_stm32);
    
    printf("\n========== 对比：继承 vs 桥接 ==========\n");
    printf("继承方案：2屏幕 × 2平台 = 4个子类\n");
    printf("桥接方案：2 + 2 = 4个类\n");
    printf("加新屏幕：继承+1类，桥接+1类（相同）\n");
    printf("加新平台：继承+2类(每个屏幕一个)，桥接+1类\n");
    printf("关键：桥接让两个维度正交分解，各自独立扩展\n");
    
    return 0;
}
