/**
 * 传感器配置建造者实现
 * 
 * 实现了 ISensorBuilder 接口的各个方法。
 * 每个方法返回 Builder 自身，支持链式调用：
 * 
 *   builder->set_sampling_rate(b, 1000)
 *          ->set_filter(b, "LPF")
 *          ->build(b);
 */

#include "sensor_builder_impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================
 * ISensorBuilder 方法实现
 * ========================================== */

static ISensorBuilder* impl_set_sampling_rate(ISensorBuilder *builder, uint32_t rate_hz) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    impl->config.sampling_rate_hz = rate_hz;
    printf("[Builder]   设置采样率: %u Hz\n", rate_hz);
    return builder;
}

static ISensorBuilder* impl_set_filter(ISensorBuilder *builder, const char *filter_type) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    
    /* 释放之前的字符串（如果有） */
    if (impl->config.filter_type) {
        free((void*)impl->config.filter_type);
    }
    
    /* 复制字符串到堆上 */
    size_t len = strlen(filter_type) + 1;
    impl->config.filter_type = malloc(len);
    if (impl->config.filter_type) {
        memcpy((void*)impl->config.filter_type, filter_type, len);
    }
    
    printf("[Builder]   设置滤波器: %s\n", filter_type);
    return builder;
}

static ISensorBuilder* impl_set_data_format(ISensorBuilder *builder, uint8_t bits) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    impl->config.data_format = bits;
    printf("[Builder]   设置数据格式: %u-bit\n", bits);
    return builder;
}

static ISensorBuilder* impl_set_resolution(ISensorBuilder *builder, uint8_t bits) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    impl->config.resolution = bits;
    printf("[Builder]   设置分辨率: %u-bit\n", bits);
    return builder;
}

static ISensorBuilder* impl_enable_interrupt(ISensorBuilder *builder) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    impl->config.interrupt_enabled = 1;
    printf("[Builder]   使能中断\n");
    return builder;
}

static ISensorBuilder* impl_enable_dma(ISensorBuilder *builder) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    impl->config.dma_enabled = 1;
    printf("[Builder]   使能 DMA\n");
    return builder;
}

static ISensorBuilder* impl_enable_calibration(ISensorBuilder *builder) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    impl->config.calibration_enabled = 1;
    printf("[Builder]   使能校准\n");
    return builder;
}

static SensorConfig* impl_build(ISensorBuilder *builder) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    
    if (impl->is_built) {
        printf("[Builder]   警告: 已经 build 过，将返回已构建的配置\n");
        return &impl->config;
    }
    
    /* 打印最终配置摘要 */
    printf("[Builder]   ========================================\n");
    printf("[Builder]   构建完成！配置摘要:\n");
    printf("[Builder]   ----------------------------------------\n");
    printf("[Builder]     采样率: %u Hz\n", impl->config.sampling_rate_hz);
    printf("[Builder]     滤波器: %s\n", 
           impl->config.filter_type ? impl->config.filter_type : "未设置");
    printf("[Builder]     数据格式: %u-bit\n", impl->config.data_format);
    printf("[Builder]     分辨率: %u-bit\n", impl->config.resolution);
    printf("[Builder]     中断: %s\n", 
           impl->config.interrupt_enabled ? "使能" : "禁用");
    printf("[Builder]     DMA: %s\n", 
           impl->config.dma_enabled ? "使能" : "禁用");
    printf("[Builder]     校准: %s\n", 
           impl->config.calibration_enabled ? "使能" : "禁用");
    printf("[Builder]   ========================================\n");
    
    impl->is_built = 1;
    return &impl->config;
}

static void impl_reset(ISensorBuilder *builder) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    
    /* 释放之前分配的字符串 */
    if (impl->config.filter_type) {
        free((void*)impl->config.filter_type);
        impl->config.filter_type = NULL;
    }
    
    /* 重置为默认值 */
    memset(&impl->config, 0, sizeof(SensorConfig));
    impl->is_built = 0;
    
    printf("[Builder]   建造者已重置，可以开始新的配置\n");
}

/* ==========================================
 * 构造函数
 * ========================================== */

SensorBuilderImpl* SensorBuilderImpl_create(void) {
    SensorBuilderImpl *impl = malloc(sizeof(SensorBuilderImpl));
    if (!impl) {
        fprintf(stderr, "[Builder] 错误: 分配内存失败\n");
        return NULL;
    }
    
    /* 初始化虚函数表（设置接口实现） */
    impl->base.set_sampling_rate = impl_set_sampling_rate;
    impl->base.set_filter = impl_set_filter;
    impl->base.set_data_format = impl_set_data_format;
    impl->base.set_resolution = impl_set_resolution;
    impl->base.enable_interrupt = impl_enable_interrupt;
    impl->base.enable_dma = impl_enable_dma;
    impl->base.enable_calibration = impl_enable_calibration;
    impl->base.build = impl_build;
    impl->base.reset = impl_reset;
    
    /* 初始化配置为 0 */
    memset(&impl->config, 0, sizeof(SensorConfig));
    impl->is_built = 0;
    
    printf("[Builder] 传感器配置建造者已创建\n");
    return impl;
}