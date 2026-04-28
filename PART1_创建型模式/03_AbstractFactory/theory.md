# 抽象工厂模式（Abstract Factory Pattern）

## 本章目标

1. 理解抽象工厂解决的问题：**创建一系列相关产品**（产品族）而不只是单个产品
2. 理解"产品族"概念：相关产品的一组集合（例如 Linux 工厂同时生产 Widget + Button）
3. 掌握抽象工厂在 Linux ACPI 子系统中的应用
4. 掌握 C 语言实现抽象工厂的方式（工厂接口返回多个类型的产品）

---

## 1. 从一个问题开始

设计一个跨平台的 GUI 框架，需要同时创建多个相关组件：
- 窗口（Window）
- 按钮（Button）
- 输入框（Input）

在 Linux 上需要创建 Linux 风格组件，在 macOS 上需要创建 macOS 风格组件。

**工厂方法的局限**：一个工厂方法只能创建一种产品。如果要同时创建 Window + Button + Input，需要很多次调用，而且无法保证风格一致性。

**抽象工厂的思路**：把"产品族"作为整体来创建。一个工厂能创建**一整套**相关产品（Window + Button + Input），每个平台有不同的具体工厂，但都返回一致风格的组件。

---

## 2. 抽象工厂模式的定义

> **抽象工厂模式**：提供一个创建一系列相关或相互依赖对象的接口，而无需指定它们具体的类。

三个关键词：
- **产品族**：一组相关的产品（Window + Button + Input 是"GUI 组件族"）
- **一致性**：同一个工厂创建的产品相互兼容（Linux 工厂创建的都是 Linux 风格）
- **平台切换**：换一个工厂，就能换一整套产品

---

## 3. 工厂方法 vs 抽象工厂

```
工厂方法（Factory Method）：
  一个工厂 → 一个产品
  NVMeFactory.create() → NVMeDevice
  EthFactory.create()   → EthDevice

抽象工厂（Abstract Factory）：
  一个工厂 → 一个产品族（多个相关产品）
  LinuxFactory.createWindow() + createButton() → LinuxWindow + LinuxButton
  MacFactory.createWindow()  + createButton()  → MacWindow  + MacButton
```

| 对比 | 工厂方法 | 抽象工厂 |
|:---|:---|:---|
| 创建范围 | 单个产品 | 产品族（多个相关产品） |
| 新增产品 | 加新产品 → 加具体工厂 | 加新产品 → 修改抽象工厂 + 所有具体工厂 |
| 新增产品族 | 加新工厂 | 加新具体工厂 |
| 适用场景 | 产品线单一 | 产品线多维度相关 |

---

## 4. C 语言实现：跨平台 UI 组件工厂

### 4.1 定义抽象产品接口

```c
// ui_product.h
#ifndef UI_PRODUCT_H
#define UI_PRODUCT_H

#include <stdint.h>

/* 抽象产品1：窗口 */
typedef struct IWindow IWindow;
struct IWindow {
    void (*show)(IWindow *win);
    void (*hide)(IWindow *win);
    void (*set_title)(IWindow *win, const char *title);
    void (*destroy)(IWindow *win);
    void *private_data;
};

/* 抽象产品2：按钮 */
typedef struct IButton IButton;
struct IButton {
    void (*show)(IButton *btn);
    void (*hide)(IButton *btn);
    void (*set_text)(IButton *btn, const char *text);
    void (*on_click)(IButton *btn, void (*handler)(void *), void *arg);
    void (*destroy)(IButton *btn);
    void *private_data;
};

#endif // UI_PRODUCT_H
```

### 4.2 定义抽象工厂接口

```c
// ui_factory.h
#ifndef UI_FACTORY_H
#define UI_FACTORY_H

#include "ui_product.h"

/**
 * UI 组件抽象工厂
 * 
 * 可以创建一整套 UI 组件：
 * - 窗口（Window）
 * - 按钮（Button）
 * 
 * 具体工厂（如 LinuxFactory、MacFactory）负责创建对应平台风格的组件。
 */
typedef struct IUIFactory IUIFactory;
struct IUIFactory {
    /**
     * 创建窗口
     */
    IWindow* (*create_window)(IUIFactory *factory, const char *title);
    
    /**
     * 创建按钮
     */
    IButton* (*create_button)(IUIFactory *factory, const char *text);
    
    /**
     * 获取工厂名称
     */
    const char* (*get_name)(IUIFactory *factory);
    
    /**
     * 销毁工厂
     */
    void (*destroy)(IUIFactory *factory);
};

#endif // UI_FACTORY_H
```

### 4.3 具体产品：Linux 风格组件

```c
// linux_ui.h + linux_ui.c

#ifndef LINUX_UI_H
#define LINUX_UI_H

#include "ui_product.h"
#include "ui_factory.h"

/* Linux 窗口 */
typedef struct {
    IWindow base;
    const char *title;
    int visible;
} LinuxWindow;

/* Linux 按钮 */
typedef struct {
    IButton base;
    const char *text;
    int visible;
    void (*click_handler)(void *);
    void *click_arg;
} LinuxButton;

LinuxWindow* LinuxWindow_create(const char *title);
LinuxButton* LinuxButton_create(const char *text);

/* Linux 工厂 */
typedef struct {
    IUIFactory base;
    const char *name;
} LinuxFactory;
LinuxFactory* LinuxFactory_create(void);

#endif // LINUX_UI_H
```

```c
// linux_ui.c
#include "linux_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== LinuxWindow ========== */
static void linux_win_show(IWindow *win) {
    LinuxWindow *w = (LinuxWindow*)win->private_data;
    printf("[LinuxWindow] 显示窗口: %s\n", w->title);
    w->visible = 1;
}
static void linux_win_hide(IWindow *win) {
    LinuxWindow *w = (LinuxWindow*)win->private_data;
    printf("[LinuxWindow] 隐藏窗口: %s\n", w->title);
    w->visible = 0;
}
static void linux_win_set_title(IWindow *win, const char *title) {
    LinuxWindow *w = (LinuxWindow*)win->private_data;
    printf("[LinuxWindow] 设置标题: %s → %s\n", w->title, title);
    w->title = title;
}
static void linux_win_destroy(IWindow *win) {
    printf("[LinuxWindow] 销毁窗口\n");
    free(win->private_data);
}

LinuxWindow* LinuxWindow_create(const char *title) {
    LinuxWindow *w = malloc(sizeof(LinuxWindow));
    if (!w) return NULL;
    memset(w, 0, sizeof(LinuxWindow));
    w->base.show = linux_win_show;
    w->base.hide = linux_win_hide;
    w->base.set_title = linux_win_set_title;
    w->base.destroy = linux_win_destroy;
    w->base.private_data = w;
    w->title = title;
    w->visible = 0;
    printf("[LinuxWindow] 创建 Linux 风格窗口: %s\n", title);
    return w;
}

/* ========== LinuxButton ========== */
static void linux_btn_show(IButton *btn) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    printf("[LinuxButton] 显示按钮: [%s]\n", b->text);
    b->visible = 1;
}
static void linux_btn_hide(IButton *btn) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    printf("[LinuxButton] 隐藏按钮: [%s]\n", b->text);
    b->visible = 0;
}
static void linux_btn_set_text(IButton *btn, const char *text) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    printf("[LinuxButton] 设置文本: %s → %s\n", b->text, text);
    b->text = text;
}
static void linux_btn_on_click(IButton *btn, void (*handler)(void*), void *arg) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    b->click_handler = handler;
    b->click_arg = arg;
    printf("[LinuxButton] 注册点击处理器\n");
}
static void linux_btn_destroy(IButton *btn) {
    printf("[LinuxButton] 销毁按钮\n");
    free(btn->private_data);
}

LinuxButton* LinuxButton_create(const char *text) {
    LinuxButton *b = malloc(sizeof(LinuxButton));
    if (!b) return NULL;
    memset(b, 0, sizeof(LinuxButton));
    b->base.show = linux_btn_show;
    b->base.hide = linux_btn_hide;
    b->base.set_text = linux_btn_set_text;
    b->base.on_click = linux_btn_on_click;
    b->base.destroy = linux_btn_destroy;
    b->base.private_data = b;
    b->text = text;
    b->visible = 0;
    printf("[LinuxButton] 创建 Linux 风格按钮: [%s]\n", text);
    return b;
}

/* ========== LinuxFactory ========== */
static IWindow* linux_create_window(IUIFactory *f, const char *title) {
    (void)f;
    return &LinuxWindow_create(title)->base;
}
static IButton* linux_create_button(IUIFactory *f, const char *text) {
    (void)f;
    return &LinuxButton_create(text)->base;
}
static const char* linux_get_name(IUIFactory *f) {
    (void)f;
    return "Linux Factory";
}
static void linux_destroy(IUIFactory *f) {
    free(f);
}

LinuxFactory* LinuxFactory_create(void) {
    LinuxFactory *f = malloc(sizeof(LinuxFactory));
    if (!f) return NULL;
    f->base.create_window = linux_create_window;
    f->base.create_button = linux_create_button;
    f->base.get_name = linux_get_name;
    f->base.destroy = linux_destroy;
    f->name = "LinuxFactory";
    printf("[LinuxFactory] 创建 Linux 工厂\n");
    return f;
}
```

### 4.4 具体产品：Mac 风格组件

```c
// mac_ui.h + mac_ui.c（结构与 Linux 版本类似，只是风格不同）

#ifndef MAC_UI_H
#define MAC_UI_H

#include "ui_product.h"
#include "ui_factory.h"

typedef struct {
    IWindow base;
    const char *title;
    int visible;
} MacWindow;

typedef struct {
    IButton base;
    const char *text;
    int visible;
    void (*click_handler)(void *);
    void *click_arg;
} MacButton;

MacWindow* MacWindow_create(const char *title);
MacButton* MacButton_create(const char *text);

typedef struct {
    IUIFactory base;
    const char *name;
} MacFactory;
MacFactory* MacFactory_create(void);

#endif // MAC_UI_H
```

```c
// mac_ui.c
#include "mac_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MacWindow */
static void mac_win_show(IWindow *win) {
    MacWindow *w = (MacWindow*)win->private_data;
    printf("[MacWindow] ▶ 显示窗口: \"%s\"\n", w->title);
    w->visible = 1;
}
static void mac_win_hide(IWindow *win) {
    MacWindow *w = (MacWindow*)win->private_data;
    printf("[MacWindow] ◀ 隐藏窗口: \"%s\"\n", w->title);
    w->visible = 0;
}
static void mac_win_set_title(IWindow *win, const char *title) {
    MacWindow *w = (MacWindow*)win->private_data;
    printf("[MacWindow] 标题: \"%s\" → \"%s\"\n", w->title, title);
    w->title = title;
}
static void mac_win_destroy(IWindow *win) {
    printf("[MacWindow] 释放窗口资源\n");
    free(win->private_data);
}

MacWindow* MacWindow_create(const char *title) {
    MacWindow *w = malloc(sizeof(MacWindow));
    if (!w) return NULL;
    memset(w, 0, sizeof(MacWindow));
    w->base.show = mac_win_show;
    w->base.hide = mac_win_hide;
    w->base.set_title = mac_win_set_title;
    w->base.destroy = mac_win_destroy;
    w->base.private_data = w;
    w->title = title;
    w->visible = 0;
    printf("[MacWindow] ● 创建 macOS 风格窗口: \"%s\"\n", title);
    return w;
}

/* MacButton */
static void mac_btn_show(IButton *btn) {
    MacButton *b = (MacButton*)btn->private_data;
    printf("[MacButton]  ● 显示按钮: 「%s」\n", b->text);
    b->visible = 1;
}
static void mac_btn_hide(IButton *btn) {
    MacButton *b = (MacButton*)btn->private_data;
    printf("[MacButton]  ○ 隐藏按钮: 「%s」\n", b->text);
    b->visible = 0;
}
static void mac_btn_set_text(IButton *btn, const char *text) {
    MacButton *b = (MacButton*)btn->private_data;
    printf("[MacButton]  「%s」 → 「%s」\n", b->text, text);
    b->text = text;
}
static void mac_btn_on_click(IButton *btn, void (*handler)(void*), void *arg) {
    MacButton *b = (MacButton*)btn->private_data;
    b->click_handler = handler;
    b->click_arg = arg;
    printf("[MacButton]  注册回调处理程序\n");
}
static void mac_btn_destroy(IButton *btn) {
    printf("[MacButton]  释放按钮资源\n");
    free(btn->private_data);
}

MacButton* MacButton_create(const char *text) {
    MacButton *b = malloc(sizeof(MacButton));
    if (!b) return NULL;
    memset(b, 0, sizeof(MacButton));
    b->base.show = mac_btn_show;
    b->base.hide = mac_btn_hide;
    b->base.set_text = mac_btn_set_text;
    b->base.on_click = mac_btn_on_click;
    b->base.destroy = mac_btn_destroy;
    b->base.private_data = b;
    b->text = text;
    b->visible = 0;
    printf("[MacButton]  ● 创建 macOS 风格按钮: 「%s」\n", text);
    return b;
}

/* MacFactory */
static IWindow* mac_create_window(IUIFactory *f, const char *title) {
    (void)f;
    return &MacWindow_create(title)->base;
}
static IButton* mac_create_button(IUIFactory *f, const char *text) {
    (void)f;
    return &MacButton_create(text)->base;
}
static const char* mac_get_name(IUIFactory *f) {
    (void)f;
    return "Mac Factory";
}
static void mac_destroy(IUIFactory *f) {
    free(f);
}

MacFactory* MacFactory_create(void) {
    MacFactory *f = malloc(sizeof(MacFactory));
    if (!f) return NULL;
    f->base.create_window = mac_create_window;
    f->base.create_button = mac_create_button;
    f->base.get_name = mac_get_name;
    f->base.destroy = mac_destroy;
    f->name = "MacFactory";
    printf("[MacFactory] 创建 macOS 工厂\n");
    return f;
}
```

### 4.5 主程序

```c
// main.c
#include "ui_factory.h"
#include "linux_ui.h"
#include "mac_ui.h"
#include <stdio.h>

void click_handler(void *arg) {
    printf(">>> 按钮被点击! (arg=%s)\n", (char*)arg);
}

int main(void) {
    printf("========================================\n");
    printf("   抽象工厂模式 - 跨平台 UI 示例\n");
    printf("========================================\n\n");
    
    /* 使用 Linux 工厂创建一套 Linux 风格组件 */
    printf("=== 创建 Linux UI 组件族 ===\n");
    LinuxFactory *linux_factory = LinuxFactory_create();
    IUIFactory *linux_base = &linux_factory->base;
    
    IWindow *linux_win = linux_base->create_window(linux_base, "Linux 应用程序");
    IButton *linux_btn = linux_base->create_button(linux_base, "确定");
    
    printf("\n=== 使用 Linux 组件 ===\n");
    linux_win->show(linux_win);
    linux_btn->show(linux_btn);
    linux_btn->on_click(linux_btn, click_handler, "linux_handler");
    
    printf("\n");
    
    /* 切换到 Mac 工厂，创建一套 Mac 风格组件（不改变使用代码）*/
    printf("=== 创建 macOS UI 组件族 ===\n");
    MacFactory *mac_factory = MacFactory_create();
    IUIFactory *mac_base = &mac_factory->base;
    
    IWindow *mac_win = mac_base->create_window(mac_base, "Mac Application");
    IButton *mac_btn = mac_base->create_button(mac_base, "OK");
    
    printf("\n=== 使用 macOS 组件 ===\n");
    mac_win->show(mac_win);
    mac_btn->show(mac_btn);
    mac_btn->on_click(mac_btn, click_handler, "mac_handler");
    
    printf("\n========================================\n");
    printf("   总结：同一个接口，不同的工厂，\n");
    printf("         产生了风格一致的产品族 ✓\n");
    printf("========================================\n");
    
    /* 清理资源 */
    linux_win->destroy(linux_win);
    linux_btn->destroy(linux_btn);
    mac_win->destroy(mac_win);
    mac_btn->destroy(mac_btn);
    
    linux_base->destroy(linux_base);
    mac_base->destroy(mac_base);
    
    return 0;
}
```

---

## 5. Linux 内核里的抽象工厂：ACPI 子系统

ACPI（Advanced Configuration and Power Interface）子系统是抽象工厂的典型应用：

```c
// 简化自 drivers/acpi/bus.c

/**
 * ACPI 设备族
 * 
 * 工厂方法只能创建一种产品（NVMe 或网卡），
 * 抽象工厂能创建整个设备族：
 * - ACPI 设备（struct acpi_device）
 * - 电源管理资源（ACPI power resource）
 * - 设备电源状态（struct acpi_device_power_state）
 */

/* 抽象工厂：ACPI 扫描枚举器 */
struct acpi_scan_handler {
    const char *id;  // 设备类型 ID
    bool (*match)(const struct acpi_device *dev);
    
    /* 工厂方法：添加设备到系统 */
    int (*add)(struct acpi_device *dev);
    
    /* 工厂方法：移除设备 */
    void (*remove)(struct acpi_device *dev);
};

/* 具体工厂：PCIe 总线扫描处理程序 */
static struct acpi_scan_handler pci_scan_handler = {
    .id = "PCI",
    .match = acpi_pci_match,
    .add = acpi_pci_add,      // 添加 PCI 设备到系统
    .remove = acpi_pci_remove,
};

/* 具体工厂：I2C 总线扫描处理程序 */
static struct acpi_scan_handler i2c_scan_handler = {
    .id = "I2C",
    .match = acpi_i2c_match,
    .add = acpi_i2c_add,
    .remove = acpi_i2c_remove,
};
```

ACPI 扫描器对每个设备遍历所有已注册的 handler，每个 handler 能创建：
- 设备节点（struct acpi_device）
- 电源资源
- 中断资源
- GPIO 资源

这就是"一个工厂创建整个设备族"的概念。

---

## 6. 抽象工厂模式的适用场景

**应该用：**
- 需要创建一组相关的产品（产品族），且产品之间有约束关系
- 需要支持多个产品系列（Linux 系列 / Windows 系列）
- 客户不关心具体实现，只关心接口一致性

**不应该用：**
- 只创建单个产品（用工厂方法就够了）
- 产品之间没有关联，不需要一致性保证
- 需要经常添加新产品（维护成本高）

---

## 7. 抽象工厂的"开闭原则"代价

抽象工厂的问题：**增加新产品（如 Checkbox）需要修改抽象工厂接口 + 所有具体工厂**

这违反了开闭原则，但这是"产品族"维度扩展的必然代价。

解决方案：
- 在设计前充分考虑产品族的范围
- 如果产品族会频繁变化，考虑用"组件注册 + 反射"的方式

---

## 8. 本章小结

| 要素 | C 语言实现 |
|:---|:---|
| 抽象工厂 | 包含多个 create 方法的结构体（createWindow, createButton...） |
| 具体工厂 | 实现抽象工厂的多个 create 方法，返回对应风格的具体产品 |
| 产品族一致性 | 同一个工厂创建的所有产品风格一致 |
| 平台切换 | 换一个工厂，整个产品线一起换 |

**抽象工厂的核心**：把"创建产品族"作为整体来设计，保证产品之间的一致性。

---

## 9. 练习

1. 在上面的 UI 框架中添加 Checkbox 组件，实现 Linux 和 macOS 两种风格

2. 实现一个"嵌入式传感器套件"抽象工厂：温度传感器 + 湿度传感器 + 气压传感器，每个平台（STM32、ESP32）有不同的具体工厂

3. 读 Linux 内核 `drivers/acpi/bus.c`，找出 `acpi_scan_handler` 的定义，理解 ACPI 如何用抽象工厂创建设备族

4. （思考题）抽象工厂的"开闭原则"问题如何解决？有什么设计模式可以弥补？

---

## 代码

`include/ui_product.h` — UI 产品接口（Window、Button）
`include/ui_factory.h` — 抽象工厂接口
`include/linux_ui.h` + `src/linux_ui.c` — Linux 风格组件实现
`include/mac_ui.h` + `src/mac_ui.c` — macOS 风格组件实现
`src/main.c` — 测试主程序
`Makefile` — 编译脚本