/**
 * 传感器配置 - 产品角色
 * 
 * 这是一个相对复杂的配置结构体，包含多个相互关联的参数。
 * 使用建造者模式分步构造，比大型构造函数更易读。
 */
#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>

/**
 * 传感器配置
 */
typedef struct {
    uint32_t sampling_rate_hz;    // 采样率（Hz）
    const char *filter_type;       // 滤波器类型（字符串，堆上分配）
    uint8_t data_format;          // 数据格式位数 (8/12/16/24)
    uint8_t resolution;           // 分辨率位数
    int interrupt_enabled:1;     // 中断使能标志
    int dma_enabled:1;           // DMA 使能标志
    int calibration_enabled:1;    // 校准使能标志
} SensorConfig;

/**
 * 创建默认配置的传感器
 * 
 * 作为参考：传统构造函数方式一次性设置所有参数。
 * 这里提供一个默认配置供参考。
 */
SensorConfig* SensorConfig_create_default(void);

#endif // SENSOR_CONFIG_H