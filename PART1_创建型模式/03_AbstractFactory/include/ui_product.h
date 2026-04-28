/**
 * UI 组件抽象产品接口
 * 
 * 定义所有 UI 组件的通用接口（抽象产品角色）。
 */
#ifndef UI_PRODUCT_H
#define UI_PRODUCT_H

#include <stdint.h>

/**
 * 抽象产品1：窗口（IWindow）
 */
typedef struct IWindow IWindow;
struct IWindow {
    void (*show)(IWindow *win);
    void (*hide)(IWindow *win);
    void (*set_title)(IWindow *win, const char *title);
    void (*destroy)(IWindow *win);
    void *private_data;  // 指向具体产品实例
};

/**
 * 抽象产品2：按钮（IButton）
 */
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