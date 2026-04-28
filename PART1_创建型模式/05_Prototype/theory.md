# 原型模式（Prototype Pattern）

## 本章目标

1. 理解原型模式解决的问题：**通过复制已有对象创建新对象**，而不需要知道具体类型
2. 掌握深拷贝 vs 浅拷贝的区别与实现
3. 掌握 C 语言实现原型模式（IClone 接口 + 结构体复制）
4. 理解固件配置块克隆中的应用
5. 理解原型模式在嵌入式场景的特殊性（内存受限、无继承体系）

---

## 1. 从一个问题开始

设计一个 PCIe 网卡的"配置模板"功能：
- 需要保存当前网卡的配置（MAC 地址、VLAN、QoS 策略等）
- 需要快速基于模板创建新的配置实例
- 新实例与模板相互独立（修改模板不影响实例，修改实例不影响模板）

**糟糕的设计**：
```c
// 每次都重新创建，复制大量字段
Config* create_from_template(Config *template) {
    Config *new = malloc(sizeof(Config));
    new->mac = template->mac;
    new->vlan = template->vlan;
    // ... 20 个字段的复制
    return new;
}
```
问题：字段多了容易遗漏、结构体改了要改函数、没有统一的克隆接口。

**原型模式的思路**：把"克隆"作为对象的原生能力，每个对象知道怎么复制自己。

---

## 2. 原型模式的定义

> **原型模式**：用原型实例指定创建对象的种类，并且通过拷贝这些原型创建新的对象。

三个关键词：
- **原型实例**：一个已经存在的对象，作为"母板"
- **拷贝**：从原型实例复制出新的对象（克隆）
- **种类**：被克隆的对象类型由原型自己决定，不需要工厂知道类型

---

## 3. 原型模式的核心：克隆

克隆的本质是**创建已有对象的副本**，有两种方式：

### 3.1 浅拷贝（Shallow Copy）
只复制对象的**值类型字段**，指针字段只复制地址，两个对象共享同一个指针指向的内存。

```c
typedef struct {
    int type;           // 值类型 - 复制值
    char name[32];      // 值类型 - 复制值
    void *buffer;       // 指针类型 - 只复制地址（共享）
} Config;

// 浅拷贝：buffer 指针被复制，但指向同一块内存
Config shallow_copy(Config *orig) {
    Config new;
    memcpy(&new, orig, sizeof(Config));  // 整体复制
    // new.buffer == orig->buffer (同一个地址)
    return new;
}
```

### 3.2 深拷贝（Deep Copy）
复制对象的所有字段，包括指针指向的内容，创建完全独立的副本。

```c
// 深拷贝：buffer 内容也被复制
Config deep_copy(Config *orig) {
    Config new;
    memcpy(&new, orig, sizeof(Config));
    
    // 额外复制 buffer 内容
    if (orig->buffer) {
        new.buffer = malloc(256);
        memcpy(new.buffer, orig->buffer, 256);
    }
    return new;
}
```

### 3.3 何时用深拷贝、浅拷贝？

| 场景 | 方式 | 原因 |
|:---|:---|:---|
| 配置模板克隆 | 深拷贝 | 新旧实例不能互相影响 |
| 只读数据共享 | 浅拷贝 | 节省内存，不需要副本 |
| 有嵌套指针结构 | 深拷贝 | 不然会产生悬空指针 |
| 简单结构体（无指针）| 浅拷贝 | `memcpy` 即可，深浅无区别 |

---

## 4. C 语言实现：配置块原型克隆

### 4.1 定义原型接口

```c
// config_prototype.h
#ifndef CONFIG_PROTOTYPE_H
#define CONFIG_PROTOTYPE_H

#include <stdint.h>

/**
 * 原型接口
 * 
 * 定义克隆方法，所有支持克隆的对象都要实现这个接口。
 */
typedef struct IPrototype IPrototype;
struct IPrototype {
    /**
     * 克隆（浅拷贝）
     * @return 新对象的指针
     */
    void* (*clone)(IPrototype *proto);
    
    /**
     * 深拷贝克隆
     * @return 新对象的指针（完全独立）
     */
    void* (*deep_clone)(IPrototype *proto);
    
    /**
     * 获取对象名称
     */
    const char* (*get_name)(IPrototype *proto);
    
    /**
     * 销毁原型
     */
    void (*destroy)(IPrototype *proto);
};

#endif // CONFIG_PROTOTYPE_H
```

### 4.2 产品：网络配置块

```c
// network_config.h
#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include "config_prototype.h"

/**
 * 网络配置块 - 具体原型
 * 
 * 包含多个配置字段，支持克隆创建副本。
 */
typedef struct {
    IPrototype base;              // 继承原型接口
    
    const char *name;           // 配置名称
    uint8_t mac_addr[6];        // MAC 地址
    uint16_t vlan_id;           // VLAN ID
    uint32_t mtu;               // MTU 大小
    void *custom_data;          // 自定义数据（指针，需要深拷贝）
    size_t custom_data_size;    // 自定义数据大小
} NetworkConfig;

/**
 * 创建网络配置
 */
NetworkConfig* NetworkConfig_create(const char *name);

#endif // NETWORK_CONFIG_H
```

### 4.3 实现克隆方法

```c
// network_config.c
#include "network_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 内部函数 ========== */
static void* shallow_clone(IPrototype *proto);
static void* deep_clone(IPrototype *proto);
static const char* get_name(IPrototype *proto);
static void destroy(IPrototype *proto);

/* ========== 构造函数 ========== */
NetworkConfig* NetworkConfig_create(const char *name) {
    NetworkConfig *config = malloc(sizeof(NetworkConfig));
    if (!config) {
        fprintf(stderr, "[NetworkConfig] 分配内存失败\n");
        return NULL;
    }
    
    /* 初始化基类接口 */
    config->base.clone = shallow_clone;
    config->base.deep_clone = deep_clone;
    config->base.get_name = get_name;
    config->base.destroy = destroy;
    
    /* 初始化字段 */
    config->name = name;
    memset(config->mac_addr, 0, sizeof(config->mac_addr));
    config->vlan_id = 0;
    config->mtu = 1500;
    config->custom_data = NULL;
    config->custom_data_size = 0;
    
    printf("[NetworkConfig] 创建配置: %s\n", name);
    return config;
}

/* ========== 原型方法实现 ========== */

/**
 * 浅拷贝克隆
 * 
 * 使用 memcpy 整体复制结构体。
 * 注意：custom_data 指针会被复制，但指向同一块内存！
 */
static void* shallow_clone(IPrototype *proto) {
    NetworkConfig *orig = (NetworkConfig*)proto;
    
    printf("[NetworkConfig] 浅拷贝克隆: %s\n", orig->name);
    
    NetworkConfig *new_config = malloc(sizeof(NetworkConfig));
    if (!new_config) {
        fprintf(stderr, "[NetworkConfig] 克隆失败: 分配内存失败\n");
        return NULL;
    }
    
    /* 复制所有字段（包括指针值） */
    memcpy(new_config, orig, sizeof(NetworkConfig));
    
    printf("  [浅拷贝] custom_data 指针: %p → %p（共享内存）\n",
           orig->custom_data, new_config->custom_data);
    
    return new_config;
}

/**
 * 深拷贝克隆
 * 
 * 除了复制结构体外，还会复制 custom_data 指向的内容。
 * 新旧实例完全独立。
 */
static void* deep_clone(IPrototype *proto) {
    NetworkConfig *orig = (NetworkConfig*)proto;
    
    printf("[NetworkConfig] 深拷贝克隆: %s\n", orig->name);
    
    NetworkConfig *new_config = malloc(sizeof(NetworkConfig));
    if (!new_config) {
        fprintf(stderr, "[NetworkConfig] 克隆失败: 分配内存失败\n");
        return NULL;
    }
    
    /* 复制基础字段 */
    memcpy(new_config, orig, sizeof(NetworkConfig));
    
    /* 深拷贝 custom_data */
    if (orig->custom_data && orig->custom_data_size > 0) {
        new_config->custom_data = malloc(orig->custom_data_size);
        if (new_config->custom_data) {
            memcpy(new_config->custom_data, orig->custom_data, orig->custom_data_size);
        }
        new_config->custom_data_size = orig->custom_data_size;
        printf("  [深拷贝] custom_data 内容已复制，大小: %zu bytes\n", 
               orig->custom_data_size);
    }
    
    printf("  [深拷贝] custom_data 指针: %p → %p（独立内存）\n",
           orig->custom_data, new_config->custom_data);
    
    return new_config;
}

static const char* get_name(IPrototype *proto) {
    NetworkConfig *config = (NetworkConfig*)proto;
    return config->name;
}

static void destroy(IPrototype *proto) {
    NetworkConfig *config = (NetworkConfig*)proto;
    printf("[NetworkConfig] 销毁配置: %s\n", config->name);
    
    /* 释放 custom_data（如果是独立分配的话） */
    if (config->custom_data) {
        free(config->custom_data);
    }
    free(config);
}
```

### 4.4 原型管理器

```c
// prototype_manager.h
#ifndef PROTOTYPE_MANAGER_H
#define PROTOTYPE_MANAGER_H

#include "config_prototype.h"

/**
 * 原型管理器
 * 
 * 管理一组原型对象，通过名称注册和获取克隆。
 */
typedef struct PrototypeManager PrototypeManager;

PrototypeManager* PrototypeManager_create(void);

/**
 * 注册原型
 */
void PrototypeManager_register(PrototypeManager *mgr, 
                                const char *name, 
                                IPrototype *proto);

/**
 * 通过名称克隆原型
 * @return 新创建的对象（调用者负责销毁）
 */
void* PrototypeManager_clone(PrototypeManager *mgr, const char *name);

/**
 * 销毁管理器
 */
void PrototypeManager_destroy(PrototypeManager *mgr);

#endif // PROTOTYPE_MANAGER_H
```

```c
// prototype_manager.c
#include "prototype_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROTOTYPES 16

typedef struct {
    const char *name;
    IPrototype *proto;
} PrototypeEntry;

struct PrototypeManager {
    PrototypeEntry entries[MAX_PROTOTYPES];
    int count;
};

PrototypeManager* PrototypeManager_create(void) {
    PrototypeManager *mgr = malloc(sizeof(PrototypeManager));
    if (!mgr) return NULL;
    mgr->count = 0;
    printf("[PrototypeManager] 创建原型管理器\n");
    return mgr;
}

void PrototypeManager_register(PrototypeManager *mgr, 
                                const char *name, 
                                IPrototype *proto) {
    if (mgr->count >= MAX_PROTOTYPES) {
        fprintf(stderr, "[PrototypeManager] 已满，无法注册更多原型\n");
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
            printf("[PrototypeManager] 克隆原型: %s\n", name);
            return mgr->entries[i].proto->clone(&mgr->entries[i].proto);
        }
    }
    fprintf(stderr, "[PrototypeManager] 未找到原型: %s\n", name);
    return NULL;
}

void PrototypeManager_destroy(PrototypeManager *mgr) {
    printf("[PrototypeManager] 销毁管理器\n");
    free(mgr);
}
```

### 4.5 主程序

```c
// main.c
#include "network_config.h"
#include "prototype_manager.h"
#include <stdio.h>

int main(void) {
    printf("========================================\n");
    printf("   原型模式 - 固件配置块克隆\n");
    printf("========================================\n\n");
    
    /* ========================================
     * 步骤1：创建原型配置
     * ======================================== */
    printf("=== 步骤1：创建原型配置 ===\n");
    NetworkConfig *proto = NetworkConfig_create("默认模板");
    proto->mac_addr[0] = 0x00;
    proto->mac_addr[1] = 0x11;
    proto->mac_addr[2] = 0x22;
    proto->mac_addr[3] = 0x33;
    proto->mac_addr[4] = 0x44;
    proto->mac_addr[5] = 0x55;
    proto->vlan_id = 100;
    proto->mtu = 1500;
    
    /* 附加自定义数据 */
    char custom[] = "这是自定义配置数据";
    proto->custom_data = malloc(sizeof(custom));
    memcpy(proto->custom_data, custom, sizeof(custom));
    proto->custom_data_size = sizeof(custom);
    
    printf("\n原始配置:\n");
    printf("  MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           proto->mac_addr[0], proto->mac_addr[1], proto->mac_addr[2],
           proto->mac_addr[3], proto->mac_addr[4], proto->mac_addr[5]);
    printf("  VLAN: %u\n", proto->vlan_id);
    printf("  custom_data 地址: %p\n", proto->custom_data);
    
    printf("\n");
    
    /* ========================================
     * 步骤2：浅拷贝克隆
     * ======================================== */
    printf("=== 步骤2：浅拷贝克隆 ===\n");
    NetworkConfig *clone1 = (NetworkConfig*)proto->base.clone(&proto->base);
    
    printf("\n浅拷贝结果:\n");
    printf("  名称: %s\n", clone1->base.get_name(&clone1->base));
    printf("  VLAN: %u (原: %u)\n", clone1->vlan_id, proto->vlan_id);
    printf("  custom_data 地址: %p (原: %p)\n", 
           clone1->custom_data, proto->custom_data);
    printf("  custom_data 内容: %s\n", (char*)clone1->custom_data);
    
    /* 修改克隆的配置，验证是否影响原版 */
    printf("\n修改克隆的 VLAN 和 custom_data...\n");
    clone1->vlan_id = 200;
    strcpy(clone1->custom_data, "修改后的数据");
    
    printf("修改后:\n");
    printf("  原型 VLAN: %u | 克隆 VLAN: %u\n", proto->vlan_id, clone1->vlan_id);
    printf("  原型 custom_data: %s\n", (char*)proto->custom_data);
    printf("  克隆 custom_data: %s\n", (char*)clone1->custom_data);
    
    printf("\n>>> 浅拷贝结论：VLAN是值类型，互不影响；\n");
    printf("                custom_data是指针类型，共享内存，相互影响！\n\n");
    
    /* ========================================
     * 步骤3：深拷贝克隆
     * ======================================== */
    printf("=== 步骤3：深拷贝克隆 ===\n");
    NetworkConfig *clone2 = (NetworkConfig*)proto->base.deep_clone(&proto->base);
    
    printf("\n深拷贝结果:\n");
    printf("  名称: %s\n", clone2->base.get_name(&clone2->base));
    printf("  custom_data 地址: %p (原: %p)\n", 
           clone2->custom_data, proto->custom_data);
    printf("  custom_data 内容: %s\n", (char*)clone2->custom_data);
    
    /* 修改深拷贝的配置，验证是否独立 */
    printf("\n修改深拷贝的 custom_data...\n");
    strcpy(clone2->custom_data, "深拷贝独立数据");
    
    printf("修改后:\n");
    printf("  原型 custom_data: %s\n", (char*)proto->custom_data);
    printf("  深拷贝 custom_data: %s\n", (char*)clone2->custom_data);
    
    printf("\n>>> 深拷贝结论：custom_data 完全独立，修改互不影响 ✓\n\n");
    
    /* ========================================
     * 步骤4：使用原型管理器
     * ======================================== */
    printf("=== 步骤4：使用原型管理器 ===\n");
    PrototypeManager *mgr = PrototypeManager_create();
    PrototypeManager_register(mgr, "默认模板", &proto->base);
    
    NetworkConfig *from_mgr = PrototypeManager_clone(mgr, "默认模板");
    if (from_mgr) {
        printf("从管理器克隆: %s, VLAN=%u\n", 
               from_mgr->base.get_name(&from_mgr->base), from_mgr->vlan_id);
        from_mgr->base.destroy(&from_mgr->base);
    }
    
    /* ========================================
     * 清理资源
     * ======================================== */
    printf("\n=== 清理资源 ===\n");
    clone1->base.destroy(&clone1->base);  /* 浅拷贝，注意 custom_data 已被 free 一次 */
    clone2->base.destroy(&clone2->base);  /* 深拷贝，custom_data 独立释放 */
    proto->base.destroy(&proto->base);    /* 原型，custom_data 最后释放 */
    PrototypeManager_destroy(mgr);
    
    printf("\n========================================\n");
    printf("   总结:\n");
    printf("   - 浅拷贝：快速，但指针字段共享\n");
    printf("   - 深拷贝：完整独立，但需要额外内存\n");
    printf("   - 选择依据：是否有共享数据的需要 ✓\n");
    printf("========================================\n");
    
    return 0;
}
```

---

## 5. 固件配置块的克隆场景

在嵌入式固件中，原型模式非常适合配置管理：

```c
// 固件配置块示例
typedef struct {
    uint32_t magic;
    uint8_t mode;
    uint16_t flags;
    uint8_t reserved[15];
    uint32_t checksum;
} FirmwareConfigBlock;

// 原型管理器管理多种配置模板
FirmwareConfigBlock default_cfg = {...};
FirmwareConfigBlock low_power_cfg = {...};
FirmwareConfigBlock high_perf_cfg = {...};

// 运行时快速克隆模板创建实例
FirmwareConfigBlock *instance = clone_from_template(&default_cfg);
```

---

## 6. 原型模式的适用场景

**应该用：**
- 需要创建对象的副本，但不想知道具体类型（解耦）
- 创建一个对象比直接构造更复杂/更耗时（从原型克隆更快）
- 需要管理一组可复制的模板（原型的原型管理器）

**不应该用：**
- 对象非常简单，没有多个字段（直接构造更直接）
- 不需要独立副本（共享更高效）
- 对象有循环引用（深拷贝复杂，可能死循环）

---

## 7. 本章小结

| 要素 | C 语言实现 |
|:---|:---|
| 原型接口 | `IClone* (*clone)()` + `IClone* (*deep_clone)()` |
| 浅拷贝 | `memcpy(&new, &orig, sizeof(T))` — 指针字段共享 |
| 深拷贝 | `memcpy` + 递归复制指针指向的内容 |
| 原型管理器 | `Map<string, IPrototype*>` — 按名称注册和克隆 |

**原型模式的核心**：把"复制自己"的能力交给对象本身，而不是让外部代码逐字段复制。

---

## 8. 练习

1. 实现一个"日志配置"的原型克隆，支持浅拷贝和深拷贝，验证指针字段是否共享

2. 实现一个"嵌套结构"的深拷贝：A 包含 B，B 包含 C，验证深拷贝后三层都独立

3. 在原型管理器中添加 `deep_clone(name)` 方法，支持深拷贝克隆

4. （思考题）如果原型对象包含指向自己的指针（链表节点），深拷贝该如何处理？会不会产生循环复制的无限递归？

---

## 代码

`include/config_prototype.h` — 原型接口
`include/network_config.h` + `src/network_config.c` — 网络配置原型实现
`include/prototype_manager.h` + `src/prototype_manager.c` — 原型管理器
`src/main.c` — 测试主程序
`Makefile` — 编译脚本