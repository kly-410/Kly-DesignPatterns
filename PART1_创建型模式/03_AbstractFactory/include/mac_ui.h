/**
 * macOS 风格 UI 组件
 * 
 * 具体产品：MacWindow、MacButton
 * 具体工厂：MacFactory
 */
#ifndef MAC_UI_H
#define MAC_UI_H

#include "ui_product.h"
#include "ui_factory.h"

/**
 * macOS 窗口 - 具体产品
 */
typedef struct {
    IWindow base;
    const char *title;
    int visible;
} MacWindow;

/**
 * macOS 按钮 - 具体产品
 */
typedef struct {
    IButton base;
    const char *text;
    int visible;
    void (*click_handler)(void *);
    void *click_arg;
} MacButton;

/**
 * 创建 macOS 窗口
 */
MacWindow* MacWindow_create(const char *title);

/**
 * 创建 macOS 按钮
 */
MacButton* MacButton_create(const char *text);

/**
 * macOS 工厂 - 具体工厂
 */
typedef struct {
    IUIFactory base;
    const char *name;
} MacFactory;

/**
 * 创建 macOS 工厂
 */
MacFactory* MacFactory_create(void);

#endif // MAC_UI_H