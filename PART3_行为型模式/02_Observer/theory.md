# Observer Pattern — 观察者模式

## 1. 模式定义

**观察者模式（Observer Pattern）**：定义对象间的一种一对多依赖关系，当一个对象（Subject，被观察者）状态改变时，所有依赖它的对象（Observer，观察者）都会自动收到通知并更新。

> 核心思想：**发布（Publish）— 订阅（Subscribe）** 机制。Subject 不知道具体的 Observer 是谁，只知道它们实现了统一的更新接口。

---

## 2. 《Head First 设计模式》核心内容

### 问题背景：气象站案例

一个气象站应用，需要：
- `WeatherData` 追踪温度、湿度、气压
- 当数据变化时，自动更新三个布告板（当前状况、统计预报、预测预报）

### 核心原则

> **交互对象之间应尽量松耦合（Loose Coupling）**

### 类结构

```
Subject（WeatherData）
  - registerObserver()
  - removeObserver()
  - notifyObservers()

Observer（接口）
  + update(temperature, humidity, pressure)
```

---

## 3. Linux 内核实例

### 3.1 Notifier Chain（通知链）

Linux 内核提供 `notifier_call_chain` 机制，用于事件通知：

```c
static struct notifier_block my_notifier = {
    .notifier_call = my_event_notifier,
};
register_netdevice_notifier(&my_notifier);
```

### 3.2 中断回调注册表

PCIe 驱动通过 `request_irq()` 注册中断处理函数，本质也是观察者模式。

---

## 4. C 语言实现思路

```c
typedef struct _IObserver {
    void (*update)(void* self, float temp, float humidity, float pressure);
    void* instance;
} IObserver;

typedef struct _WeatherData {
    IObserver** observers;
    int count;
    void (*registerObserver)(struct _WeatherData*, IObserver*);
    void (*notifyObservers)(struct _WeatherData*);
} WeatherData;
```

---

## 5. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 定义对象间一对多的依赖关系 |
| **核心** | 松耦合 — Subject 和 Observer 互不知道对方细节 |
| **实现** | 函数指针列表 + 动态注册/注销 + 遍历通知 |
| **Linux 场景** | Notifier Chain、IRQ Handler、uevent |
