/**
 * 传感器配置建造者 - 具体建造者角色
 * 
 * 实现 ISensorBuilder 接口，分步骤构建 SensorConfig。
 */
#ifndef SENSOR_BUILDER_IMPL_H
#define SENSOR_BUILDER_IMPL_H

#include "sensor_builder.h"

/**
 * 传感器配置具体建造者
 * 
 * 持有正在构建的配置对象，实现各个构建步骤。
 * 支持链式调用，方法返回 Builder 自身。
 */
typedef struct {
    ISensorBuilder base;          // 继承 ISensorBuilder 接口
    SensorConfig config;          // 正在构建的配置
    int is_built;                 // 是否已经 build 过
} SensorBuilderImpl;

/**
 * 创建传感器配置建造者
 */
SensorBuilderImpl* SensorBuilderImpl_create(void);

#endif // SENSOR_BUILDER_IMPL_H