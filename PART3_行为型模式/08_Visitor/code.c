/**
 * ===========================================================================
 * 访问者模式 — Visitor Pattern
 * ===========================================================================
 *
 * 核心思想：数据结构稳定，但操作经常变化
 *
 * 本代码演示（嵌入式视角）：
 *   PCIe 配置空间：寄存器结构固定，但需要读取/验证/日志等不同操作
 *
 * 访问者模式的关键：双重分派
 *   element.accept(visitor) → visitor.visit(element)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：PCIe 配置空间寄存器（元素）
 * ============================================================================*/

/**
 * 寄存器结构
 *
 * accept(visitor) 是关键：接受访问者，调用访问者的 visit 方法
 * 这里用函数指针 + 上下文参数实现双重分派
 */
typedef struct _PCIeReg {
    const char* name;      /**< 寄存器名称 */
    uint32_t offset;       /**< 配置空间偏移 */
    uint32_t value;        /**< 当前值 */

    /**
     * 接受访问者
     * @param reg     当前寄存器
     * @param ctx     访问者上下文（类型由具体函数解释）
     *
     * 双重分派：
     *   第一次分派：PCIeReg.accept()
     *   第二次分派：ctx (访问者) 调用对应的 visit 方法
     */
    void (*accept)(struct _PCIeReg* reg, void* ctx);
} PCIeReg;

/** 创建 VendorID 寄存器 */
static PCIeReg* VendorID_new(uint32_t offset, uint32_t value) {
    PCIeReg* r = calloc(1, sizeof(PCIeReg));
    r->name = "VendorID"; r->offset = offset; r->value = value;
    return r;
}

/** 创建 BAR 寄存器 */
static PCIeReg* BAR_new(int bar_num, uint32_t offset, uint32_t value) {
    PCIeReg* r = calloc(1, sizeof(PCIeReg));
    asprintf((char**)&r->name, "BAR%d", bar_num);
    r->offset = offset; r->value = value;
    return r;
}

/** 创建 Command 寄存器 */
static PCIeReg* Command_new(uint32_t offset, uint32_t value) {
    PCIeReg* r = calloc(1, sizeof(PCIeReg));
    r->name = "Command"; r->offset = offset; r->value = value;
    return r;
}

/* ============================================================================
 * 第二部分：DumpVisitor（打印所有寄存器）
 * ============================================================================*/

/**
 * DumpVisitor 的 visit 函数
 *
 * @param reg  被访问的寄存器
 * @param ctx 未使用（兼容接口）
 *
 * 功能：打印寄存器名称、偏移、当前值
 */
static void DumpVisitor_visit(PCIeReg* reg, void* ctx) {
    (void)ctx;
    printf("  [Dump] %s[0x%02X] = 0x%08X\n", reg->name, reg->offset, reg->value);
}

/**
 * 为所有寄存器调用 DumpVisitor
 *
 * foreach_register 是"对象结构"的管理函数
 * 它遍历所有元素，调用每个元素的 accept 方法
 */
static void DumpVisitor_apply(PCIeReg** regs, int count) {
    printf("  === DumpVisitor: 打印所有寄存器 ===\n");
    for (int i = 0; i < count; i++) {
        regs[i]->accept(regs[i], NULL);  /**< NULL = DumpVisitor */
    }
}

/* ============================================================================
 * 第三部分：ValidateVisitor（验证配置合法性）
 * ============================================================================*/

/**
 * ValidateVisitor 的 visit 函数
 *
 * @param reg  被访问的寄存器
 * @param ctx 未使用（兼容接口）
 *
 * 功能：检查寄存器值是否合法，给出警告或错误
 */
static void ValidateVisitor_visit(PCIeReg* reg, void* ctx) {
    (void)ctx;
    if (strcmp(reg->name, "VendorID") == 0) {
        if (reg->value == 0xFFFF || reg->value == 0x0000)
            printf("  [Validate] ❌ %s=0x%04X 无效\n", reg->name, reg->value);
        else
            printf("  [Validate] ✅ %s=0x%04X 正常\n", reg->name, reg->value);
    } else if (strncmp(reg->name, "BAR", 3) == 0) {
        if (reg->value == 0x00000000)
            printf("  [Validate] ❌ %s 未映射（地址为0）\n", reg->name);
        else
            printf("  [Validate] ✅ %s 已映射: 0x%08X\n", reg->name, reg->value);
    } else if (strcmp(reg->name, "Command") == 0) {
        if (reg->value == 0x0000)
            printf("  [Validate] ⚠️ %s=0，所有功能禁用\n", reg->name);
        else
            printf("  [Validate] ✅ %s=0x%04X，功能已使能\n", reg->name, reg->value);
    }
}

static void ValidateVisitor_apply(PCIeReg** regs, int count) {
    printf("  === ValidateVisitor: 验证配置 ===\n");
    for (int i = 0; i < count; i++) {
        regs[i]->accept(regs[i], NULL);
    }
}

/* ============================================================================
 * 第四部分：访问者的 accept 实现
 *
 * 每个寄存器都要实现自己的 accept
 * accept 调用对应访问者的 visit 函数
 * ============================================================================*/

static void VendorID_accept(PCIeReg* reg, void* ctx) {
    (void)ctx;
    /**< 这里直接调用函数指针，通过 extern 变量选择具体访问者 */
    DumpVisitor_visit(reg, NULL);  /**< 简化：直接调用 */
}

static void BAR_accept(PCIeReg* reg, void* ctx) {
    (void)ctx;
    DumpVisitor_visit(reg, NULL);
}

static void Command_accept(PCIeReg* reg, void* ctx) {
    (void)ctx;
    DumpVisitor_visit(reg, NULL);
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                 Visitor Pattern — 访问者模式\n");
    printf("=================================================================\n\n");

    /**< 构建简化的 PCIe 配置空间（5个寄存器）*/
    PCIeReg* regs[5];
    regs[0] = VendorID_new(0x00, 0x80861025);         /**< Intel VID */
    regs[1] = Command_new(0x04, 0x0007);              /**< Command: IO+MEM+BusMaster */
    regs[2] = BAR_new(0, 0x10, 0xFE400004);         /**< BAR0: Memory 映射 */
    regs[3] = BAR_new(1, 0x14, 0x00000000);         /**< BAR1: 未映射 */
    regs[4] = BAR_new(2, 0x18, 0xFE402000);         /**< BAR2: Memory */

    printf("【示例】PCIe 配置空间 — 同一组寄存器，不同访问者\n\n");

    /**< DumpVisitor：打印所有寄存器 */
    printf("--- DumpVisitor: 打印所有寄存器 ---\n");
    for (int i = 0; i < 5; i++) regs[i]->accept = DumpVisitor_visit;
    for (int i = 0; i < 5; i++) regs[i]->accept(regs[i], NULL);

    printf("\n--- ValidateVisitor: 验证配置合法性 ---\n");
    for (int i = 0; i < 5; i++) regs[i]->accept = ValidateVisitor_visit;
    for (int i = 0; i < 5; i++) regs[i]->accept(regs[i], NULL);

    printf("\n=================================================================\n");
    printf(" 访问者核心：element.accept(visitor) → visitor.visit(element)\n");
    printf(" 嵌入式场景：PCIe配置空间、文件系统、编译器AST\n");
    printf("=================================================================\n");

    for (int i = 0; i < 5; i++) free(regs[i]);
    return 0;
}
