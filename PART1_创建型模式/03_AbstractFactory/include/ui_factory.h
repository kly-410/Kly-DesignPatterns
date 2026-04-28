/**
 * UI 组件抽象工厂接口
 * 
 * 定义创建一整套 UI 组件的工厂方法（抽象工厂角色）。
 * 
 * 产品族概念：
 * - LinuxFactory 创建的：LinuxWindow + LinuxButton（Linux 风格产品族）
 * - MacFactory 创建的：MacWindow + MacButton（macOS 风格产品族）
 */
#ifndef UI_FACTORY_H
#define UI_FACTORY_H

#include "ui_product.h"

/* 前向声明 */
typedef struct IUIFactory IUIFactory;

/**
 * 抽象工厂接口
 * 
 * 负责创建整个产品族（Window + Button）。
 * 具体的工厂（LinuxFactory、MacFactory）实现这些方法，
 * 返回对应平台风格的具体产品。
 */
struct IUIFactory {
    /**
     * 创建窗口
     * @param title 窗口标题
     * @return IWindow* 抽象窗口接口
     */
    IWindow* (*create_window)(IUIFactory *factory, const char *title);
    
    /**
     * 创建按钮
     * @param text 按钮文本
     * @return IButton* 抽象按钮接口
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