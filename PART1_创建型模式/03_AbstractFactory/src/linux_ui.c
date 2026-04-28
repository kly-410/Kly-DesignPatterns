/**
 * Linux 风格 UI 组件实现
 * 
 * 展示 Linux 风格的 UI 组件实现（GTK+ 风格）。
 */

#include "linux_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================
 * LinuxWindow 实现
 * ========================================== */

static void linux_win_show(IWindow *win) {
    LinuxWindow *w = (LinuxWindow*)win->private_data;
    w->visible = 1;
    printf("[LinuxWindow] █ 显示窗口: %s\n", w->title);
}

static void linux_win_hide(IWindow *win) {
    LinuxWindow *w = (LinuxWindow*)win->private_data;
    w->visible = 0;
    printf("[LinuxWindow] ▪ 隐藏窗口: %s\n", w->title);
}

static void linux_win_set_title(IWindow *win, const char *title) {
    LinuxWindow *w = (LinuxWindow*)win->private_data;
    printf("[LinuxWindow] 设置标题: \"%s\" → \"%s\"\n", w->title, title);
    w->title = title;
}

static void linux_win_destroy(IWindow *win) {
    printf("[LinuxWindow] 销毁窗口实例\n");
    free(win->private_data);
}

LinuxWindow* LinuxWindow_create(const char *title) {
    LinuxWindow *w = malloc(sizeof(LinuxWindow));
    if (!w) {
        fprintf(stderr, "[LinuxWindow] 分配内存失败\n");
        return NULL;
    }
    memset(w, 0, sizeof(LinuxWindow));
    
    /* 设置接口（模拟继承） */
    w->base.show = linux_win_show;
    w->base.hide = linux_win_hide;
    w->base.set_title = linux_win_set_title;
    w->base.destroy = linux_win_destroy;
    w->base.private_data = w;  /* 多态关键 */
    
    /* 初始化子类数据 */
    w->title = title;
    w->visible = 0;
    
    printf("[LinuxWindow] 创建 Linux 风格窗口: %s\n", title);
    return w;
}

/* ==========================================
 * LinuxButton 实现
 * ========================================== */

static void linux_btn_show(IButton *btn) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    b->visible = 1;
    printf("[LinuxButton]  [ %s ] 显示按钮\n", b->text);
}

static void linux_btn_hide(IButton *btn) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    b->visible = 0;
    printf("[LinuxButton]  [ %s ] 隐藏按钮\n", b->text);
}

static void linux_btn_set_text(IButton *btn, const char *text) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    printf("[LinuxButton]  [ %s ] → [ %s ]\n", b->text, text);
    b->text = text;
}

static void linux_btn_on_click(IButton *btn, void (*handler)(void*), void *arg) {
    LinuxButton *b = (LinuxButton*)btn->private_data;
    b->click_handler = handler;
    b->click_arg = arg;
    printf("[LinuxButton]  点击处理器已注册\n");
}

static void linux_btn_destroy(IButton *btn) {
    printf("[LinuxButton]  销毁按钮实例\n");
    free(btn->private_data);
}

LinuxButton* LinuxButton_create(const char *text) {
    LinuxButton *b = malloc(sizeof(LinuxButton));
    if (!b) {
        fprintf(stderr, "[LinuxButton] 分配内存失败\n");
        return NULL;
    }
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

/* ==========================================
 * LinuxFactory 实现
 * ========================================== */

static IWindow* linux_create_window(IUIFactory *f, const char *title) {
    (void)f;
    printf("[LinuxFactory] 创建 Linux 窗口: %s\n", title);
    return &LinuxWindow_create(title)->base;
}

static IButton* linux_create_button(IUIFactory *f, const char *text) {
    (void)f;
    printf("[LinuxFactory] 创建 Linux 按钮: [%s]\n", text);
    return &LinuxButton_create(text)->base;
}

static const char* linux_get_name(IUIFactory *f) {
    (void)f;
    return "Linux UI Factory (GTK+ 风格)";
}

static void linux_destroy(IUIFactory *f) {
    printf("[LinuxFactory] 销毁工厂实例\n");
    free(f);
}

LinuxFactory* LinuxFactory_create(void) {
    LinuxFactory *f = malloc(sizeof(LinuxFactory));
    if (!f) {
        fprintf(stderr, "[LinuxFactory] 分配内存失败\n");
        return NULL;
    }
    
    f->base.create_window = linux_create_window;
    f->base.create_button = linux_create_button;
    f->base.get_name = linux_get_name;
    f->base.destroy = linux_destroy;
    f->name = "LinuxFactory";
    
    printf("[LinuxFactory] 创建 Linux UI 工厂 (GTK+ 风格)\n");
    return f;
}