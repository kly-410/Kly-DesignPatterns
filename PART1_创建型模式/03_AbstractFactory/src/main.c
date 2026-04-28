/**
 * 抽象工厂模式测试程序
 * 
 * 演示如何使用抽象工厂创建一整套跨平台的 UI 组件。
 * 
 * 关键点：
 * 1. 一个工厂创建一组相关产品（产品族）
 * 2. 切换工厂时，整个产品线一起切换（风格一致性）
 * 3. 客户代码只依赖抽象工厂和抽象产品接口
 */

#include "ui_factory.h"
#include "linux_ui.h"
#include "mac_ui.h"
#include <stdio.h>

/**
 * 按钮点击处理程序
 */
void on_button_click(void *arg) {
    printf("    >>> 按钮点击回调触发! (arg=%s)\n", (char*)arg);
}

/**
 * 使用工厂创建一整套 UI（产品族）
 */
void create_ui_components(IUIFactory *factory, const char *window_title, 
                          const char *button_text) {
    printf("----------------------------------------\n");
    
    /* 通过抽象工厂接口创建产品（不知道具体是什么平台）*/
    IWindow *win = factory->create_window(factory, window_title);
    IButton *btn = factory->create_button(factory, button_text);
    
    printf("\n  显示窗口和按钮:\n");
    win->show(win);
    btn->show(btn);
    
    printf("\n  注册按钮回调:\n");
    btn->on_click(btn, on_button_click, "来自主程序的数据");
    
    /* 清理资源 */
    printf("\n  销毁组件:\n");
    btn->destroy(btn);
    win->destroy(win);
}

int main(void) {
    printf("========================================\n");
    printf("   抽象工厂模式 - 跨平台 UI 组件族\n");
    printf("========================================\n");
    
    /* ========================================
     * 使用 Linux 工厂创建 Linux 风格产品族
     * ======================================== */
    printf("\n### 步骤1：使用 Linux 工厂\n");
    LinuxFactory *linux_factory = LinuxFactory_create();
    IUIFactory *linux_base = &linux_factory->base;
    
    create_ui_components(linux_base, "Linux 应用程序", "确定");
    
    /* ========================================
     * 使用 macOS 工厂创建 macOS 风格产品族
     * ======================================== */
    printf("\n### 步骤2：使用 macOS 工厂\n");
    MacFactory *mac_factory = MacFactory_create();
    IUIFactory *mac_base = &mac_factory->base;
    
    create_ui_components(mac_base, "Mac Application", "OK");
    
    /* ========================================
     * 清理工厂
     * ======================================== */
    printf("\n### 步骤3：清理工厂\n");
    linux_base->destroy(linux_base);
    mac_base->destroy(mac_base);
    
    printf("\n========================================\n");
    printf("   总结：同一个接口 + 不同工厂 =\n");
    printf("         不同风格的产品族 ✓\n");
    printf("========================================\n");
    
    return 0;
}