/*
 * 04_Decorator - 装饰器模式
 * 
 * 装饰器模式核心：动态地给对象添加职责，比继承更灵活
 * 
 * 本代码演示：
 * 1. Stream 包装器（缓冲、压缩、加密）
 * 2. 洋葱式包装多层叠加
 * 3. 装饰器和被装饰对象实现同一接口
 * 
 * 装饰器 = 包装器，把对象包起来，添加新功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// Component 接口：所有流设备都要实现这套接口
// ============================================================
typedef struct _Stream Stream;
struct _Stream {
    const char* name;
    void (*write)(Stream*, const char* data, size_t len);
    void (*read)(Stream*, char* buf, size_t len);
    void (*flush)(Stream*);
    void (*close)(Stream*);
};

// ============================================================
// Concrete Component：基础 UART 设备
// ============================================================
typedef struct _UARTStream {
    Stream base;
    const char* port;
    char rx_buffer[64];
    int rx_head;
} UARTStream;

static void uart_write(Stream* s, const char* data, size_t len)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] TX: ", u->port);
    for (size_t i = 0; i < len; i++) putchar(data[i]);
    putchar('\n');
}

static void uart_read(Stream* s, char* buf, size_t len)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] RX: waiting %zu bytes...\n", u->port, len);
    (void)u; (void)buf; (void)len;
}

static void uart_flush(Stream* s)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] flush\n", u->port);
}

static void uart_close(Stream* s)
{
    UARTStream* u = (UARTStream*)s;
    printf("  [UART %s] closed\n", u->port);
    free(s);
}

UARTStream* new_UARTStream(const char* port)
{
    UARTStream* u = calloc(1, sizeof(UARTStream));
    u->base.name = "UART";
    u->base.write  = uart_write;
    u->base.read   = uart_read;
    u->base.flush  = uart_flush;
    u->base.close  = uart_close;
    u->port = port;
    return u;
}

// ============================================================
// Decorator：缓冲层
// 累积数据，减少硬件中断次数
// ============================================================
typedef struct _BufferedStream {
    Stream base;
    Stream* wrapped;           // 被包装对象（被装饰者）
    char buffer[256];
    int buf_len;
    int flush_threshold;
} BufferedStream;

static void buffered_write(Stream* s, const char* data, size_t len)
{
    BufferedStream* b = (BufferedStream*)s;
    
    // 累积到缓冲区
    for (size_t i = 0; i < len; i++) {
        if (b->buf_len < (int)sizeof(b->buffer) - 1) {
            b->buffer[b->buf_len++] = data[i];
        } else {
            // 缓冲区满了，自动刷出
            b->base.flush(s);
            b->wrapped->write(b->wrapped, b->buffer, b->buf_len);
            b->buf_len = 0;
        }
    }
    printf("  [Buffer] accumulated %zu bytes (total %d)\n", len, b->buf_len);
}

static void buffered_flush(Stream* s)
{
    BufferedStream* b = (BufferedStream*)s;
    if (b->buf_len > 0) {
        b->wrapped->write(b->wrapped, b->buffer, b->buf_len);
        printf("  [Buffer] flush: sent %d bytes\n", b->buf_len);
        b->buf_len = 0;
    }
}

static void buffered_close(Stream* s)
{
    BufferedStream* b = (BufferedStream*)s;
    b->base.flush(s);
    b->wrapped->close(b->wrapped);
    printf("  [Buffer] closed\n");
    free(s);
}

BufferedStream* new_BufferedStream(Stream* inner)
{
    BufferedStream* b = calloc(1, sizeof(BufferedStream));
    b->base.name = "Buffer";
    b->base.write  = buffered_write;
    b->base.read   = inner->read;
    b->base.flush  = buffered_flush;
    b->base.close  = buffered_close;
    b->wrapped = inner;
    return b;
}

// ============================================================
// Decorator：压缩层
// ============================================================
typedef struct _ZipStream {
    Stream base;
    Stream* wrapped;
} ZipStream;

static void zip_write(Stream* s, const char* data, size_t len)
{
    ZipStream* z = (ZipStream*)s;
    // 模拟压缩：实际会调用 zlib 等库
    size_t compressed_len = len * 2 / 3 + 1;  // 模拟 33% 压缩率
    printf("  [ZIP] compress %zu bytes -> %zu bytes\n", len, compressed_len);
    z->wrapped->write(z->wrapped, data, len);  // 透传，这里简化
}

static void zip_close(Stream* s)
{
    ZipStream* z = (ZipStream*)s;
    printf("  [ZIP] close\n");
    z->wrapped->close(z->wrapped);
    free(s);
}

ZipStream* new_ZipStream(Stream* inner)
{
    ZipStream* z = calloc(1, sizeof(ZipStream));
    z->base.name = "ZIP";
    z->base.write  = zip_write;
    z->base.read   = inner->read;
    z->base.flush  = inner->flush;
    z->base.close  = zip_close;
    z->wrapped = inner;
    return z;
}

// ============================================================
// Decorator：加密层
// ============================================================
typedef struct _CryptoStream {
    Stream base;
    Stream* wrapped;
} CryptoStream;

static void crypto_write(Stream* s, const char* data, size_t len)
{
    CryptoStream* c = (CryptoStream*)s;
    printf("  [CRYPTO] encrypt %zu bytes\n", len);
    // 模拟 XOR 加密
    char encrypted[256];
    size_t n = len < 256 ? len : 256;
    for (size_t i = 0; i < n; i++) {
        encrypted[i] = data[i] ^ 0xAA;
    }
    c->wrapped->write(c->wrapped, encrypted, n);
}

static void crypto_close(Stream* s)
{
    CryptoStream* c = (CryptoStream*)s;
    printf("  [CRYPTO] close\n");
    c->wrapped->close(c->wrapped);
    free(s);
}

CryptoStream* new_CryptoStream(Stream* inner)
{
    CryptoStream* c = calloc(1, sizeof(CryptoStream));
    c->base.name = "Crypto";
    c->base.write  = crypto_write;
    c->base.read   = inner->read;
    c->base.flush  = inner->flush;
    c->base.close  = crypto_close;
    c->wrapped = inner;
    return c;
}

// ============================================================
// 演示辅助函数
// ============================================================
void write_and_close(Stream* s, const char* msg)
{
    printf("\n=== 写入数据到 %s ===\n", s->name);
    s->write(s, msg, strlen(msg));
    s->flush(s);
    s->close(s);
}

int main()
{
    printf("========== 装饰器模式演示 ==========\n\n");

    // 方案1：裸 UART（无包装）
    printf("--- 方案1: 裸 UART ---\n");
    Stream* s1 = (Stream*)new_UARTStream("ttyS0");
    write_and_close(s1, "Hello UART");

    // 方案2：UART + Buffer
    printf("\n--- 方案2: UART + Buffer ---\n");
    Stream* uart2 = (Stream*)new_UARTStream("ttyS1");
    Stream* s2 = (Stream*)new_BufferedStream(uart2);
    write_and_close(s2, "Hello Buffer");

    // 方案3：UART + Buffer + ZIP + CRYPTO（洋葱式）
    printf("\n--- 方案3: UART + Buffer + ZIP + CRYPTO ---\n");
    printf("层层包装：client -> CRYPTO -> ZIP -> Buffer -> UART\n\n");
    
    Stream* uart3   = (Stream*)new_UARTStream("ttyUSB0");
    Stream* buf3   = (Stream*)new_BufferedStream(uart3);
    Stream* zip3   = (Stream*)new_ZipStream(buf3);
    Stream* crp3   = (Stream*)new_CryptoStream(zip3);
    
    write_and_close(crp3, "Hello Layered!");
    
    // 方案4：只加 Crypto（不需要缓冲）
    printf("\n--- 方案4: UART + CRYPTO only ---\n");
    Stream* uart4 = (Stream*)new_UARTStream("ttyUSB1");
    Stream* crp4 = (Stream*)new_CryptoStream(uart4);
    write_and_close(crp4, "Secret");

    // 方案5：Buffer + CRYPTO（不要 ZIP）
    printf("\n--- 方案5: UART + Buffer + CRYPTO (无ZIP) ---\n");
    Stream* uart5 = (Stream*)new_UARTStream("ttyS2");
    Stream* buf5 = (Stream*)new_BufferedStream(uart5);
    Stream* crp5 = (Stream*)new_CryptoStream(buf5);
    write_and_close(crp5, "Quick message");

    printf("\n========== 对比：装饰器 vs 继承 ==========\n");
    printf("继承方案：需要 2^3 = 8 个子类才能覆盖所有组合\n");
    printf("装饰器方案：只需要 4 个类（UART + Buffer + ZIP + Crypto）\n");
    printf("新组合：加1个装饰器类即可，任意叠加\n");
    
    return 0;
}
