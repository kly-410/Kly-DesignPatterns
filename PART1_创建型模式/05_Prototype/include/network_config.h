/**
 * 网络配置块 - 具体原型
 * 
 * 包含网卡的多种配置字段，支持克隆创建副本。
 * 用于演示原型模式：浅拷贝 vs 深拷贝。
 */
#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include "config_prototype.h"
#include <stdint.h>
#include <stddef.h>

/**
 * 网络配置块
 * 
 * 包含配置字段和自定义数据区域。
 * 自定义数据（custom_data）是指针类型，展示了深浅拷贝的区别。
 */
typedef struct {
    IPrototype base;              // 继承原型接口
    
    const char *name;           // 配置名称
    uint8_t mac_addr[6];        // MAC 地址（值类型，直接复制）
    uint16_t vlan_id;           // VLAN ID（值类型，直接复制）
    uint32_t mtu;               // MTU 大小（值类型，直接复制）
    uint8_t flags;              // 标志位（值类型，直接复制）
    
    /**
     * 自定义数据区域（指针类型）
     * 
     * 这是关键字段：
     * - 浅拷贝时：只复制指针地址，两个对象共享同一块内存
     * - 深拷贝时：分配新内存，复制内容，两个对象独立
     */
    void *custom_data;          
    size_t custom_data_size;    // 自定义数据大小
} NetworkConfig;

/**
 * 创建网络配置
 * 
 * @param name 配置名称
 * @return NetworkConfig* 新创建的配置对象
 */
NetworkConfig* NetworkConfig_create(const char *name);

/**
 * 设置 MAC 地址
 */
void NetworkConfig_set_mac(NetworkConfig *config, uint8_t *mac);

/**
 * 打印配置详情
 */
void NetworkConfig_print(const NetworkConfig *config);

#endif // NETWORK_CONFIG_H