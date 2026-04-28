# Mediator Pattern — 中介者模式

## 1. 模式定义

**中介者模式（Mediator Pattern）**：用一个中介对象来封装一系列的对象交互。中介者使各对象不需要显式地相互引用。

> 核心思想：**对象间通信集中化**。

---

## 2. 《Head First 设计模式》核心内容

```cpp
class Mediator {
    Light* light;
    AC* ac;
public:
    void notify(Colleague* sender, String event) override {
        if (sender == light && event == "ON") {
            ac->setTemperature(26);
        }
    }
};
```

---

## 3. Linux 内核实例

PCIe Core 协调 RC/EP/Switch，TCP 作为应用层和 IP 层的中介。

---

## 4. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 将对象间网状通信集中为星形通信 |
| **核心** | Mediator 封装对象间交互逻辑 |
| **Linux 场景** | PCIe Core、I2C 总线仲裁、TCP 协议栈 |
