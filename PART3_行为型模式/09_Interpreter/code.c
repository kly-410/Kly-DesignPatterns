/**
 * ===========================================================================
 * 解释器模式 — Interpreter Pattern
 * ===========================================================================
 *
 * 核心思想：定义语言的语法表示，解释其中的句子
 *
 * 本代码演示（嵌入式视角）：
 * 1. 简易计算器：解析并计算 "3 + 5 * 2" 这样的表达式
 * 2. 配置脚本解释器：解析 "SET bar0 0xFEBC0000" 这样的固件配置命令
 *
 * 解释器模式结构：
 *   输入文本 → 词法分析(Lexer) → Token流 → 语法分析(Parser) → AST → 求值(Evaluator)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * 第一部分：词法分析器（Lexer）
 *
 * 把输入字符串切分成 Token（标记）
 * 例如："3 + 5 * 2" → [NUM:3][PLUS][NUM:5][MUL][NUM:2]
 *
 * 固件场景：AT命令解析、JSON/XML解析、指令解码
 * ============================================================================*/

/** Token 类型枚举：语言的词汇表 */
typedef enum {
    TOKEN_NUM,      /**< 数字：0 1 2 ... 9 */
    TOKEN_PLUS,     /**< 加号 + */
    TOKEN_MINUS,    /**< 减号 - */
    TOKEN_MUL,      /**< 乘号 * */
    TOKEN_DIV,      /**< 除号 / */
    TOKEN_LPAREN,   /**< 左括号 ( */
    TOKEN_RPAREN,   /**< 右括号 ) */
    TOKEN_EOF       /**< 输入结束 */
} TokenType;

/** Token 结构：词法分析的最小单元 */
typedef struct {
    TokenType type;  /**< Token 类型 */
    int value;       /**< 如果是数字，存储数值 */
} Token;

/** Lexer：输入文本 + 当前读取位置 */
typedef struct {
    const char* input;  /**< 输入的表达式字符串 */
    int pos;            /**< 当前读取位置（索引）*/
} Lexer;

/**
 * 跳过空白字符（空格、Tab等）
 * C 标准库的 isspace() 判断字符是否为空白字符
 */
static void Lexer_skip_ws(Lexer* lex) {
    while (lex->input[lex->pos] && isspace(lex->input[lex->pos]))
        lex->pos++;
}

/**
 * 获取下一个 Token
 *
 * 词法分析核心逻辑：
 *   1. 跳过空白
 *   2. 判断首字符决定 Token 类型
 *   3. 数字需要读取完整数值（可能多位）
 */
static Token Lexer_next(Lexer* lex) {
    Lexer_skip_ws(lex);

    char c = lex->input[lex->pos];
    if (!c) return (Token){ TOKEN_EOF, 0 };  /**< 输入结束 */

    /**< 数字：读取完整的十进制整数 */
    if (isdigit(c)) {
        int val = 0;
        while (isdigit(lex->input[lex->pos])) {
            /**< 逐字符拼接成整数：val = val * 10 + 当前位 */
            val = val * 10 + (lex->input[lex->pos] - '0');
            lex->pos++;
        }
        return (Token){ TOKEN_NUM, val };
    }

    /**< 单字符 Token（操作符、括号）*/
    lex->pos++;  /**< 消费当前字符 */
    switch (c) {
    case '+': return (Token){ TOKEN_PLUS, 0 };
    case '-': return (Token){ TOKEN_MINUS, 0 };
    case '*': return (Token){ TOKEN_MUL, 0 };
    case '/': return (Token){ TOKEN_DIV, 0 };
    case '(': return (Token){ TOKEN_LPAREN, 0 };
    case ')': return (Token){ TOKEN_RPAREN, 0 };
    default:  return (Token){ TOKEN_EOF, 0 };
    }
}

/* ============================================================================
 * 第二部分：语法分析器（Parser）— 递归下降
 *
 * 经典优先级处理（从低到高）：
 *   expr   = term (('+' | '-') term)*
 *   term   = factor (('*' | '/') factor)*
 *   factor = '(' expr ')' | NUMBER
 *
 * 翻译：expr 由 term 组成，term 由 factor 组成，factor 最高优先级
 * ============================================================================*/

/** AST 节点类型 */
typedef enum {
    EXPR_NUMBER,   /**< 叶子节点：数字 */
    EXPR_BINARY     /**< 二叉节点：二元运算（左右操作数 + 操作符）*/
} ExprType;

/** 二元操作符 */
typedef enum { OP_PLUS, OP_MINUS, OP_MUL, OP_DIV } OpType;

/**
 * AST 节点（抽象语法树）
 *
 * 用 union 节省内存：
 *   - 数字节点：value 存储数值
 *   - 二元节点：op + left + right 三个字段
 */
typedef struct _Expr {
    ExprType type;  /**< 节点类型 */
    union {
        int value;  /**< EXPR_NUMBER 时使用 */
        struct {
            OpType op;         /**< 操作符 */
            struct _Expr* left;   /**< 左操作数 */
            struct _Expr* right;  /**< 右操作数 */
        } binary;
    } data;
} Expr;

/** Parser：维护 Token 流，提供递归下降解析 */
typedef struct {
    Token token;    /**< 当前 Token（lookahead）*/
    Lexer* lex;     /**< Lexer 引用 */
} Parser;

/** 初始化 Parser：读取第一个 Token */
static void Parser_init(Parser* p, Lexer* lex) {
    p->lex = lex;
    p->token = Lexer_next(lex);  /**< 预读第一个 Token */
}

/** 消费指定类型的 Token（匹配）*/
static void Parser_eat(Parser* p, TokenType expected) {
    if (p->token.type == expected)
        p->token = Lexer_next(p->lex);  /**< 读下一个 Token */
}

/** 声明：解析 expr 需要先声明 term 和 factor */
static Expr* Parser_parse_expr(Parser* p);
static Expr* Parser_parse_factor(Parser* p);

/**
 * 解析 term：处理乘除（优先级高于加减）
 *
 * term = factor (('*' | '/') factor)*
 */
static Expr* Parser_parse_term(Parser* p) {
    Expr* left = Parser_parse_factor(p);  /**< 先解析左操作数（高优先级）*/

    /**< 处理连续的 * 或 / */
    while (p->token.type == TOKEN_MUL || p->token.type == TOKEN_DIV) {
        Expr* expr = calloc(1, sizeof(Expr));
        expr->type = EXPR_BINARY;
        expr->data.binary.op = (p->token.type == TOKEN_MUL) ? OP_MUL : OP_DIV;
        expr->data.binary.left = left;  /**< 连接左操作数 */

        Parser_eat(p, p->token.type);  /**< 消费 * 或 / */
        expr->data.binary.right = Parser_parse_factor(p);  /**< 解析右操作数 */

        left = expr;  /**< 将新节点作为下一次循环的左操作数 */
    }
    return left;
}

/**
 * 解析 expr：处理加减（优先级低于乘除）
 *
 * expr = term (('+' | '-') term)*
 */
static Expr* Parser_parse_expr(Parser* p) {
    Expr* left = Parser_parse_term(p);  /**< 先解析左 term */

    while (p->token.type == TOKEN_PLUS || p->token.type == TOKEN_MINUS) {
        Expr* expr = calloc(1, sizeof(Expr));
        expr->type = EXPR_BINARY;
        expr->data.binary.op = (p->token.type == TOKEN_PLUS) ? OP_PLUS : OP_MINUS;
        expr->data.binary.left = left;

        Parser_eat(p, p->token.type);  /**< 消费 + 或 - */
        expr->data.binary.right = Parser_parse_term(p);

        left = expr;
    }
    return left;
}

/**
 * 解析 factor：最高优先级
 *
 * factor = '(' expr ')' | NUMBER
 *
 * 括号内是完整的 expr，递归调用 Parser_parse_expr
 */
static Expr* Parser_parse_factor(Parser* p) {
    if (p->token.type == TOKEN_LPAREN) {
        Parser_eat(p, TOKEN_LPAREN);          /**< 消费 ( */
        Expr* expr = Parser_parse_expr(p);   /**< 解析括号内的表达式 */
        Parser_eat(p, TOKEN_RPAREN);          /**< 消费 ) */
        return expr;
    }
    /**< 数字：创建叶子节点 */
    Expr* expr = calloc(1, sizeof(Expr));
    expr->type = EXPR_NUMBER;
    expr->data.value = p->token.value;
    Parser_eat(p, TOKEN_NUM);
    return expr;
}

/** 入口：开始解析 expr */
static Expr* Parser_parse(Parser* p) { return Parser_parse_expr(p); }

/* ============================================================================
 * 第三部分：AST 求值器（Evaluator）
 * ============================================================================*/

/**
 * 递归求值：从叶子节点向上计算
 *
 * 数字节点 → 返回数值
 * 二元节点 → 递归求值左右子树 → 按操作符计算
 */
static int Expr_eval(Expr* expr) {
    if (expr->type == EXPR_NUMBER)
        return expr->data.value;

    int l = Expr_eval(expr->data.binary.left);
    int r = Expr_eval(expr->data.binary.right);
    switch (expr->data.binary.op) {
    case OP_PLUS:  return l + r;
    case OP_MINUS: return l - r;
    case OP_MUL:   return l * r;
    case OP_DIV:   return l / r;
    }
    return 0;
}

/** 打印 AST（括号中缀表示法）*/
static void Expr_print(Expr* expr) {
    if (expr->type == EXPR_NUMBER) {
        printf("%d", expr->data.value);
        return;
    }
    printf("(");
    Expr_print(expr->data.binary.left);
    switch (expr->data.binary.op) {
    case OP_PLUS:  printf(" + "); break;
    case OP_MINUS: printf(" - "); break;
    case OP_MUL:   printf(" * "); break;
    case OP_DIV:   printf(" / "); break;
    }
    Expr_print(expr->data.binary.right);
    printf(")");
}

/** 释放 AST 内存（二叉树后序遍历释放）*/
static void Expr_free(Expr* expr) {
    if (!expr) return;
    if (expr->type == EXPR_BINARY) {
        Expr_free(expr->data.binary.left);
        Expr_free(expr->data.binary.right);
    }
    free(expr);
}

/* ============================================================================
 * 第四部分：固件配置脚本解释器
 *
 * 场景：固件通过脚本配置外设
 *   SET bar0 0xFEBC0000  — 设置 BAR0 地址
 *   ENABLE pcie          — 使能 PCIe
 *   PRINT irq            — 打印 IRQ 配置
 * ============================================================================*/

/** 配置命令类型 */
typedef enum {
    CMD_SET,     /**< SET key value — 设置配置 */
    CMD_ENABLE,  /**< ENABLE target — 使能某功能 */
    CMD_DISABLE,  /**< DISABLE target — 禁用某功能 */
    CMD_PRINT    /**< PRINT target — 打印配置值 */
} ConfigCmdType;

/** 配置命令结构 */
typedef struct {
    ConfigCmdType type;  /**< 命令类型 */
    char target[32];     /**< 目标配置项（如 bar0、irq）*/
    int value;           /**< SET 命令的值 */
} ConfigCmd;

/**
 * 解析一行配置脚本（简易版：空格分隔）
 *
 * sscanf 格式解释：
 *   %31s  — 读取最多31个非空白字符
 *   %d     — 读取整数
 */
static ConfigCmd parse_config_cmd(const char* line) {
    ConfigCmd cmd = { CMD_SET, "", 0 };
    char cmd_str[32] = {0};
    char target[32] = {0};
    int value = 0;

    /**< 尝试解析命令和目标（value 可选）*/
    if (sscanf(line, "%31s %31s %d", cmd_str, target, &value) >= 2) {
        if (strcmp(cmd_str, "SET") == 0) {
            cmd.type = CMD_SET;
            strncpy(cmd.target, target, 31);
            cmd.value = value;
        } else if (strcmp(cmd_str, "ENABLE") == 0) {
            cmd.type = CMD_ENABLE;
            strncpy(cmd.target, target, 31);
        } else if (strcmp(cmd_str, "DISABLE") == 0) {
            cmd.type = CMD_DISABLE;
            strncpy(cmd.target, target, 31);
        } else if (strcmp(cmd_str, "PRINT") == 0) {
            cmd.type = CMD_PRINT;
            strncpy(cmd.target, target, 31);
        }
    }
    return cmd;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                Interpreter Pattern — 解释器模式\n");
    printf("=================================================================\n\n");

    /** -------------------- 示例1：计算器 -------------------- */
    printf("【示例1】简易计算器 — 递归下降解析表达式\n");
    printf("-----------------------------------------------------------------\n");
    printf("表达式解析顺序：Lexer → Token流 → Parser → AST → Evaluator\n\n");

    const char* exprs[] = { "3 + 5 * 2", "(3 + 5) * 2", "10 / 2 - 3" };
    for (int i = 0; i < 3; i++) {
        Lexer lex = { .input = exprs[i], .pos = 0 };
        Parser parser; Parser_init(&parser, &lex);
        Expr* expr = Parser_parse(&parser);

        printf("  表达式: %s\n", exprs[i]);
        printf("  AST:    "); Expr_print(expr); printf("\n");
        printf("  结果:   %d\n", Expr_eval(expr));
        Expr_free(expr);
    }

    /** -------------------- 示例2：配置脚本 -------------------- */
    printf("\n【示例2】固件配置脚本解释器\n");
    printf("-----------------------------------------------------------------\n");

    const char* cmds[] = {
        "SET bar0 0xFEBC0000",
        "ENABLE pcie",
        "SET irq 17",
        "PRINT bar0"
    };
    for (int i = 0; i < 4; i++) {
        ConfigCmd cmd = parse_config_cmd(cmds[i]);
        printf("  > %s\n", cmds[i]);
        if (cmd.type == CMD_SET)     printf("    → 设置 %s = 0x%X\n", cmd.target, cmd.value);
        else if (cmd.type == CMD_ENABLE)  printf("    → 使能 %s\n", cmd.target);
        else if (cmd.type == CMD_DISABLE) printf("    → 禁用 %s\n", cmd.target);
        else if (cmd.type == CMD_PRINT)   printf("    → 打印 %s\n", cmd.target);
    }

    printf("\n=================================================================\n");
    printf(" 解释器核心：Lexer → Token → Parser → AST → Evaluator\n");
    printf(" 嵌入式场景：AT命令、配置脚本、协议解析、指令解码\n");
    printf("=================================================================\n");
    return 0;
}
