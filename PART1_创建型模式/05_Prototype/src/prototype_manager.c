/**
 * 原型管理器实现
 */

#include "prototype_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================
 * 内部结构
 * ========================================== */

typedef struct {
    const char *name;
    IPrototype *proto;
} PrototypeEntry;

struct PrototypeManager {
    PrototypeEntry entries[MAX_PROTOTYPES];
    int count;
};

/* ==========================================
 * 方法实现
 * ========================================== */

PrototypeManager* PrototypeManager_create(void) {
    PrototypeManager *mgr = malloc(sizeof(PrototypeManager));
    if (!mgr) {
        fprintf(stderr, "[PrototypeManager] 错误: 分配内存失败\n");
        return NULL;
    }
    mgr->count = 0;
    printf("[PrototypeManager] 创建原型管理器\n");
    return mgr;
}

void PrototypeManager_register(PrototypeManager *mgr, 
                                const char *name, 
                                IPrototype *proto) {
    if (mgr->count >= MAX_PROTOTYPES) {
        fprintf(stderr, "[PrototypeManager] 错误: 已满，无法注册更多原型 (最多 %d)\n", 
                MAX_PROTOTYPES);
        return;
    }
    
    mgr->entries[mgr->count].name = name;
    mgr->entries[mgr->count].proto = proto;
    mgr->count++;
    
    printf("[PrototypeManager] 注册原型: %s\n", name);
}

void* PrototypeManager_clone(PrototypeManager *mgr, const char *name) {
    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->entries[i].name, name) == 0) {
            printf("[PrototypeManager] 浅拷贝克隆原型: %s\n", name);
            return mgr->entries[i].proto->clone(mgr->entries[i].proto);
        }
    }
    fprintf(stderr, "[PrototypeManager] 错误: 未找到原型 \"%s\"\n", name);
    return NULL;
}

void* PrototypeManager_deep_clone(PrototypeManager *mgr, const char *name) {
    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->entries[i].name, name) == 0) {
            printf("[PrototypeManager] 深拷贝克隆原型: %s\n", name);
            return mgr->entries[i].proto->deep_clone(mgr->entries[i].proto);
        }
    }
    fprintf(stderr, "[PrototypeManager] 错误: 未找到原型 \"%s\"\n", name);
    return NULL;
}

void PrototypeManager_destroy(PrototypeManager *mgr) {
    printf("[PrototypeManager] 销毁管理器 (管理 %d 个原型)\n", mgr->count);
    free(mgr);
}