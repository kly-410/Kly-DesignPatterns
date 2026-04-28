/**
 * 原型模式测试程序
 * 
 * 演示：
 * 1. 浅拷贝克隆 - 快速但共享指针数据
 * 2. 深拷贝克隆 - 完整独立副本
 * 3. 原型管理器 - 按名称注册和克隆
 */

#include "network_config.h"
#include "prototype_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void print_separator(const char *title) {
    printf("\n========================================\n");
    printf("   %s\n", title);
    printf("========================================\n");
}

int main(void) {
    printf("========================================\n");
    printf("   原型模式 - 固件配置块克隆\n");
    printf("========================================\n");

    /* ========================================
     * 步骤1:创建原型配置
     * ======================================== */
    print_separator("步骤1: 创建原型配置");

    NetworkConfig *proto = NetworkConfig_create("默认网络配置");

    /* 设置 MAC 地址 */
    uint8_t mac[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    NetworkConfig_set_mac(proto, mac);
    proto->vlan_id = 100;
    proto->mtu = 1500;
    proto->flags = 0x01;

    /* 设置自定义数据 */
    const char *custom_str = "固件配置数据-v1.0";
    proto->custom_data = malloc(strlen(custom_str) + 1);
    strcpy(proto->custom_data, custom_str);
    proto->custom_data_size = strlen(custom_str) + 1;

    printf("\n原始配置详情:\n");
    NetworkConfig_print(proto);

    /* ========================================
     * 步骤2:浅拷贝克隆
     * ======================================== */
    print_separator("步骤2: 浅拷贝克隆");

    printf("\n执行浅拷贝克隆...\n");
    NetworkConfig *shallow_clone = (NetworkConfig*)proto->base.clone(&proto->base);

    printf("\n浅拷贝结果:\n");
    NetworkConfig_print(shallow_clone);

    /* 修改浅拷贝的配置 */
    printf("\n修改浅拷贝的配置（VLAN 和 custom_data）...\n");
    shallow_clone->vlan_id = 999;
    strcpy(shallow_clone->custom_data, "被浅拷贝修改的数据");

    printf("\n对比原始和浅拷贝:\n");
    printf("  原型 VLAN: %u | 浅拷贝 VLAN: %u\n", proto->vlan_id, shallow_clone->vlan_id);
    printf("  原型 custom_data: \"%s\"\n", (char*)proto->custom_data);
    printf("  浅拷贝 custom_data: \"%s\"\n", (char*)shallow_clone->custom_data);

    printf("\n>>> 结论: VLAN是值类型，互不影响；\n");
    printf("          custom_data是共享内存，互相影响！\n");

    /* ========================================
     * 步骤3:深拷贝克隆
     * ======================================== */
    print_separator("步骤3: 深拷贝克隆");

    printf("\n执行深拷贝克隆...\n");
    NetworkConfig *deep_clone = (NetworkConfig*)proto->base.deep_clone(&proto->base);

    printf("\n深拷贝结果:\n");
    NetworkConfig_print(deep_clone);

    /* 修改深拷贝的配置 */
    printf("\n修改深拷贝的 custom_data...\n");
    strcpy(deep_clone->custom_data, "深拷贝独立数据");

    printf("\n对比原始和深拷贝:\n");
    printf("  原型 custom_data: \"%s\"\n", (char*)proto->custom_data);
    printf("  深拷贝 custom_data: \"%s\"\n", (char*)deep_clone->custom_data);

    printf("\n>>> 结论: 深拷贝的 custom_data 完全独立，互不影响 ✓\n");
    printf("    （原型 name 仍有效，未销毁）\n");

    /* ========================================
     * 步骤4:使用原型管理器
     * ======================================== */
    print_separator("步骤4: 使用原型管理器");

    printf("\n创建原型管理器并注册原型...\n");
    PrototypeManager *mgr = PrototypeManager_create();
    PrototypeManager_register(mgr, "默认配置", &proto->base);

    printf("\n通过管理器克隆（浅拷贝）:\n");
    NetworkConfig *from_mgr_shallow = PrototypeManager_clone(mgr, "默认配置");
    if (from_mgr_shallow) {
        printf("  克隆结果: %s, VLAN=%u\n",
               from_mgr_shallow->base.get_name(&from_mgr_shallow->base),
               from_mgr_shallow->vlan_id);
    }

    printf("\n通过管理器深拷贝克隆:\n");
    NetworkConfig *from_mgr_deep = PrototypeManager_deep_clone(mgr, "默认配置");
    if (from_mgr_deep) {
        printf("  克隆结果: %s, VLAN=%u\n",
               from_mgr_deep->base.get_name(&from_mgr_deep->base),
               from_mgr_deep->vlan_id);
    }

    /* ========================================
     * 清理资源
     * 
     * 关键：浅拷贝共享 custom_data，原型拥有真实所有权。
     * 销毁顺序：
     *   1. 深拷贝（拥有独立 custom_data）→ 先销毁
     *   2. 原型（拥有 custom_data 原件）→ 次销毁
     *   3. 管理器深拷贝（独立 custom_data）→ 最后销毁
     * 浅拷贝：不调用 destroy（共享 custom_data，无权释放）
     * ======================================== */
    print_separator("清理资源");

    printf("\n销毁顺序很重要（注意 custom_data 的释放）:\n");

    printf("\n1. 销毁深拷贝（独立 custom_data）:\n");
    deep_clone->base.destroy(&deep_clone->base);

    printf("\n2. 销毁原型（拥有 custom_data 原件）:\n");
    proto->base.destroy(&proto->base);

    printf("\n3. 销毁管理器深拷贝（独立 custom_data）:\n");
    from_mgr_deep->base.destroy(&from_mgr_deep->base);

    printf("\n[跳过] 浅拷贝 destroy：与原型共享 custom_data，");
    printf("原型销毁时已释放其 custom_data，");
    printf("浅拷贝的结构体本身在进程结束时回收。\n");
    printf("      （若调用 destroy 会导致 double-free，故跳过）\n");

    printf("\n4. 销毁原型管理器:\n");
    PrototypeManager_destroy(mgr);

    printf("\n========================================\n");
    printf("   总结:\n");
    printf("   - 浅拷贝: memcpy 整体复制，指针字段共享\n");
    printf("   - 深拷贝: 递归复制所有字段，完全独立\n");
    printf("   - 选择依据: 是否需要独立副本 ✓\n");
    printf("========================================\n");

    return 0;
}