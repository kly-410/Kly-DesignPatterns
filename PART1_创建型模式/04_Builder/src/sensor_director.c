/**
 * 传感器配置指挥者实现
 * 
 * 使用具体的 SensorBuilderImpl 构建标准配置。
 * 展示了如何用 Director 封装标准构建流程。
 */

#include "sensor_director.h"
#include "sensor_builder_impl.h"
#include <stdio.h>
#include <stdlib.h>

/* ==========================================
 * 内部结构
 * ========================================== */

typedef struct SensorDirectorImpl {
    SensorDirector base;
    SensorBuilderImpl *builder;  /* 持有具体建造者 */
} SensorDirectorImpl;

/* ==========================================
 * 内部函数
 * ========================================== */

static SensorConfig* build_high_speed(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    ISensorBuilder *builder = &impl->builder->base;
    
    printf("\n[Director] =======================================\n");
    printf("[Director] 构建【高速采样配置】\n");
    printf("[Director] 场景: 动态测量、快速响应\n");
    printf("[Director] =======================================\n");
    
    /* 重置建造者，开始新的构建 */
    builder->reset(builder);
    
    /* 高速配置：10kHz, 16-bit, LPF, DMA */
    SensorConfig *config = builder
        ->set_sampling_rate(builder, 10000)   /* 10 kHz */
        ->set_filter(builder, "LPF")          /* 低通滤波器 */
        ->set_data_format(builder, 16)        /* 16-bit 数据 */
        ->set_resolution(builder, 16)          /* 16-bit 分辨率 */
        ->enable_dma(builder)                 /* DMA 加速 */
        ->build(builder);
    
    return config;
}

static SensorConfig* build_low_power(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    ISensorBuilder *builder = &impl->builder->base;
    
    printf("\n[Director] =======================================\n");
    printf("[Director] 构建【低功耗配置】\n");
    printf("[Director] 场景: 电池供电、持续监测\n");
    printf("[Director] =======================================\n");
    
    builder->reset(builder);
    
    /* 低功耗配置：100Hz, 8-bit, 关闭不必要功能 */
    SensorConfig *config = builder
        ->set_sampling_rate(builder, 100)     /* 100 Hz */
        ->set_data_format(builder, 8)          /* 8-bit 节省空间 */
        ->set_resolution(builder, 8)
        ->build(builder);
    
    return config;
}

static SensorConfig* build_high_precision(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    ISensorBuilder *builder = &impl->builder->base;
    
    printf("\n[Director] =======================================\n");
    printf("[Director] 构建【高精度配置】\n");
    printf("[Director] 场景: 静态测量、精确数据\n");
    printf("[Director] =======================================\n");
    
    builder->reset(builder);
    
    /* 高精度配置：1kHz, 24-bit, LPF, 校准 */
    SensorConfig *config = builder
        ->set_sampling_rate(builder, 1000)    /* 1 kHz */
        ->set_filter(builder, "LPF")          /* 低通滤波 */
        ->set_data_format(builder, 24)         /* 24-bit 高精度 */
        ->set_resolution(builder, 24)
        ->enable_calibration(builder)          /* 使能校准 */
        ->build(builder);
    
    return config;
}

static void director_destroy(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    printf("[Director] 销毁指挥者\n");
    free(impl);
}

/* ==========================================
 * 构造函数
 * ========================================== */

SensorDirector* SensorDirector_create(void) {
    SensorDirectorImpl *impl = malloc(sizeof(SensorDirectorImpl));
    if (!impl) {
        fprintf(stderr, "[Director] 错误: 分配内存失败\n");
        return NULL;
    }
    
    /* 创建具体建造者 */
    impl->builder = SensorBuilderImpl_create();
    if (!impl->builder) {
        free(impl);
        return NULL;
    }
    
    /* 初始化接口方法 */
    impl->base.build_high_speed_config = build_high_speed;
    impl->base.build_low_power_config = build_low_power;
    impl->base.build_high_precision_config = build_high_precision;
    impl->base.destroy = director_destroy;
    
    printf("[Director] 传感器配置指挥者已创建\n");
    return &impl->base;
}