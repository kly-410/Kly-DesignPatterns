# 01_Adapter — 适配器模式

## 问题引入：接口不兼容？

你手里有一个第三方库，提供的是"老式"接口：
```c
void legacy_write(int fd, char* buf, int len);  // 旧接口：fd + buf
```

但你的系统已经演进到"新式"接口：
```c
typedef struct _IODevice IODevice;
struct _IODevice {
    void (*write)(IODevice*, const void* buf, size_t len);
};
```

**问题**：如何让旧库被新系统直接使用，而不修改旧库代码？

---

## 适配器模式核心思想

> **把"不兼容"的接口，包装成"兼容"的接口。**

适配器就像电源插头转换器——欧洲插头插不进中国插座，加个转换头就解决了。

### 适配器模式分类

| 类型 | 实现方式 | C语言模拟 |
|:---|:---|:---|
| **类适配器** | 继承（多重继承） | 多重结构体嵌入 |
| **对象适配器** | 组合（持有被适配者） | 结构体持有被适配对象指针 |

---

## 模式定义

**Adapter Pattern**：将一个类的接口转换成客户期望的另一个接口。适配器让原本接口不兼容的类可以合作无间。

**类图（对象适配器）**：
```
┌─────────────┐     ┌──────────────────┐     ┌─────────────┐
│   Client   │────>│     Target       │     │  Adaptee   │
│  (使用者)   │     │  (期望的接口)     │     │ (已有的接口)│
└─────────────┘     └────────┬─────────┘     └──────┬──────┘
                             │                       │
                             │    ┌──────────────┐  │
                             └───>│   Adapter    │◄─┘
                                  │  (转换器)      │
                                  └───────────────┘
```

---

## C语言实现：对象适配器

### 场景：USB ↔ PCIe 转换层

```c
// ============================================================
// 场景：嵌入式系统中，USB 控制器驱动需要兼容 PCIe 协议栈
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------- 被适配者：已有的 PCIe 设备接口 ---------
typedef struct _PCIeDevice PCIeDevice;
struct _PCIeDevice {
    char name[32];
    void (*write_reg)(PCIeDevice*, int offset, int value);
    int  (*read_reg)(PCIeDevice*, int offset);
};

// --------- 目标接口：系统期望的统一IO接口 ---------
typedef struct _IOHandler IOHandler;
struct _IOHandler {
    void (*send)(IOHandler*, const void* data, size_t len);
    void (*recv)(IOHandler*, void* buf, size_t len);
};

// --------- 适配器：把 PCIe 接口适配成 IOHandler ---------
typedef struct _PCIeToUSBAdapter {
    PCIeDevice* dev;           // 持有被适配对象（组合）
    char        tx_buffer[64];
    char        rx_buffer[64];
} PCIeToUSBAdapter;

static void adapter_send(IOHandler* h, const void* data, size_t len)
{
    PCIeToUSBAdapter* ad = (PCIeToUSBAdapter*)h;
    memcpy(ad->tx_buffer, data, len < 64 ? len : 64);
    printf("[Adapter] PCIe->USB send: %d bytes via %s\n", 
           (int)len, ad->dev->name);
    ad->dev->write_reg(ad->dev, 0x10, *(int*)data);
}

static void adapter_recv(IOHandler* h, void* buf, size_t len)
{
    PCIeToUSBAdapter* ad = (PCIeToUSBAdapter*)h;
    int val = ad->dev->read_reg(ad->dev, 0x14);
    memcpy(buf, &val, len < 4 ? len : 4);
    printf("[Adapter] PCIe->USB recv: read reg 0x14 = 0x%X\n", val);
}

IOHandler* new_PCIeToUSBAdapter(PCIeDevice* dev)
{
    PCIeToUSBAdapter* ad = calloc(1, sizeof(PCIeToUSBAdapter));
    ad->dev = dev;
    ad->send = adapter_send;
    ad->recv = adapter_recv;
    return (IOHandler*)ad;
}

// --------- 模拟 PCIe 设备（被适配者） ---------
void fake_write_reg(PCIeDevice* dev, int offset, int value)
{
    printf("  [PCIe Device %s] WRITE reg@0x%02X <- 0x%X\n", 
           dev->name, offset, value);
}

int fake_read_reg(PCIeDevice* dev, int offset)
{
    printf("  [PCIe Device %s] READ  reg@0x%02X -> 0xDEAD\n", 
           dev->name, offset);
    return 0xDEAD;
}

PCIeDevice* new_PCIeDevice(const char* name)
{
    PCIeDevice* dev = calloc(1, sizeof(PCIeDevice));
    strcpy(dev->name, name);
    dev->write_reg = fake_write_reg;
    dev->read_reg  = fake_read_reg;
    return dev;
}

// --------- 客户端代码：只认识 IOHandler ---------
void client_use_io_handler(IOHandler* h)
{
    char data[] = "Hello USB!";
    h->send(h, data, strlen(data));
    char buf[8];
    h->recv(h, buf, 4);
}

// ============================================================
int main()
{
    printf("========== 适配器模式演示 ==========\n\n");

    // 创建真实的 PCIe 设备
    PCIeDevice* pcie_dev = new_PCIeDevice("PCIe-1");
    
    // 用适配器把 PCIe 设备转换成 USB 兼容的 IOHandler
    IOHandler* usb_handler = new_PCIeToUSBAdapter(pcie_dev);
    
    // 客户端代码完全不感知底层是 PCIe
    client_use_io_handler(usb_handler);
    
    // 换一个不同的被适配者，同样的适配器逻辑照样工作
    printf("\n--- 换另一个 PCIe 设备 ---\n");
    PCIeDevice* pcie_dev2 = new_PCIeDevice("PCIe-2");
    IOHandler* usb_handler2 = new_PCIeToUSBAdapter(pcie_dev2);
    client_use_io_handler(usb_handler2);
    
    free(pcie_dev);
    free(pcie_dev2);
    free(usb_handler);
    free(usb_handler2);
    
    return 0;
}
```

**输出：**
```
========== 适配器模式演示 ==========

[Adapter] PCIe->USB send: 11 bytes via PCIe-1
  [PCIe Device PCIe-1] WRITE reg@0x10 <- 0x6C6C6548
[Adapter] PCIe->USB recv: read reg 0x14 = 0xDEAD
  [PCIe Device PCIe-1] READ  reg@0x14 -> 0xDEAD

--- 换另一个 PCIe 设备 ---

[Adapter] PCIe->USB send: 11 bytes via PCIe-2
  [PCIe Device PCIe-2] WRITE reg@0x10 <- 0x6C6C6548
[Adapter] PCIe->USB recv: read reg 0x14 = 0xDEAD
  [PCIe Device PCIe-2] READ  reg@0x14 -> 0xDEAD
```

---

## Linux 内核实例：platform_driver 适配 PCIe

Linux 内核通过适配器思想，统一了不同总线的设备驱动模型。

```c
// 内核中 platform_device 和 pci_device 共用一套驱动接口
// 驱动作者只需要实现 platform_driver，
// 通过内核适配层自动对接具体硬件总线

static int my_probe(struct platform_device* pdev)
{
    // 统一的 driver 逻辑，完全不知道是 PCIe 还是 platform
    struct resource* res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    // ...
    return 0;
}

static struct platform_driver my_driver = {
    .probe  = my_probe,
    .driver = {
        .name  = "my-device",
        .owner = THIS_MODULE,
        // 通过.of_match_table 适配不同硬件
        .of_match_table = my_of_matches,
    },
};
// 内核在 PCI 总线上注册时会自动把 pci_device 适配成 platform_device
// 驱动作者完全不用改代码
```

---

## 类适配器 vs 对象适配器

```c
// ========== 类适配器（多重继承） ==========
// C 语言通过结构体嵌入模拟"继承"
typedef struct _LegacyUSBDevice {
    PCIeDevice base;       // "继承" PCIeDevice（通过嵌入）
    // 额外状态
} LegacyUSBDevice;

// ========== 对象适配器（组合） ==========
typedef struct _PCIeToUSBAdapter {
    PCIeDevice* dev;       // "组合" 持有 PCIeDevice 指针
    // 额外状态
} PCIeToUSBAdapter;
// 推荐用对象适配器，更灵活，符合"组合优先于继承"
```

---

## 练习

1. **基础练习**：把一个 `SerialDevice`（有 `send_byte`/`recv_byte` 接口）适配成 `IOHandler`（有 `send`/`recv` 接口）。

2. **进阶练习**：实现双向适配器——既可以把 A 接口适配成 B，也可以把 B 接口适配成 A。

3. **内核练习**：阅读 Linux 内核 `drivers/pci/pci-driver.c`，找找适配器的影子。

---

## 关键收获

- **适配器解决的是"已有代码"和"新需求"接口不匹配的问题**
- **对象适配器**用组合，更灵活；**类适配器**用继承，C语言通过结构体嵌入模拟
- **Linux 内核**：platform 总线适配 PCI总线、I2C适配SPI，各种驱动总线都靠适配器思想统一
