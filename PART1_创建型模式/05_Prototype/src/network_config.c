/**
 * 网络配置块实现
 * 
 * 实现了 IPrototype 接口的克隆方法。
 * 重点展示浅拷贝和深拷贝的区别。
 */

#include "network_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================
 * 内部函数
 * ========================================== */

static void* shallow_clone(IPrototype *proto);
static void* deep_clone(IPrototype *proto);
static const char* get_name(IPrototype *proto);
static void destroy(IPrototype *proto);

/* ==========================================
 * 构造函数
 * ========================================== */

NetworkConfig* NetworkConfig_create(const char *name) {
    NetworkConfig *config = malloc(sizeof(NetworkConfig));
    if (!config) {
        fprintf(stderr, "[NetworkConfig] 错误: 分配内存失败\n");
        return NULL;
    }
    
    /* 初始化基类接口（虚函数表） */
    config->base.clone = shallow_clone;
    config->base.deep_clone = deep_clone;
    config->base.get_name = get_name;
    config->base.destroy = destroy;
    
    /* 初始化字段 */
    config->name = name;
    memset(config->mac_addr, 0, sizeof(config->mac_addr));
    config->vlan_id = 0;
    config->mtu = 1500;  /* 默认 MTU */
    config->flags = 0;
    config->custom_data = NULL;
    config->custom_data_size = 0;
    
    printf("[NetworkConfig] 创建配置: %s\n", name);
    return config;
}

void NetworkConfig_set_mac(NetworkConfig *config, uint8_t *mac) {
    memcpy(config->mac_addr, mac, 6);
}

void NetworkConfig_print(const NetworkConfig *config) {
    printf("  名称: %s\n", config->name);
    printf("  MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           config->mac_addr[0], config->mac_addr[1], config->mac_addr[2],
           config->mac_addr[3], config->mac_addr[4], config->mac_addr[5]);
    printf("  VLAN: %u\n", config->vlan_id);
    printf("  MTU: %u\n", config->mtu);
    printf("  标志: 0x%02X\n", config->flags);
    printf("  custom_data: %p (大小: %zu bytes)\n", 
           config->custom_data, config->custom_data_size);
    if (config->custom_data && config->custom_data_size > 0) {
        printf("  custom_data 内容: \"%s\"\n", (char*)config->custom_data);
    }
}

/* ==========================================
 * IPrototype 接口实现
 * ========================================== */

/**
 * 浅拷贝克隆
 * 
 * 使用 memcpy 整体复制结构体。
 * 关键点：custom_data 指针被复制，但指向同一块内存！
 * 
 * 警告：调用者需要注意，浅拷贝的对象和原对象共享 custom_data，
 *       销毁时不能重复释放 custom_data。
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
    
    /* 注意：custom_data 指针被复制，指向同一块内存 */
    printf("  [浅拷贝] custom_data 指针: %p → %p (共享内存)\n",
           orig->custom_data, new_config->custom_data);
    
    return new_config;
}

/**
 * 深拷贝克隆
 * 
 * 除了复制结构体外，还会递归复制 custom_data 指向的内容。
 * 新旧实例完全独立，修改一个不影响另一个。
 */
static void* deep_clone(IPrototype *proto) {
    NetworkConfig *orig = (NetworkConfig*)proto;
    
    printf("[NetworkConfig] 深拷贝克隆: %s\n", orig->name);
    
    NetworkConfig *new_config = malloc(sizeof(NetworkConfig));
    if (!new_config) {
        fprintf(stderr, "[NetworkConfig] 克隆失败: 分配内存失败\n");
        return NULL;
    }
    
    /* 复制基础字段（包括指针值本身） */
    memcpy(new_config, orig, sizeof(NetworkConfig));
    
    /* 深拷贝 custom_data 内容 */
    if (orig->custom_data && orig->custom_data_size > 0) {
        new_config->custom_data = malloc(orig->custom_data_size);
        if (new_config->custom_data) {
            memcpy(new_config->custom_data, 
                   orig->custom_data, 
                   orig->custom_data_size);
        }
        /* custom_data_size 也复制了 */
        printf("  [深拷贝] custom_data 内容已复制，大小: %zu bytes\n", 
               orig->custom_data_size);
    }
    
    printf("  [深拷贝] custom_data 指针: %p → %p (独立内存)\n",
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
    
    /* 释放 custom_data（如果有） */
    if (config->custom_data) {
        printf("  [销毁] 释放 custom_data: %p\n", config->custom_data);
        free(config->custom_data);
        config->custom_data = NULL;
    }
    
    printf("  [销毁] 释放 NetworkConfig 结构体\n");
    free(config);
}