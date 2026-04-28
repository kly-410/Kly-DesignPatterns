/**
 * 传感器配置指挥者 - Director 角色
 * 
 * 定义标准的传感器配置序列。
 * 指挥者知道最优的构造步骤，但不关心每个步骤的具体实现。
 */
#ifndef SENSOR_DIRECTOR_H
#define SENSOR_DIRECTOR_H

#include "sensor_config.h"

/* 前向声明 */
typedef struct SensorDirector SensorDirector;

/**
 * 传感器配置指挥者
 * 
 * 持有建造者，定义标准配置流程。
 * 可以通过不同方法构建不同用途的标准配置。
 */
struct SensorDirector {
    /**
     * 构建高速采样配置（用于动态测量）
     * 特性: 10kHz采样, 16-bit, LPF, DMA使能
     */
    SensorConfig* (*build_high_speed_config)(SensorDirector *director);
    
    /**
     * 构建低功耗配置（用于电池供电场景）
     * 特性: 100Hz采样, 8-bit, 无DMA/中断
     */
    SensorConfig* (*build_low_power_config)(SensorDirector *director);
    
    /**
     * 构建高精度配置（用于静态测量）
     * 特性: 1kHz采样, 24-bit, LPF, 校准使能
     */
    SensorConfig* (*build_high_precision_config)(SensorDirector *director);
    
    /**
     * 销毁指挥者
     */
    void (*destroy)(SensorDirector *director);
};

/**
 * 创建传感器配置指挥者
 */
SensorDirector* SensorDirector_create(void);

#endif // SENSOR_DIRECTOR_H