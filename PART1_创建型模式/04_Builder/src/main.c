/**
 * 建造者模式测试程序
 * 
 * 演示两种使用方式：
 * 1. 直接使用建造者：灵活自定义配置
 * 2. 使用指挥者：复用标准配置流程
 */

#include "sensor_builder_impl.h"
#include "sensor_director.h"
#include <stdio.h>

/**
 * 打印配置详情
 */
void print_config(const char *name, SensorConfig *config) {
    printf("    [配置 %s]\n", name);
    printf("      采样率: %u Hz\n", config->sampling_rate_hz);
    printf("      滤波器: %s\n", 
           config->filter_type ? config->filter_type : "未设置");
    printf("      数据格式: %u-bit\n", config->data_format);
    printf("      分辨率: %u-bit\n", config->resolution);
    printf("      中断: %s\n", config->interrupt_enabled ? "使能" : "禁用");
    printf("      DMA: %s\n", config->dma_enabled ? "使能" : "禁用");
    printf("      校准: %s\n", config->calibration_enabled ? "使能" : "禁用");
}

int main(void) {
    printf("========================================\n");
    printf("   建造者模式 - 嵌入式传感器配置\n");
    printf("========================================\n\n");
    
    /* ========================================
     * 方式1：直接使用建造者（自定义配置）
     * ======================================== */
    printf("=== 方式1：直接使用建造者（自定义）===\n");
    printf("    链式调用，每步都有方法名，可读性高\n\n");
    
    SensorBuilderImpl *custom_builder = SensorBuilderImpl_create();
    ISensorBuilder *builder = &custom_builder->base;
    
    printf("\n[主程序] 构建自定义配置: 中频采样 + 中断 + 校准\n");
    SensorConfig *custom_config = builder
        ->set_sampling_rate(builder, 5000)      /* 5 kHz */
        ->set_filter(builder, "HPF")            /* 高通滤波 */
        ->set_data_format(builder, 12)          /* 12-bit */
        ->set_resolution(builder, 12)
        ->enable_interrupt(builder)            /* 使能中断 */
        ->enable_calibration(builder)          /* 使能校准 */
        ->build(builder);
    
    printf("\n>>> 自定义配置结果:\n");
    print_config("custom", custom_config);
    
    /* ========================================
     * 方式2：使用指挥者（标准配置）
     * ======================================== */
    printf("\n\n=== 方式2：使用指挥者（标准配置）===\n");
    printf("    定义了三种标准配置流程，可直接复用\n\n");
    
    SensorDirector *director = SensorDirector_create();
    
    /* 高速采样配置 */
    printf("\n>>> 高速采样配置结果:\n");
    SensorConfig *hs_config = director->build_high_speed_config(director);
    print_config("high_speed", hs_config);
    
    /* 低功耗配置 */
    printf("\n>>> 低功耗配置结果:\n");
    SensorConfig *lp_config = director->build_low_power_config(director);
    print_config("low_power", lp_config);
    
    /* 高精度配置 */
    printf("\n>>> 高精度配置结果:\n");
    SensorConfig *hp_config = director->build_high_precision_config(director);
    print_config("high_precision", hp_config);
    
    printf("\n========================================\n");
    printf("   总结:\n");
    printf("   - 建造者：灵活自定义，每步有名称\n");
    printf("   - 指挥者：标准流程，可复用\n");
    printf("   - 链式调用：代码简洁，可读性高\n");
    printf("   - 对比构造函数：参数再多也不乱 ✓\n");
    printf("========================================\n");
    
    /* 清理 */
    director->destroy(director);
    
    return 0;
}