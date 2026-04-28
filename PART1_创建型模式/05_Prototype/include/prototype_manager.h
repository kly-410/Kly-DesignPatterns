/**
 * 原型管理器
 * 
 * 管理一组原型对象，通过名称注册和克隆。
 * 类似注册表模式（Registry），但专门用于原型模式。
 */
#ifndef PROTOTYPE_MANAGER_H
#define PROTOTYPE_MANAGER_H

#include "config_prototype.h"

#define MAX_PROTOTYPES 16

/**
 * 原型管理器
 * 
 * 通过名称注册原型，然后通过名称克隆。
 * 适用于需要管理多个可复制模板的场景。
 */
typedef struct PrototypeManager PrototypeManager;

PrototypeManager* PrototypeManager_create(void);

/**
 * 注册原型
 * 
 * @param mgr 管理器
 * @param name 原型名称
 * @param proto 原型对象
 */
void PrototypeManager_register(PrototypeManager *mgr, 
                                const char *name, 
                                IPrototype *proto);

/**
 * 通过名称克隆原型（浅拷贝）
 * 
 * @param mgr 管理器
 * @param name 原型名称
 * @return 新创建的对象（调用者负责销毁）
 */
void* PrototypeManager_clone(PrototypeManager *mgr, const char *name);

/**
 * 通过名称深拷贝克隆原型
 * 
 * @param mgr 管理器
 * @param name 原型名称
 * @return 新创建的对象（调用者负责销毁）
 */
void* PrototypeManager_deep_clone(PrototypeManager *mgr, const char *name);

/**
 * 销毁管理器
 */
void PrototypeManager_destroy(PrototypeManager *mgr);

#endif // PROTOTYPE_MANAGER_H