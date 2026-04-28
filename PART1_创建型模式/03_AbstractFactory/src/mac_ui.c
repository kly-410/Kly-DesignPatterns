/**
 * macOS 风格 UI 组件实现
 * 
 * 展示 macOS 风格的 UI 组件实现（Aqua 风格）。
 */

#include "mac_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================
 * MacWindow 实现
 * ========================================== */

static void mac_win_show(IWindow *win) {
    MacWindow *w = (MacWindow*)win->private_data;
    w->visible = 1;
    printf("[MacWindow] ▶ 显示窗口: \"%s\"\n", w->title);
}

static void mac_win_hide(IWindow *win) {
    MacWindow *w = (MacWindow*)win->private_data;
    w->visible = 0;
    printf("[MacWindow] ◀ 隐藏窗口: \"%s\"\n", w->title);
}

static void mac_win_set_title(IWindow *win, const char *title) {
    MacWindow *w = (MacWindow*)win->private_data;
    printf("[MacWindow] 标题变更: \"%s\" → \"%s\"\n", w->title, title);
    w->title = title;
}

static void mac_win_destroy(IWindow *win) {
    printf("[MacWindow] 释放窗口资源\n");
    free(win->private_data);
}

MacWindow* MacWindow_create(const char *title) {
    MacWindow *w = malloc(sizeof(MacWindow));
    if (!w) {
        fprintf(stderr, "[MacWindow] 分配内存失败\n");
        return NULL;
    }
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

/* ==========================================
 * MacButton 实现
 * ========================================== */

static void mac_btn_show(IButton *btn) {
    MacButton *b = (MacButton*)btn->private_data;
    b->visible = 1;
    printf("[MacButton]  ● 显示按钮: 「%s」\n", b->text);
}

static void mac_btn_hide(IButton *btn) {
    MacButton *b = (MacButton*)btn->private_data;
    b->visible = 0;
    printf("[MacButton]  ○ 隐藏按钮: 「%s」\n", b->text);
}

static void mac_btn_set_text(IButton *btn, const char *text) {
    MacButton *b = (MacButton*)btn->private_data;
    printf("[MacButton]  文本变更: 「%s」 → 「%s」\n", b->text, text);
    b->text = text;
}

static void mac_btn_on_click(IButton *btn, void (*handler)(void*), void *arg) {
    MacButton *b = (MacButton*)btn->private_data;
    b->click_handler = handler;
    b->click_arg = arg;
    printf("[MacButton]  回调处理程序已注册\n");
}

static void mac_btn_destroy(IButton *btn) {
    printf("[MacButton]  释放按钮资源\n");
    free(btn->private_data);
}

MacButton* MacButton_create(const char *text) {
    MacButton *b = malloc(sizeof(MacButton));
    if (!b) {
        fprintf(stderr, "[MacButton] 分配内存失败\n");
        return NULL;
    }
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

/* ==========================================
 * MacFactory 实现
 * ========================================== */

static IWindow* mac_create_window(IUIFactory *f, const char *title) {
    (void)f;
    printf("[MacFactory] 创建 macOS 窗口: %s\n", title);
    return &MacWindow_create(title)->base;
}

static IButton* mac_create_button(IUIFactory *f, const char *text) {
    (void)f;
    printf("[MacFactory] 创建 macOS 按钮: [%s]\n", text);
    return &MacButton_create(text)->base;
}

static const char* mac_get_name(IUIFactory *f) {
    (void)f;
    return "macOS UI Factory (Aqua 风格)";
}

static void mac_destroy(IUIFactory *f) {
    printf("[MacFactory] 销毁工厂实例\n");
    free(f);
}

MacFactory* MacFactory_create(void) {
    MacFactory *f = malloc(sizeof(MacFactory));
    if (!f) {
        fprintf(stderr, "[MacFactory] 分配内存失败\n");
        return NULL;
    }
    
    f->base.create_window = mac_create_window;
    f->base.create_button = mac_create_button;
    f->base.get_name = mac_get_name;
    f->base.destroy = mac_destroy;
    f->name = "MacFactory";
    
    printf("[MacFactory] 创建 macOS UI 工厂 (Aqua 风格)\n");
    return f;
}