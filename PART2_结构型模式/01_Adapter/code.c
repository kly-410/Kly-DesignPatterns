/*
 * 01_Adapter - 适配器模式
 * 
 * 适配器模式核心：将"不兼容"的接口包装成"兼容"的接口
 * 
 * 本代码演示：
 * 1. 对象适配器：通过组合持有被适配对象
 * 2. USB/PCIe 转换层的实际应用
 * 3. 类适配器 vs 对象适配器对比
 * 
 * 适配器 = 转换器，就像电源插头转换器
 * 让原本接口不兼容的类可以合作无间
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// 目标接口：系统期望的统一IO接口
// ============================================================
typedef struct _IOHandler IOHandler;
struct _IOHandler {
    void (*send)(IOHandler*, const void* data, size_t len);
    void (*recv)(IOHandler*, void* buf, size_t len);
    void (*close)(IOHandler*);
};

// ============================================================
// 被适配者：已有的 PCIe 设备接口（老接口）
// ============================================================
typedef struct _PCIeDevice PCIeDevice;
struct _PCIeDevice {
    char name[32];
    int bus, dev, func;
    void (*write_reg)(PCIeDevice*, int offset, int value);
    int  (*read_reg)(PCIeDevice*, int offset);
};

// PCIe 设备寄存器写入（模拟）
static void pcie_write_reg(PCIeDevice* dev, int offset, int value)
{
    printf("  [PCIe %s] WRITE reg@0x%02X <- 0x%X\n", 
           dev->name, offset, value);
}

// PCIe 设备寄存器读取（模拟）
static int pcie_read_reg(PCIeDevice* dev, int offset)
{
    printf("  [PCIe %s] READ  reg@0x%02X -> 0xDEAD\n", dev->name, offset);
    return 0xDEAD;
}

// PCIe 设备构造函数
PCIeDevice* new_PCIeDevice(const char* name)
{
    PCIeDevice* dev = calloc(1, sizeof(PCIeDevice));
    strncpy(dev->name, name, 31);
    dev->write_reg = pcie_write_reg;
    dev->read_reg  = pcie_read_reg;
    return dev;
}

// ============================================================
// 适配器：把 PCIe 接口适配成 IOHandler（对象适配器）
// 使用组合：适配器持有被适配对象的指针
// ============================================================
typedef struct _PCIeToUSBAdapter {
    IOHandler base;          // 实现目标接口
    PCIeDevice* pcie_dev;    // 持有被适配对象（组合）
    char tx_buffer[64];
    char rx_buffer[64];
    int tx_len;
    int rx_len;
} PCIeToUSBAdapter;

// 适配器写入：把 IOHandler 的 send 适配成 PCIe 的 write_reg
static void adapter_send(IOHandler* h, const void* data, size_t len)
{
    PCIeToUSBAdapter* ad = (PCIeToUSBAdapter*)h;
    
    // 累积到缓冲区（这里简化，实际会做协议封装）
    int copy_len = (len < 64 - ad->tx_len) ? len : 64 - ad->tx_len;
    memcpy(ad->tx_buffer + ad->tx_len, data, copy_len);
    ad->tx_len += copy_len;
    
    printf("[Adapter] PCIe->USB send: %zu bytes via %s\n", len, ad->pcie_dev->name);
    
    // 模拟 USB 转 PCIe：把第一个字写入 BAR 偏移 0x10
    int val = *(int*)data;
    ad->pcie_dev->write_reg(ad->pcie_dev, 0x10, val);
}

// 适配器读取：把 PCIe 的 read_reg 适配成 IOHandler 的 recv
static void adapter_recv(IOHandler* h, void* buf, size_t len)
{
    PCIeToUSBAdapter* ad = (PCIeToUSBAdapter*)h;
    
    int val = ad->pcie_dev->read_reg(ad->pcie_dev, 0x14);
    memcpy(buf, &val, len < 4 ? len : 4);
    
    printf("[Adapter] PCIe->USB recv: read reg 0x14 = 0x%X\n", val);
}

static void adapter_close(IOHandler* h)
{
    PCIeToUSBAdapter* ad = (PCIeToUSBAdapter*)h;
    free(ad->pcie_dev);
    free(ad);
    printf("[Adapter] PCIe->USB adapter closed\n");
}

// 适配器构造函数
IOHandler* new_PCIeToUSBAdapter(PCIeDevice* pcie_dev)
{
    PCIeToUSBAdapter* ad = calloc(1, sizeof(PCIeToUSBAdapter));
    ad->base.send = adapter_send;
    ad->base.recv = adapter_recv;
    ad->base.close = adapter_close;
    ad->pcie_dev = pcie_dev;
    return (IOHandler*)ad;
}

// ============================================================
// 示例2：类适配器 vs 对象适配器
// ============================================================
typedef struct _LegacySerialDevice LegacySerialDevice;
struct _LegacySerialDevice {
    void (*send_byte)(LegacySerialDevice*, char c);
    char (*recv_byte)(LegacySerialDevice*);
};

// 老式串口设备
static void serial_send_byte(LegacySerialDevice* dev, char c)
{
    printf("  [Legacy Serial] TX byte: 0x%02X ('%c')\n", c, c);
}

static char serial_recv_byte(LegacySerialDevice* dev)
{
    printf("  [Legacy Serial] RX byte: waiting...\n");
    return 'K';
}

LegacySerialDevice* new_LegacySerialDevice(void)
{
    LegacySerialDevice* dev = calloc(1, sizeof(LegacySerialDevice));
    dev->send_byte = serial_send_byte;
    dev->recv_byte = serial_recv_byte;
    return dev;
}

// 类适配器：多重结构体嵌入（模拟多重继承）
typedef struct _SerialToIOAdapter {
    IOHandler base;                    // 实现目标接口
    LegacySerialDevice serial;         // "继承" LegacySerialDevice（嵌入）
} SerialToIOAdapter;

// 适配器 send：把 IOHandler 的 send 适配成 LegacySerial 的 send_byte
static void serial_adapter_send(IOHandler* h, const void* data, size_t len)
{
    SerialToIOAdapter* ad = (SerialToIOAdapter*)h;
    const char* str = (const char*)data;
    
    printf("[SerialAdapter] Sending %zu bytes via legacy serial:\n", len);
    for (size_t i = 0; i < len; i++) {
        // 逐字节发送
        ad->serial.send_byte(&ad->serial, str[i]);
    }
}

static void serial_adapter_recv(IOHandler* h, void* buf, size_t len)
{
    SerialToIOAdapter* ad = (SerialToIOAdapter*)h;
    *(char*)buf = ad->serial.recv_byte(&ad->serial);
}

static void serial_adapter_close(IOHandler* h)
{
    free(h);
    printf("[SerialAdapter] closed\n");
}

// 类适配器构造函数
IOHandler* new_SerialToIOAdapter(void)
{
    SerialToIOAdapter* ad = calloc(1, sizeof(SerialToIOAdapter));
    ad->base.send = serial_adapter_send;
    ad->base.recv = serial_adapter_recv;
    ad->base.close = serial_adapter_close;
    ad->serial.send_byte = serial_send_byte;
    ad->serial.recv_byte = serial_recv_byte;
    return (IOHandler*)ad;
}

// ============================================================
// 客户端代码：只认识 IOHandler，完全不知道底层是什么设备
// ============================================================
void client_send_message(IOHandler* handler, const char* msg)
{
    handler->send(handler, msg, strlen(msg));
}

void client_recv_message(IOHandler* handler, void* buf, size_t len)
{
    handler->recv(handler, buf, len);
}

// ============================================================
int main()
{
    printf("========== 适配器模式演示 ==========\n\n");

    // --------------------------------------------------
    // 示例1：对象适配器 - PCIe 转 USB
    // --------------------------------------------------
    printf("--- 示例1: 对象适配器 (PCIe -> USB) ---\n");
    
    // 创建真实的 PCIe 设备
    PCIeDevice* pcie_dev = new_PCIeDevice("0000:01:00.0");
    
    // 用适配器把 PCIe 设备转换成 USB 兼容的 IOHandler
    IOHandler* usb_handler = new_PCIeToUSBAdapter(pcie_dev);
    
    // 客户端代码完全不感知底层是 PCIe
    client_send_message(usb_handler, "Hi USB!");
    char buf[8];
    client_recv_message(usb_handler, buf, 4);
    
    printf("\n");
    usb_handler->close(usb_handler);
    
    // --------------------------------------------------
    // 示例2：类适配器 - 老式串口适配成 IOHandler
    // --------------------------------------------------
    printf("\n--- 示例2: 类适配器 (Legacy Serial -> IOHandler) ---\n");
    
    IOHandler* serial_handler = new_SerialToIOAdapter();
    client_send_message(serial_handler, "Hello!");
    char recv_buf[8];
    client_recv_message(serial_handler, recv_buf, 1);
    serial_handler->close(serial_handler);
    
    // --------------------------------------------------
    // 示例3：同一个适配器，支持多个不同的被适配对象
    // --------------------------------------------------
    printf("\n--- 示例3: 同一个适配器，不同被适配对象 ---\n");
    
    PCIeDevice* pcie_dev2 = new_PCIeDevice("0000:02:00.0");
    IOHandler* usb_handler2 = new_PCIeToUSBAdapter(pcie_dev2);
    client_send_message(usb_handler2, "Another device!");
    usb_handler2->close(usb_handler2);
    
    printf("\n========== 演示结束 ==========\n");
    return 0;
}
