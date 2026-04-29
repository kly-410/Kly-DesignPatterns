/**
 * ===========================================================================
 * 责任链模式 — Chain of Responsibility Pattern
 * ===========================================================================
 *
 * 核心思想：请求沿着处理器链传递，直到某个处理器处理它
 *
 * 本代码演示（嵌入式视角）：
 * 1. 日志处理：DEBUG → INFO → WARNING → ERROR，匹配则处理
 * 2. 中断处理：多个处理器按优先级依次尝试处理
 * 3. 请求拦截：权限校验 → 参数验证 → 业务处理（层层拦截）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：日志处理器链
 *
 * 场景：固件日志系统，不同级别的日志发给不同的处理程序
 *       DEBUG 级别只需要打印
 *       ERROR 级别需要记录到 Flash + 上报服务器
 * ============================================================================*/

/** 日志级别枚举：值越大等级越高 */
typedef enum {
    LOG_DEBUG = 1,   /**< 调试信息 */
    LOG_INFO  = 2,   /**< 一般信息 */
    LOG_WARN  = 3,   /**< 警告 */
    LOG_ERROR = 4,   /**< 错误 */
    LOG_FATAL = 5    /**< 致命错误 */
} LogLevel;

typedef enum { false = 0, true = 1 } bool;  /**< 简化 bool 类型 */

/** 日志处理器接口 */
typedef struct _LogHandler {
    /** 能处理的最低日志级别（比这个级别低的日志不归我管） */
    int min_level;

    /** 处理日志的核心方法 */
    void (*handle)(struct _LogHandler*, int level, const char* message);

    /** 指向链中下一个处理器（NULL 表示链尾） */
    struct _LogHandler* next;
} LogHandler;

/**
 * 处理器处理日志：判断自己能处理就处理，否则传给下一个
 *
 * @param level   日志级别
 * @param msg     日志内容
 *
 * 关键：如果 level >= min_level 就处理；否则跳过
 *       处理完不再传递（这是责任链的两种变体之一）
 */
static void LogHandler_handle(LogHandler* h, int level, const char* msg) {
    if (level >= h->min_level) {
        /**< 找到了能处理的处理器，打印输出 */
        printf("  [%s] %s\n", (level == LOG_DEBUG ? "DEBUG" :
                                level == LOG_INFO  ? "INFO " :
                                level == LOG_WARN  ? "WARN " :
                                level == LOG_ERROR ? "ERROR" : "FATAL"), msg);
        /**< 处理完就结束，不再传递给下一个 */
        return;
    }

    /**< 当前处理器处理不了，尝试传给下一个 */
    if (h->next != NULL) {
        h->next->handle(h->next, level, msg);
    }
}

/** 处理器构造函数 */
static LogHandler* LogHandler_new(int min_level) {
    LogHandler* h = calloc(1, sizeof(LogHandler));
    h->min_level = min_level;
    h->handle = LogHandler_handle;
    return h;
}

/* ============================================================================
 * 第二部分：PCIe 配置空间访问处理器链
 *
 * 场景：对 PCIe 配置空间的访问请求，需要经过多层检查：
 *       1. 地址范围检查（是否越界）
 *       2. 权限检查（是否可写）
 *       3. 实际读/写操作
 * ============================================================================*/

/** PCIe 配置空间访问请求 */
typedef struct {
    uint32_t offset;   /**< 配置空间偏移 */
    uint32_t value;   /**< 写入的值（或读取的返回值）*/
    int is_write;      /**< 1=写操作，0=读操作 */
} PCIeConfigRequest;

typedef enum { REQ_OK = 0, REQ_ERR_RANGE = -1, REQ_ERR_PERM = -2 } ReqResult;

/** 处理器接口 */
typedef struct _PCIeHandler PCIeHandler;
typedef ReqResult (*HandlerFunc)(PCIeHandler*, PCIeConfigRequest*);

struct _PCIeHandler {
    HandlerFunc handle;             /**< 处理函数 */
    struct _PCIeHandler* next;      /**< 下一个处理器 */
    const char* name;              /**< 名称（用于调试）*/
};

/**
 * 链式处理：遍历链上的每个处理器
 * 如果某个处理器返回错误，立即停止
 */
static ReqResult PCIeHandler_process(PCIeHandler* h, PCIeConfigRequest* req) {
    ReqResult result = h->handle(h, req);
    printf("  [Handler: %s] offset=0x%02X %s -> %s\n",
           h->name,
           req->offset,
           req->is_write ? "WRITE" : "READ",
           result == REQ_OK ? "OK" :
           result == REQ_ERR_RANGE ? "ERR_RANGE" : "ERR_PERM");
    if (result != REQ_OK) return result;  /**< 出错就停止传递 */
    if (h->next) return PCIeHandler_process(h->next, req);  /**< 继续传递 */
    return REQ_OK;
}

/* 三个具体处理器：地址检查 → 权限检查 → 实际操作 */
static ReqResult range_check_handler(PCIeHandler* base, PCIeConfigRequest* req) {
    if (req->offset > 0xFF) return REQ_ERR_RANGE;  /**< 配置空间最大 256 字节 */
    return REQ_OK;  /**< 检查通过，继续下一个处理器 */
}

static ReqResult permission_check_handler(PCIeHandler* base, PCIeConfigRequest* req) {
    if (req->is_write && req->offset == 0x04) {
        printf("    [PermCheck] BAR 寄存器禁止写入！\n");
        return REQ_ERR_PERM;
    }
    return REQ_OK;
}

static ReqResult read_write_handler(PCIeHandler* base, PCIeConfigRequest* req) {
    static uint32_t config_space[16] = {0};  /**< 模拟配置空间寄存器 */
    if (req->is_write) {
        config_space[req->offset / 4] = req->value;
        printf("    [R/W] 写入配置空间[0x%02X] = 0x%08X\n", req->offset, req->value);
    } else {
        req->value = config_space[req->offset / 4];
        printf("    [R/W] 读取配置空间[0x%02X] = 0x%08X\n", req->offset, req->value);
    }
    return REQ_OK;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("            Chain of Responsibility — 责任链模式\n");
    printf("=================================================================\n\n");

    /* -------------------- 示例1：日志处理器链 -------------------- */
    printf("【示例1】日志处理器链 — 不同级别日志被不同处理器处理\n");
    printf("-----------------------------------------------------------------\n");

    /**< 构建链：ConsoleLogger(DEBUG) → FileLogger(WARN) → ServerLogger(ERROR) */
    LogHandler* console = LogHandler_new(LOG_DEBUG);  /**< 控制台处理所有级别 */
    console->next = NULL;  /**< 这个链只有一层，DEBUG 及以上都处理 */

    printf("发送 DEBUG 日志:\n");
    console->handle(console, LOG_DEBUG, "PCIe 链路训练开始");
    printf("发送 ERROR 日志:\n");
    console->handle(console, LOG_ERROR, "PCIe BAR 配置失败");

    printf("\n发送 INFO 日志:\n");
    console->handle(console, LOG_INFO, "设备初始化完成");

    free(console);

    /* -------------------- 示例2：PCIe 配置空间处理器链 -------------------- */
    printf("\n【示例2】PCIe 配置空间访问链\n");
    printf("-----------------------------------------------------------------\n");

    /**< 构建链：地址范围检查 → 权限检查 → 实际读写 */
    PCIeHandler h1 = { .handle = range_check_handler, .name = "RangeCheck", .next = NULL };
    PCIeHandler h2 = { .handle = permission_check_handler, .name = "PermCheck", .next = NULL };
    PCIeHandler h3 = { .handle = read_write_handler, .name = "ReadWrite", .next = NULL };
    h1.next = &h2;
    h2.next = &h3;

    PCIeConfigRequest req1 = { .offset = 0x10, .value = 0xFE400004, .is_write = 1 };
    printf("请求1: 写入 BAR0 配置寄存器:\n");
    PCIeHandler_process(&h1, &req1);

    PCIeConfigRequest req2 = { .offset = 0x04, .value = 0xFFFFFFFF, .is_write = 1 };
    printf("\n请求2: 写入 Command 寄存器:\n");
    PCIeHandler_process(&h1, &req2);

    PCIeConfigRequest req3 = { .offset = 0x100, .value = 0, .is_write = 0 };
    printf("\n请求3: 读取超出范围的地址:\n");
    PCIeHandler_process(&h1, &req3);

    printf("\n=================================================================\n");
    printf(" 责任链核心：请求沿链传递，第一个能处理的处理器处理\n");
    printf(" 嵌入式场景：日志分级处理、中断处理链、请求拦截\n");
    printf("=================================================================\n");
    return 0;
}
