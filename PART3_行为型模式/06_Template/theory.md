# Template Method Pattern — 模板方法模式

## 1. 模式定义

**模板方法模式（Template Method Pattern）**：在一个方法中定义一个算法的骨架，将某些步骤延迟到子类中实现。

> 核心思想：**算法骨架不变，具体步骤可定制**。

---

## 2. 《Head First 设计模式》核心内容

```cpp
class CaffeineBeverage {
    void prepareRecipe() {
        boilWater();
        brew();
        pourInCup();
        addCondiments();
    }
    void boilWater() { cout << "Boiling water\n"; }
    void pourInCup() { cout << "Pouring into cup\n"; }
    virtual void brew() = 0;
    virtual void addCondiments() = 0;
};
```

> **好莱坞原则**：Don't call us, we'll call you.

---

## 3. Linux 内核实例

### PCIe 驱动 probe 模板

```c
static int my_pci_driver_probe(struct pci_dev* pdev) {
    if (pci_enable_device(pdev)) return -ENODEV;
    if (pci_request_regions(pdev, DRV_NAME)) { pci_disable_device(pdev); return -EBUSY; }
    void* bar0 = pci_iomap(pdev, 0, 0);
    if (!bar0) { pci_release_regions(pdev); pci_disable_device(pdev); return -ENOMEM; }
    if (request_irq(irq, handler, IRQF_SHARED, DRV_NAME, drv_data)) { ... }
    return 0;
}
```

---

## 4. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 定义算法骨架，某些步骤延迟到子类实现 |
| **核心** | 模板方法 = 算法骨架 + 抽象步骤函数指针 |
| **Linux 场景** | PCIe probe、固件初始化序列 |
