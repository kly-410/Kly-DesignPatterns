# 建造者模式（Builder Pattern）

## 本章目标

1. 理解建造者模式解决的问题：**分步构造复杂对象**，特别是构造参数多、步骤有依赖的情况
2. 掌握链式调用（Method Chaining）在 C 语言中的实现
3. 理解嵌入式固件初始化序列中的应用（分步初始化外设）
4. 掌握 C 语言实现建造者模式（Builder + Director）

---

## 1. 从一个问题开始

设计一个嵌入式固件的"传感器模块"，需要配置以下参数：
- 采样率（Sampling Rate）
- 滤波器类型（Filter Type）
- 数据格式（Data Format）
- 中断使能（Interrupt Enable）
- DMA 使能（DMA Enable）

**糟糕的设计**：
```c
// 一个有 10 个参数的构造函数
SensorConfig *config = SensorConfig_create(
    1000,      // 采样率
    FILTER_LPF,  // 滤波器类型
    FORMAT_16BIT,  // 数据格式
    1,          // 中断使能
    1,          // DMA 使能
    // ... 还有 5 个参数
);
```
调用时根本不知道每个数字是什么意思，可读性为零。

**建造者的思路**：把构造过程分成多个步骤，每一步负责设置一个相关参数，方法名说明含义。

---

## 2. 建造者模式的定义

> **建造者模式**：将一个复杂对象的构建与它的表示分离，使得同样的构建过程可以创建不同的表示。

五个要素：
- **Product（产品）**：要创建的复杂对象
- **Builder（抽象建造者）**：定义创建产品的各个部分的接口
- **ConcreteBuilder（具体建造者）**：实现 Builder 接口，构建和装配产品的各个部分
- **Director（指挥者）**：构造一个使用 Builder 接口的对象，控制构建过程

---

## 3. 建造者 vs 构造函数

| 对比 | 构造函数 | 建造者 |
|:---|:---|:---|
| 参数数量 | 多，调用时容易出错 | 分多次设置，每步有名称 |
| 可读性 | 参数意义不明确 | 方法名说明参数含义 |
| 可选参数 | 通常需要重载多个构造函数 | 每个参数都是可选的 |
| 不可变对象 | 一般需要全部参数 | 可以先生成 Builder，逐步设置，最后 Build |
| 适用场景 | 参数少（≤3-4 个）| 参数多、有默认值、有顺序依赖 |

---

## 4. C 语言实现：传感器配置建造者

### 4.1 产品：传感器配置

```c
// sensor_config.h
#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>

/**
 * 传感器配置产品
 * 
 * 这是一个相对复杂的配置结构体，包含多个相互关联的参数。
 */
typedef struct {
    uint32_t sampling_rate_hz;    // 采样率 Hz
    const char *filter_type;      // 滤波器类型字符串
    uint8_t data_format;         // 数据格式 (8/12/16 bit)
    uint8_t resolution;          // 分辨率位数
    int interrupt_enabled;       // 中断使能
    int dma_enabled;             // DMA 使能
    int calibration_enabled;     // 校准使能
} SensorConfig;

/**
 * 创建默认配置的传感器
 */
SensorConfig* SensorConfig_create_default(void);

#endif // SENSOR_CONFIG_H
```

### 4.2 抽象建造者

```c
// sensor_builder.h
#ifndef SENSOR_BUILDER_H
#define SENSOR_BUILDER_H

#include "sensor_config.h"

/* 前向声明 */
typedef struct ISensorBuilder ISensorBuilder;

/**
 * 传感器配置建造者接口（抽象建造者）
 * 
 * 定义构建 SensorConfig 的各个步骤。
 * 具体建造者（如 SensorBuilderImpl）实现这些方法。
 */
struct ISensorBuilder {
    /**
     * 设置采样率
     */
    ISensorBuilder* (*set_sampling_rate)(ISensorBuilder *builder, uint32_t rate_hz);
    
    /**
     * 设置滤波器类型
     */
    ISensorBuilder* (*set_filter)(ISensorBuilder *builder, const char *filter_type);
    
    /**
     * 设置数据格式
     */
    ISensorBuilder* (*set_data_format)(ISensorBuilder *builder, uint8_t bits);
    
    /**
     * 设置分辨率
     */
    ISensorBuilder* (*set_resolution)(ISensorBuilder *builder, uint8_t bits);
    
    /**
     * 使能中断
     */
    ISensorBuilder* (*enable_interrupt)(ISensorBuilder *builder);
    
    /**
     * 使能 DMA
     */
    ISensorBuilder* (*enable_dma)(ISensorBuilder *builder);
    
    /**
     * 使能校准
     */
    ISensorBuilder* (*enable_calibration)(ISensorBuilder *builder);
    
    /**
     * 构建出最终产品
     */
    SensorConfig* (*build)(ISensorBuilder *builder);
    
    /**
     * 重置建造者到初始状态
     */
    void (*reset)(ISensorBuilder *builder);
};

#endif // SENSOR_BUILDER_H
```

### 4.3 具体建造者实现

```c
// sensor_builder_impl.h
#ifndef SENSOR_BUILDER_IMPL_H
#define SENSOR_BUILDER_IMPL_H

#include "sensor_builder.h"

/**
 * 传感器配置具体建造者
 * 
 * 实现 ISensorBuilder 接口，分步骤构建 SensorConfig。
 * 支持链式调用，例如：
 *   builder->set_sampling_rate(b, 1000)
 *           ->set_filter(b, "LPF")
 *           ->build(b);
 */
typedef struct {
    ISensorBuilder base;         // 继承 ISensorBuilder 接口
    
    SensorConfig config;         // 正在构建的配置
    int is_built;                // 是否已经 build 过
} SensorBuilderImpl;

/**
 * 创建传感器建造者
 */
SensorBuilderImpl* SensorBuilderImpl_create(void);

#endif // SENSOR_BUILDER_IMPL_H
```

```c
// sensor_builder_impl.c
#include "sensor_builder_impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 内部辅助函数 ========== */
static ISensorBuilder* return_self(ISensorBuilder *b) { return b; }

/* ========== ISensorBuilder 方法实现 ========== */

static ISensorBuilder* impl_set_sampling_rate(ISensorBuilder *builder, uint32_t rate_hz) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    impl->config.sampling_rate_hz = rate_hz;
    printf("[Builder]   设置采样率: %u Hz\n", rate_hz);
    return builder;
}

static ISensorBuilder* impl_set_filter(ISensorBuilder *builder, const char *filter_type) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
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
    
    printf("[Builder]   构建完成！\n");
    printf("              采样率: %u Hz\n", impl->config.sampling_rate_hz);
    printf("              滤波器: %s\n", impl->config.filter_type ? impl->config.filter_type : "未设置");
    printf("              数据格式: %u-bit\n", impl->config.data_format);
    printf("              分辨率: %u-bit\n", impl->config.resolution);
    printf("              中断: %s\n", impl->config.interrupt_enabled ? "使能" : "禁用");
    printf("              DMA: %s\n", impl->config.dma_enabled ? "使能" : "禁用");
    printf("              校准: %s\n", impl->config.calibration_enabled ? "使能" : "禁用");
    
    impl->is_built = 1;
    return &impl->config;
}

static void impl_reset(ISensorBuilder *builder) {
    SensorBuilderImpl *impl = (SensorBuilderImpl*)builder;
    
    /* 释放之前分配的字符串 */
    if (impl->config.filter_type) {
        free((void*)impl->config.filter_type);
    }
    
    /* 重置为默认值 */
    memset(&impl->config, 0, sizeof(SensorConfig));
    impl->is_built = 0;
    
    printf("[Builder]   建造者已重置\n");
}

/* ========== 构造函数 ========== */
SensorBuilderImpl* SensorBuilderImpl_create(void) {
    SensorBuilderImpl *impl = malloc(sizeof(SensorBuilderImpl));
    if (!impl) {
        fprintf(stderr, "[Builder] 分配内存失败\n");
        return NULL;
    }
    
    /* 初始化接口（设置虚函数表） */
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
    
    printf("[Builder]   传感器配置建造者已创建\n");
    return impl;
}
```

### 4.4 指挥者：标准配置序列

```c
// sensor_director.h
#ifndef SENSOR_DIRECTOR_H
#define SENSOR_DIRECTOR_H

#include "sensor_builder.h"

/**
 * 传感器配置指挥者
 * 
 * 定义标准的传感器配置序列。
 * 指挥者知道最优的构造步骤，但不关心每个步骤的具体实现。
 */
typedef struct {
    /**
     * 构建高速采样配置（用于动态测量）
     * 采样率 10kHz, 16-bit, LPF, DMA 使能
     */
    SensorConfig* (*build_high_speed_config)(struct SensorDirector *director);
    
    /**
     * 构建低功耗配置（用于电池供电场景）
     * 采样率 100Hz, 8-bit, 禁用 DMA 和中断
     */
    SensorConfig* (*build_low_power_config)(struct SensorDirector *director);
    
    /**
     * 构建高精度配置（用于静态测量）
     * 采样率 1kHz, 24-bit, LPF, 校准使能
     */
    SensorConfig* (*build_high_precision_config)(struct SensorDirector *director);
    
    void (*destroy)(struct SensorDirector *director);
} SensorDirector;

/**
 * 创建传感器指挥者
 */
SensorDirector* SensorDirector_create(void);

#endif // SENSOR_DIRECTOR_H
```

```c
// sensor_director.c
#include "sensor_director.h"
#include "sensor_builder_impl.h"
#include <stdio.h>
#include <stdlib.h>

/* 前向声明 */
typedef struct SensorDirectorImpl SensorDirectorImpl;

struct SensorDirectorImpl {
    SensorDirector base;
    SensorBuilderImpl *builder;  /* 持有建造者 */
};

/* 内部函数 */
static SensorConfig* build_high_speed(SensorDirector *director);
static SensorConfig* build_low_power(SensorDirector *director);
static SensorConfig* build_high_precision(SensorDirector *director);
static void director_destroy(SensorDirector *director);

SensorDirector* SensorDirector_create(void) {
    SensorDirectorImpl *impl = malloc(sizeof(SensorDirectorImpl));
    if (!impl) {
        fprintf(stderr, "[Director] 分配内存失败\n");
        return NULL;
    }
    
    impl->builder = SensorBuilderImpl_create();
    
    impl->base.build_high_speed_config = build_high_speed;
    impl->base.build_low_power_config = build_low_power;
    impl->base.build_high_precision_config = build_high_precision;
    impl->base.destroy = director_destroy;
    
    printf("[Director] 传感器配置指挥者已创建\n");
    return &impl->base;
}

static SensorConfig* build_high_speed(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    SensorBuilderImpl *b = impl->builder;
    ISensorBuilder *builder = &b->base;
    
    printf("\n[Director] 构建高速采样配置...\n");
    b->base.reset(builder);
    
    /* 高速配置：10kHz, 16-bit, LPF, DMA */
    SensorConfig *config = builder
        ->set_sampling_rate(builder, 10000)
        ->set_filter(builder, "LPF")
        ->set_data_format(builder, 16)
        ->set_resolution(builder, 16)
        ->enable_dma(builder)
        ->build(builder);
    
    return config;
}

static SensorConfig* build_low_power(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    SensorBuilderImpl *b = impl->builder;
    ISensorBuilder *builder = &b->base;
    
    printf("\n[Director] 构建低功耗配置...\n");
    b->base.reset(builder);
    
    /* 低功耗配置：100Hz, 8-bit, 无 DMA/中断 */
    SensorConfig *config = builder
        ->set_sampling_rate(builder, 100)
        ->set_data_format(builder, 8)
        ->set_resolution(builder, 8)
        ->build(builder);
    
    return config;
}

static SensorConfig* build_high_precision(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    SensorBuilderImpl *b = impl->builder;
    ISensorBuilder *builder = &b->base;
    
    printf("\n[Director] 构建高精度配置...\n");
    b->base.reset(builder);
    
    /* 高精度配置：1kHz, 24-bit, LPF, 校准 */
    SensorConfig *config = builder
        ->set_sampling_rate(builder, 1000)
        ->set_filter(builder, "LPF")
        ->set_data_format(builder, 24)
        ->set_resolution(builder, 24)
        ->enable_calibration(builder)
        ->build(builder);
    
    return config;
}

static void director_destroy(SensorDirector *director) {
    SensorDirectorImpl *impl = (SensorDirectorImpl*)director;
    printf("[Director] 销毁指挥者\n");
    free(impl);
}
```

### 4.5 主程序

```c
// main.c
#include "sensor_director.h"
#include <stdio.h>

int main(void) {
    printf("========================================\n");
    printf("   建造者模式 - 嵌入式传感器配置\n");
    printf("========================================\n\n");
    
    /* ========================================
     * 方式1：直接使用建造者
     * ======================================== */
    printf("=== 方式1：直接使用建造者（自定义配置）===\n");
    SensorBuilderImpl *custom_builder = SensorBuilderImpl_create();
    ISensorBuilder *builder = &custom_builder->base;
    
    SensorConfig *custom_config = builder
        ->set_sampling_rate(builder, 5000)
        ->set_filter(builder, "HPF")
        ->set_data_format(builder, 12)
        ->set_resolution(builder, 12)
        ->enable_interrupt(builder)
        ->enable_calibration(builder)
        ->build(builder);
    
    printf("\n自定义配置采样率: %u Hz\n\n", custom_config->sampling_rate_hz);
    
    /* ========================================
     * 方式2：使用指挥者（标准配置）
     * ======================================== */
    printf("=== 方式2：使用指挥者（标准配置序列）===\n");
    SensorDirector *director = SensorDirector_create();
    
    printf("\n--- 高速采样配置 ---");
    SensorConfig *hs_config = director->build_high_speed_config(director);
    (void)hs_config;
    
    printf("\n--- 低功耗配置 ---");
    SensorConfig *lp_config = director->build_low_power_config(director);
    (void)lp_config;
    
    printf("\n--- 高精度配置 ---");
    SensorConfig *hp_config = director->build_high_precision_config(director);
    (void)hp_config;
    
    printf("\n========================================\n");
    printf("   总结：\n");
    printf("   - 建造者：灵活自定义，每步有名称\n");
    printf("   - 指挥者：标准流程，可复用\n");
    printf("   - 链式调用：代码简洁，可读性高 ✓\n");
    printf("========================================\n");
    
    /* 清理 */
    director->destroy(director);
    
    return 0;
}
```

---

## 5. 嵌入式固件初始化序列的建造者应用

在嵌入式固件中，初始化序列天然适合建造者模式：

```c
// 嵌入式固件外设初始化示例
typedef struct {
    void (*enable_clock)(void);
    void (*configure_gpio)(uint16_t pins);
    void (*setup_interrupts)(uint8_t priority);
    void (*init_dma)(void);
    void (*calibrate)(void);
} PeripheralInitBuilder;

// 链式初始化序列
init_builder()
    ->enable_clock()
    ->configure_gpio(GPIO_PIN_0 | GPIO_PIN_1)
    ->setup_interrupts(IRQ_PRIORITY_HIGH)
    ->init_dma()
    ->calibrate()
    ->build();
```

---

## 6. 建造者模式的适用场景

**应该用：**
- 构造对象需要多个步骤，每步有特定的参数
- 参数有默认值，不是所有都需要显式设置
- 对象构造过程需要灵活组合（可选步骤）
- 需要创建不同表示的同一构造过程

**不应该用：**
- 对象构造简单，参数少（直接构造函数更直接）
- 对象不可变，所有参数必须一次性提供（用函数式参数更合适）
- 构造过程没有变化（不需要解耦构造和使用）

---

## 7. 本章小结

| 要素 | C 语言实现 |
|:---|:---|
| 抽象 Builder | `struct ISensorBuilder { func* method(); ISensorBuilder* (*method)(); }` |
| 具体 Builder | 实现各方法 + 保存中间状态 + `build()` 返回最终产品 |
| 链式调用 | `ISensorBuilder* (*method)()` 返回 `this`，支持链式 `a()->b()->c()` |
| Director | 持有 Builder，定义标准构建流程 |

**建造者模式的核心**：把"复杂对象的分步构造"封装起来，让构造代码可读、可复用。

---

## 8. 练习

1. 实现一个"网络请求"建造者，支持设置 URL、Method、Header、Body、Timeout 等参数

2. 实现一个"嵌入式 PWM 配置"建造者，支持设置频率、占空比、通道、极性等

3. 在上面的传感器建造者中添加"验证"步骤：`validate()` 方法检查采样率和数据格式的兼容性，如果冲突返回错误

4. （思考题）建造者模式和工厂方法模式都能创建对象，它们的核心区别是什么？什么场景用哪个？

---

## 代码

`include/sensor_config.h` — 产品（传感器配置）
`include/sensor_builder.h` — 抽象建造者接口
`include/sensor_builder_impl.h` + `src/sensor_builder_impl.c` — 具体建造者实现
`include/sensor_director.h` + `src/sensor_director.c` — 指挥者
`src/main.c` — 测试主程序
`Makefile` — 编译脚本