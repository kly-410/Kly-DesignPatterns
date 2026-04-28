/**
 * Linux 风格 UI 组件
 * 
 * 具体产品：LinuxWindow、LinuxButton
 * 具体工厂：LinuxFactory
 */
#ifndef LINUX_UI_H
#define LINUX_UI_H

#include "ui_product.h"
#include "ui_factory.h"

/**
 * Linux 窗口 - 具体产品
 */
typedef struct {
    IWindow base;           // 继承 IWindow 接口
    const char *title;      // 窗口标题
    int visible;           // 是否可见
} LinuxWindow;

/**
 * Linux 按钮 - 具体产品
 */
typedef struct {
    IButton base;
    const char *text;      // 按钮文本
    int visible;
    void (*click_handler)(void *);
    void *click_arg;
} LinuxButton;

/**
 * 创建 Linux 窗口
 */
LinuxWindow* LinuxWindow_create(const char *title);

/**
 * 创建 Linux 按钮
 */
LinuxButton* LinuxButton_create(const char *text);

/**
 * Linux 工厂 - 具体工厂
 */
typedef struct {
    IUIFactory base;        // 继承 IUIFactory 接口
    const char *name;
} LinuxFactory;

/**
 * 创建 Linux 工厂
 */
LinuxFactory* LinuxFactory_create(void);

#endif // LINUX_UI_H