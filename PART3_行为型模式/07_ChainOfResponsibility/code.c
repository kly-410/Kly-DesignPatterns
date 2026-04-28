/**
 * ===========================================================================
 * Chain of Responsibility Pattern — 责任链模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Help System（帮助系统）
 * ============================================================================*/

typedef enum { LEVEL_1 = 1, LEVEL_2 = 2, LEVEL_3 = 3 } IssueLevel;

typedef struct _HelpHandler {
    int level;
    const char* name;
    struct _HelpHandler* next;
    int (*handle)(struct _HelpHandler*, IssueLevel, const char*);
} HelpHandler;

static int HelpHandler_handle(HelpHandler* h, IssueLevel level, const char* issue) {
    printf("  [%s] Received issue (level=%d): %s\n", h->name, level, issue);
    if (level <= h->level) {
        printf("  [%s] Handled here\n", h->name);
        return 1;
    }
    if (h->next) {
        printf("  [%s] Passing to next...\n", h->name);
        return h->handle(h->next, level, issue);
    }
    printf("  [%s] No handler available!\n", h->name);
    return 0;
}

static HelpHandler* HelpHandler_new(const char* name, int level) {
    HelpHandler* h = calloc(1, sizeof(HelpHandler));
    h->name = name;
    h->level = level;
    h->handle = HelpHandler_handle;
    return h;
}

/* ============================================================================
 * 第二部分：IRQ Handler Chain
 * ============================================================================*/

#define IRQ_HANDLED 1

typedef int (*IRQHandlerFn)(int irq, void* dev_id);

typedef struct _IRQHandlerNode {
    IRQHandlerFn handler;
    void* dev_id;
    char name[32];
    struct _IRQHandlerNode* next;
} IRQHandlerNode;

typedef struct _IRQChain {
    IRQHandlerNode* head;
    int irq;
    void (*register_handler)(struct _IRQChain*, IRQHandlerFn, void*, const char*);
    int (*handle_irq)(struct _IRQChain*, int irq);
} IRQChain;

static void IRQChain_register_handler(IRQChain* chain, IRQHandlerFn h, void* dev, const char* name) {
    IRQHandlerNode* node = malloc(sizeof(IRQHandlerNode));
    node->handler = h; node->dev_id = dev;
    strncpy(node->name, name, 31);
    node->next = chain->head;
    chain->head = node;
    printf("  [IRQChain] Registered: %s\n", name);
}

static int IRQChain_handle_irq(IRQChain* chain, int irq) {
    printf("  [IRQChain] Handling IRQ %d\n", irq);
    IRQHandlerNode* node = chain->head;
    while (node) {
        int ret = node->handler(irq, node->dev_id);
        printf("  [IRQChain]   %s: ret=%d\n", node->name, ret);
        node = node->next;
    }
    return IRQ_HANDLED;
}

static IRQChain* IRQChain_new(int irq) {
    IRQChain* chain = calloc(1, sizeof(IRQChain));
    chain->irq = irq;
    chain->register_handler = IRQChain_register_handler;
    chain->handle_irq = IRQChain_handle_irq;
    return chain;
}

typedef struct _PCIEDevice { char name[32]; uint32_t bar0; } PCIEDevice;

static int pcie_irq_handler(int irq, void* dev_id) {
    PCIEDevice* dev = (PCIEDevice*)dev_id;
    printf("  [PCIe:%s] IRQ%d handled\n", dev->name, irq);
    return IRQ_HANDLED;
}

/* ============================================================================
 * 第三部分：Middleware Chain
 * ============================================================================*/

#define HTTP_OK 200
#define HTTP_FORBIDDEN 403

typedef struct _HTTPRequest {
    const char* path;
    const char* token;
    int user_id;
} HTTPRequest;

typedef struct _Middleware {
    int (*handle)(struct _Middleware*, HTTPRequest*);
    struct _Middleware* next;
    const char* name;
} Middleware;

static int Middleware_dispatch(Middleware* head, HTTPRequest* req) {
    Middleware* m = head;
    while (m) {
        int ret = m->handle(m, req);
        if (ret != HTTP_OK) return ret;
        m = m->next;
    }
    return HTTP_OK;
}

typedef struct _AuthMiddleware { Middleware base; } AuthMiddleware;
static int AuthMiddleware_handle(Middleware* base, HTTPRequest* req) {
    (void)base;
    if (!req->token || strlen(req->token) == 0) { printf("  [Auth] No token!\n"); return HTTP_FORBIDDEN; }
    printf("  [Auth] Token valid\n");
    return HTTP_OK;
}

static AuthMiddleware* AuthMiddleware_new(const char* name) {
    AuthMiddleware* m = calloc(1, sizeof(AuthMiddleware));
    m->base.handle = (int(*)(Middleware*, HTTPRequest*))AuthMiddleware_handle;
    m->base.name = name;
    return m;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("               Chain of Responsibility — 责任链模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Help System */
    printf("【示例1】Help System — 帮助系统\n");
    printf("-----------------------------------------------------------------\n");
    HelpHandler receptionist = { .name = "Receptionist", .level = LEVEL_1, .handle = HelpHandler_handle };
    HelpHandler tech = { .name = "TechSupport", .level = LEVEL_2, .handle = HelpHandler_handle };
    HelpHandler expert = { .name = "Expert", .level = LEVEL_3, .handle = HelpHandler_handle };
    receptionist.next = &tech; tech.next = &expert;

    printf("  Basic question (level=1):\n");
    HelpHandler_handle(&receptionist, LEVEL_1, "Where is bathroom?");
    printf("  Technical question (level=2):\n");
    HelpHandler_handle(&receptionist, LEVEL_2, "Computer won't boot");

    /* 示例2：IRQ Handler Chain */
    printf("\n【示例2】Linux IRQ Handler Chain — 中断处理链\n");
    printf("-----------------------------------------------------------------\n");
    IRQChain* irq17 = IRQChain_new(17);
    PCIEDevice dev1 = { .name = "RTL8111", .bar0 = 0xFEBC0000 };
    PCIEDevice dev2 = { .name = "X710", .bar0 = 0xFEDF0000 };
    irq17->register_handler(irq17, pcie_irq_handler, &dev1, "RTL8111");
    irq17->register_handler(irq17, pcie_irq_handler, &dev2, "X710");
    printf("\n  IRQ 17 fires:\n");
    irq17->handle_irq(irq17, 17);
    free(irq17);

    /* 示例3：Middleware Chain */
    printf("\n【示例3】Middleware Chain — 中间件链\n");
    printf("-----------------------------------------------------------------\n");
    AuthMiddleware* auth = AuthMiddleware_new("Auth");
    HTTPRequest good_req = { .path = "/api/data", .token = "valid_token", .user_id = 42 };
    HTTPRequest bad_req = { .path = "/api/admin", .token = "", .user_id = 1 };
    printf("  Valid request: status=%d\n", Middleware_dispatch(&auth->base, &good_req));
    printf("  Invalid request: status=%d\n", Middleware_dispatch(&auth->base, &bad_req));
    free(auth);

    printf("\n=================================================================\n");
    printf(" Chain of Responsibility 模式核心：handle() 决定处理 or 传递\n");
    printf("=================================================================\n");
    return 0;
}
