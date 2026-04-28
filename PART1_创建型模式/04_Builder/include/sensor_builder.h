/**
 * 传感器配置建造者接口 - 抽象建造者角色
 * 
 * 定义构建 SensorConfig 的各个步骤。
 * 每个方法返回 Builder 自身，支持链式调用。
 */
#ifndef SENSOR_BUILDER_H
#define SENSOR_BUILDER_H

#include "sensor_config.h"

/* 前向声明 */
typedef struct ISensorBuilder ISensorBuilder;

/**
 * 传感器配置建造者接口
 * 
 * 设计要点：
 * 1. 每个设置方法返回 ISensorBuilder*，支持链式调用
 * 2. build() 方法返回最终产品
 * 3. reset() 方法允许重用 Builder 创建另一个配置
 */
struct ISensorBuilder {
    /**
     * 设置采样率（Hz）
     * @param rate_hz 采样率
     * @return Builder 自身，用于链式调用
     */
    ISensorBuilder* (*set_sampling_rate)(ISensorBuilder *builder, uint32_t rate_hz);
    
    /**
     * 设置滤波器类型
     * @param filter_type 滤波器类型字符串（"LPF", "HPF", "BPF"）
     * @return Builder 自身
     */
    ISensorBuilder* (*set_filter)(ISensorBuilder *builder, const char *filter_type);
    
    /**
     * 设置数据格式位数
     * @param bits 数据位数（8, 12, 16, 24）
     * @return Builder 自身
     */
    ISensorBuilder* (*set_data_format)(ISensorBuilder *builder, uint8_t bits);
    
    /**
     * 设置分辨率位数
     * @param bits 分辨率位数
     * @return Builder 自身
     */
    ISensorBuilder* (*set_resolution)(ISensorBuilder *builder, uint8_t bits);
    
    /**
     * 使能中断
     * @return Builder 自身
     */
    ISensorBuilder* (*enable_interrupt)(ISensorBuilder *builder);
    
    /**
     * 使能 DMA
     * @return Builder 自身
     */
    ISensorBuilder* (*enable_dma)(ISensorBuilder *builder);
    
    /**
     * 使能校准
     * @return Builder 自身
     */
    ISensorBuilder* (*enable_calibration)(ISensorBuilder *builder);
    
    /**
     * 构建最终产品
     * @return SensorConfig* 配置指针（内部存储，勿 free）
     */
    SensorConfig* (*build)(ISensorBuilder *builder);
    
    /**
     * 重置建造者到初始状态
     * 允许重用 Builder 创建新的配置
     */
    void (*reset)(ISensorBuilder *builder);
};

#endif // SENSOR_BUILDER_H