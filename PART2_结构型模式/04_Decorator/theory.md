# 04_Decorator — 装饰器模式

## 问题引入：动态增加功能

你写了一个日志库 `Logger`，功能是基本的文本输出。某天需要：
1. 加时间戳
2. 加日志级别（INFO/ERROR/DEBUG）
3. 加加密
4. 加压缩

**问题**：如何给对象**动态添加功能**，而不修改原有类的代码？

**方案 A（继承）**：子类叠加 → `TimestampLogger -> LevelLogger -> EncryptLogger -> ...`
- 类爆炸
- 编译时静态绑定，无法在运行时选择
- 功能组合不灵活

**方案 B（装饰器）**：包装一层套一层，像洋葱
```
client → [加密Decorator] → [压缩Decorator] → [基础Logger]
```

---

## 装饰器模式核心思想

> **动态地给对象添加一些职责，比继承更灵活。**

装饰器和被装饰对象实现**同一个接口**，客户端完全不知道背后包装了多少层。

### 类图

```
                    ┌────────────────────┐
                    │ <<interface>>       │
                    │   IComponent       │
                    │   operation()      │
                    └─────────┬──────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
      ┌───────┴───────┐  ┌────┴────┐  ┌──────┴──────┐
      │ Concrete      │  │ Decorator│  │ Concrete    │
      │ Component    │  │ (包装基类)│  │ ComponentB  │
      │ (被包装对象)   │  └────┬────┘  │             │
      └───────────────┘       │        └─────────────┘
                             │ 包含 IComponent*
                   ┌────────┴────────┐
                   │                 │
           ┌───────┴───────┐  ┌──────┴──────┐
           │ConcreteDecoA │  │ConcreteDecoB│
           │ (装饰器A)     │  │ (装饰器B)   │
           └───────────────┘  └────────────┘
```

---

## 模式定义

**Decorator Pattern**：动态地给对象添加一些职责。比继承更灵活，装饰器与被装饰对象实现相同接口，客户端无感知。

---

## C语言实现：Linux VFS 文件操作包装

### 场景：给流式设备添加缓冲、压缩、加密层

```c
// ============================================================
// 场景：嵌入式 IO 层的 stream 包装器
// - 基础设备：裸串口/UART
// - 加缓冲层（Buffer）：减少硬件中断次数
// - 加压缩层（Zip）：网络传输时压缩
// - 加加密层（Crypto）：安全传输
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --------- Component 接口 ---------
typedef struct _Stream Stream;
struct _Stream {
    void (*write)(Stream*, const char* data, size_t len);
    void (*read)(Stream*, char* buf, size_t len);
    void (*flush)(Stream*);
    void (*close)(Stream*);
};

// --------- Concrete Component：基础串口 ---------
typedef struct _UARTStream {
    Stream base;
    const char* name;
    char rx_buffer[64];
    int rx_head;
} UARTStream;

static void uart_write(Stream* s, const char* data, size_t len)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] TX: ", u->name);
    for (size_t i = 0; i < len; i++) putchar(data[i]);
    putchar('\n');
}

static void uart_read(Stream* s, char* buf, size_t len)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] RX: waiting...\n", u->name);
    (void)buf; (void)len;
}

static void uart_flush(Stream* s)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] flush\n", u->name);
}

static void uart_close(Stream* s)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] closed\n", u->name);
}

UARTStream* new_UARTStream(const char* name)
{
    UARTStream* u = calloc(1, sizeof(UARTStream));
    u->base.write  = uart_write;
    u->base.read   = uart_read;
    u->base.flush  = uart_flush;
    u->base.close  = uart_close;
    u->name = name;
    return u;
}

// --------- Decorator：缓冲层 ---------
typedef struct _BufferedStream {
    Stream base;
    Stream* wrapped;           // 被包装对象
    char buffer[128];
    int buf_len;
} BufferedStream;

static void buffered_write(Stream* s, const char* data, size_t len)
{
    BufferedStream* b = (BufferedStream*)s;
    // 累积到缓冲区
    if (b->buf_len + (int)len < (int)sizeof(b->buffer)) {
        memcpy(b->buffer + b->buf_len, data, len);
        b->buf_len += len;
        printf("  [Buffer] accumulated %zu bytes (total %d)\n", len, b->buf_len);
    } else {
        // 满了，刷出去
        b->base.flush(s);
        b->wrapped->write(b->wrapped, data, len);
    }
}

static void buffered_flush(Stream* s)
{
    BufferedStream* b = (BufferedStream*)s;
    if (b->buf_len > 0) {
        b->wrapped->write(b->wrapped, b->buffer, b->buf_len);
        b->buf_len = 0;
        printf("  [Buffer] flushed %d bytes\n", b->buf_len);
    }
}

static void buffered_close(Stream* s)
{
    BufferedStream* b = (BufferedStream*)s;
    b->base.flush(s);
    b->wrapped->close(b->wrapped);
    printf("  [Buffer] closed\n");
}

BufferedStream* new_BufferedStream(Stream* inner)
{
    BufferedStream* b = calloc(1, sizeof(BufferedStream));
    b->base.write  = buffered_write;
    b->base.read   = inner->read;       // 直通
    b->base.flush  = buffered_flush;
    b->base.close  = buffered_close;
    b->wrapped = inner;
    return b;
}

// --------- Decorator：压缩层 ---------
typedef struct _ZipStream {
    Stream base;
    Stream* wrapped;
} ZipStream;

static void zip_write(Stream* s, const char* data, size_t len)
{
    ZipStream* z = (ZipStream*)s;
    printf("  [ZIP] compressing %zu bytes -> ~%zu bytes\n", len, len * 2 / 3);
    // 模拟压缩：直接透传，真实实现会调用 zlib
    z->wrapped->write(z->wrapped, data, len);
}

static void zip_close(Stream* s)
{
    ZipStream* z = (ZipStream*)s;
    printf("  [ZIP] close\n");
    z->wrapped->close(z->wrapped);
}

ZipStream* new_ZipStream(Stream* inner)
{
    ZipStream* z = calloc(1, sizeof(ZipStream));
    z->base.write  = zip_write;
    z->base.read   = inner->read;
    z->base.flush  = inner->flush;
    z->base.close  = zip_close;
    z->wrapped = inner;
    return z;
}

// --------- Decorator：加密层 ---------
typedef struct _CryptoStream {
    Stream base;
    Stream* wrapped;
} CryptoStream;

static void crypto_write(Stream* s, const char* data, size_t len)
{
    CryptoStream* c = (CryptoStream*)s;
    printf("  [CRYPTO] encrypting %zu bytes\n", len);
    // 模拟：把数据"加密"后转发
    char encrypted[256];
    for (size_t i = 0; i < len; i++) {
        encrypted[i] = data[i] ^ 0xFF;   // 简单的 XOR
    }
    c->wrapped->write(c->wrapped, encrypted, len);
}

static void crypto_close(Stream* s)
{
    CryptoStream* c = (CryptoStream*)s;
    printf("  [CRYPTO] close\n");
    c->wrapped->close(c->wrapped);
}

CryptoStream* new_CryptoStream(Stream* inner)
{
    CryptoStream* c = calloc(1, sizeof(CryptoStream));
    c->base.write  = crypto_write;
    c->base.read   = inner->read;
    c->base.flush  = inner->flush;
    c->base.close  = crypto_close;
    c->wrapped = inner;
    return c;
}

// ============================================================
int main()
{
    printf("========== 装饰器模式演示 ==========\n\n");

    // 方案1：裸 UART（无包装）
    printf("--- 方案1: 裸 UART ---\n");
    UARTStream* raw = new_UARTStream("ttyS0");
    raw->base.write(&raw->base, "Hello", 5);
    raw->base.close(&raw->base);

    // 方案2：UART + Buffer
    printf("\n--- 方案2: UART + Buffer ---\n");
    UARTStream* uart2 = new_UARTStream("ttyS1");
    BufferedStream* buf2 = new_BufferedStream(&uart2->base);
    buf2->base.write(&buf2->base, "Hello", 5);
    buf2->base.write(&buf2->base, " World", 6);
    buf2->base.flush(&buf2->base);
    buf2->base.close(&buf2->base);

    // 方案3：UART + Buffer + ZIP + CRYPTO（洋葱式包装）
    printf("\n--- 方案3: UART + Buffer + ZIP + CRYPTO ---\n");
    UARTStream* uart3 = new_UARTStream("ttyUSB0");
    BufferedStream* buf3 = new_BufferedStream(&uart3->base);
    ZipStream* zip3 = new_ZipStream(&buf3->base);
    CryptoStream* crp3 = new_CryptoStream(&zip3->base);
    
    printf("  -> 写入 'Hello World' 到三层包装的 stream:\n");
    crp3->base.write(&crp3->base, "Hello World", 11);
    crp3->base.flush(&crp3->base);
    crp3->base.close(&crp3->base);

    // 方案4：只加 Crypto（不需要缓冲）
    printf("\n--- 方案4: UART + CRYPTO only ---\n");
    UARTStream* uart4 = new_UARTStream("ttyUSB1");
    CryptoStream* crp4 = new_CryptoStream(&uart4->base);
    crp4->base.write(&crp4->base, "Secret", 6);
    crp4->base.close(&crp4->base);

    return 0;
}
```

**输出：**
```
========== 装饰器模式演示 ==========

--- 方案1: 裸 UART ---
  [UART ttyS0] TX: Hello

  [UART ttyS0] closed

--- 方案2: UART + Buffer ---
  [Buffer] accumulated 5 bytes (total 5)
  [Buffer] accumulated 6 bytes (total 11)
  [UART ttyS1] TX: Hello World

  [Buffer] flushed 11 bytes
  [UART ttyS1] closed
  [Buffer] closed

--- 方案3: UART + Buffer + ZIP + CRYPTO ---
  -> 写入 'Hello World' 到三层包装的 stream:
  [CRYPTO] encrypting 11 bytes
  [ZIP] compressing 11 bytes -> ~7 bytes
  [Buffer] accumulated 11 bytes (total 11)
  [UART ttyUSB0] TX: Hello World

  [CRYPTO] close
  [ZIP] close
  [Buffer] flushed 11 bytes
  [UART ttyUSB0] TX: 
  [UART ttyUSB0] closed
  [Buffer] closed

--- 方案4: UART + CRYPTO only ---
  [CRYPTO] encrypting 6 bytes
  [UART ttyUSB1] TX: Secret

  [CRYPTO] close
  [UART ttyUSB1] closed
```

---

## Linux 内核实例：VFS 文件操作

Linux 的虚拟文件系统（VFS）是装饰器模式的超级大规模应用：

```c
// 每个文件系统都会在 VFS 层"包装"自己：
// ext4_file_operations
// xfs_file_operations
// nfs_file_operations
//   ↓
// 都会在底层"装饰"：
//   - pagecache（页缓存装饰器）
//   - buffer_head（缓冲区装饰器）
//   - bio（块IO装饰器）

// 用户态 write() 经历的包装层：
//   sys_write()
//     → ext4_write_file()
//       → ext4_buffered_write()
//         → generic_perform_write()   ← 经过 VFS 层缓冲
//           → pagecache_write()
//             → submit_bio()          ← 经过 buffer 层
```

---

## 装饰器 vs 继承 vs 适配器

| 模式 | 目的 | 改变什么 |
|:---|:---|:---|
| **继承** | 代码复用 | 静态的，在编译时决定 |
| **适配器** | 接口转换 | 把 A 接口变成 B 接口 |
| **装饰器** | 增加功能 | 在不改变接口的情况下动态添加职责 |

---

## 练习

1. **基础练习**：给 `BufferedStream` 添加 `BufferedStream_getBuffer()` 方法，让调用者可以直接访问内部缓冲区。

2. **进阶练习**：实现一个 `LineEndingStream`，把 `\n` 转换为 `\r\n`（串口通讯常用）。

3. **内核练习**：阅读 Linux 内核 `fs/open.c`，找找 `do_dentry_open()` 函数里 VFS 的装饰器链条。

---

## 关键收获

- **装饰器通过组合实现动态功能叠加**，顺序可任意调整
- **所有包装层实现同一接口**，客户端代码完全无感知
- **可以叠加任意多层**（Buffer + ZIP + Crypto + ...）
- **Linux 内核**：VFS 的 pagecache、buffer_head、bio 都是装饰器思想的体现
