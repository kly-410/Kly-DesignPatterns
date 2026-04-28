/**
 * 原型接口定义
 * 
 * 所有支持克隆操作的对象都需要实现这个接口。
 * 原型模式让对象自己知道如何复制自己。
 */
#ifndef CONFIG_PROTOTYPE_H
#define CONFIG_PROTOTYPE_H

/**
 * 原型接口
 * 
 * 定义了克隆操作的标准接口。
 */
typedef struct IPrototype IPrototype;
struct IPrototype {
    /**
     * 浅拷贝克隆
     * 
     * 创建对象的浅拷贝副本。
     * 值类型字段复制值，指针类型字段复制地址（共享内存）。
     * 
     * @return 新对象的指针（调用者负责销毁）
     */
    void* (*clone)(IPrototype *proto);
    
    /**
     * 深拷贝克隆
     * 
     * 创建对象的深拷贝副本。
     * 所有字段（包括指针指向的内容）都会被复制，新旧对象完全独立。
     * 
     * @return 新对象的指针（调用者负责销毁）
     */
    void* (*deep_clone)(IPrototype *proto);
    
    /**
     * 获取对象名称
     */
    const char* (*get_name)(IPrototype *proto);
    
    /**
     * 销毁对象
     */
    void (*destroy)(IPrototype *proto);
};

#endif // CONFIG_PROTOTYPE_H