/**
 * ===========================================================================
 * Interpreter Pattern — 解释器模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * 第一部分：Simple Calculator（递归下降计算器）
 * ============================================================================*/

typedef enum { TOKEN_NUM, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV, TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_EOF } TokenType;

typedef struct { TokenType type; int value; } Token;

typedef struct {
    const char* input;
    int pos;
} Lexer;

static void Lexer_skip_ws(Lexer* lex) {
    while (lex->input[lex->pos] && isspace(lex->input[lex->pos])) lex->pos++;
}

static Token Lexer_next(Lexer* lex) {
    Lexer_skip_ws(lex);
    char c = lex->input[lex->pos];
    if (!c) return (Token){ TOKEN_EOF, 0 };
    if (isdigit(c)) {
        int val = 0;
        while (isdigit(lex->input[lex->pos])) { val = val * 10 + (lex->input[lex->pos] - '0'); lex->pos++; }
        return (Token){ TOKEN_NUM, val };
    }
    lex->pos++;
    switch (c) {
    case '+': return (Token){ TOKEN_PLUS, 0 };
    case '-': return (Token){ TOKEN_MINUS, 0 };
    case '*': return (Token){ TOKEN_MUL, 0 };
    case '/': return (Token){ TOKEN_DIV, 0 };
    case '(': return (Token){ TOKEN_LPAREN, 0 };
    case ')': return (Token){ TOKEN_RPAREN, 0 };
    default: return (Token){ TOKEN_EOF, 0 };
    }
}

typedef enum { EXPR_NUMBER, EXPR_BINARY } ExprType;
typedef enum { OP_PLUS, OP_MINUS, OP_MUL, OP_DIV } OpType;

typedef struct _Expr {
    ExprType type;
    union {
        int value;
        struct { OpType op; struct _Expr* left; struct _Expr* right; } binary;
    } data;
} Expr;

typedef struct {
    Token token;
    Lexer* lex;
} Parser;

static void Parser_init(Parser* p, Lexer* lex) {
    p->lex = lex;
    p->token = Lexer_next(lex);
}

static void Parser_eat(Parser* p, TokenType type) {
    if (p->token.type == type) p->token = Lexer_next(p->lex);
}

static Expr* Parser_parse_expr(Parser* p);
static Expr* Parser_parse_factor(Parser* p);

static Expr* Parser_parse_term(Parser* p) {
    Expr* left = Parser_parse_factor(p);
    while (p->token.type == TOKEN_MUL || p->token.type == TOKEN_DIV) {
        Expr* expr = calloc(1, sizeof(Expr));
        expr->type = EXPR_BINARY;
        expr->data.binary.op = (p->token.type == TOKEN_MUL) ? OP_MUL : OP_DIV;
        expr->data.binary.left = left;
        Parser_eat(p, p->token.type);
        expr->data.binary.right = Parser_parse_factor(p);
        left = expr;
    }
    return left;
}

static Expr* Parser_parse_expr(Parser* p) {
    Expr* left = Parser_parse_term(p);
    while (p->token.type == TOKEN_PLUS || p->token.type == TOKEN_MINUS) {
        Expr* expr = calloc(1, sizeof(Expr));
        expr->type = EXPR_BINARY;
        expr->data.binary.op = (p->token.type == TOKEN_PLUS) ? OP_PLUS : OP_MINUS;
        expr->data.binary.left = left;
        Parser_eat(p, p->token.type);
        expr->data.binary.right = Parser_parse_term(p);
        left = expr;
    }
    return left;
}

static Expr* Parser_parse_factor(Parser* p) {
    if (p->token.type == TOKEN_LPAREN) {
        Parser_eat(p, TOKEN_LPAREN);
        Expr* expr = Parser_parse_expr(p);
        Parser_eat(p, TOKEN_RPAREN);
        return expr;
    }
    Expr* expr = calloc(1, sizeof(Expr));
    expr->type = EXPR_NUMBER;
    expr->data.value = p->token.value;
    Parser_eat(p, TOKEN_NUM);
    return expr;
}

static Expr* Parser_parse(Parser* p) { return Parser_parse_expr(p); }

static int Expr_eval(Expr* expr) {
    if (expr->type == EXPR_NUMBER) return expr->data.value;
    int l = Expr_eval(expr->data.binary.left);
    int r = Expr_eval(expr->data.binary.right);
    switch (expr->data.binary.op) {
    case OP_PLUS: return l + r;
    case OP_MINUS: return l - r;
    case OP_MUL: return l * r;
    case OP_DIV: return l / r;
    }
    return 0;
}

static void Expr_print(Expr* expr) {
    if (expr->type == EXPR_NUMBER) { printf("%d", expr->data.value); return; }
    printf("(");
    Expr_print(expr->data.binary.left);
    switch (expr->data.binary.op) {
    case OP_PLUS: printf(" + "); break;
    case OP_MINUS: printf(" - "); break;
    case OP_MUL: printf(" * "); break;
    case OP_DIV: printf(" / "); break;
    }
    Expr_print(expr->data.binary.right);
    printf(")");
}

static void Expr_free(Expr* expr) {
    if (!expr) return;
    if (expr->type == EXPR_BINARY) {
        Expr_free(expr->data.binary.left);
        Expr_free(expr->data.binary.right);
    }
    free(expr);
}

/* ============================================================================
 * 第二部分：Config Script Interpreter
 * ============================================================================*/

typedef enum { CMD_SET, CMD_ENABLE, CMD_DISABLE, CMD_PRINT } ConfigCmdType;

typedef struct {
    ConfigCmdType type;
    char target[32];
    int value;
} ConfigCmd;

static ConfigCmd parse_config_cmd(const char* line) {
    ConfigCmd cmd = { CMD_SET, "", 0 };
    char cmd_str[32] = {0};
    char target[32] = {0};
    int value = 0;
    if (sscanf(line, "%31s %31s %d", cmd_str, target, &value) >= 2) {
        if (strcmp(cmd_str, "SET") == 0) { cmd.type = CMD_SET; strncpy(cmd.target, target, 31); cmd.value = value; }
        else if (strcmp(cmd_str, "ENABLE") == 0) { cmd.type = CMD_ENABLE; strncpy(cmd.target, target, 31); }
        else if (strcmp(cmd_str, "DISABLE") == 0) { cmd.type = CMD_DISABLE; strncpy(cmd.target, target, 31); }
        else if (strcmp(cmd_str, "PRINT") == 0) { cmd.type = CMD_PRINT; strncpy(cmd.target, target, 31); }
    }
    return cmd;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                  Interpreter Pattern — 解释器模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Simple Calculator */
    printf("【示例1】Simple Calculator — 递归下降计算器\n");
    printf("-----------------------------------------------------------------\n");
    const char* exprs[] = { "3 + 5 * 2", "(3 + 5) * 2", "10 / 2 - 3" };
    for (int i = 0; i < 3; i++) {
        Lexer lex = { .input = exprs[i], .pos = 0 };
        Parser parser; Parser_init(&parser, &lex);
        Expr* expr = Parser_parse(&parser);
        printf("  Expression: %s\n", exprs[i]);
        printf("  AST: "); Expr_print(expr); printf("\n");
        printf("  Result: %d\n", Expr_eval(expr));
        Expr_free(expr);
    }

    /* 示例2：Config Script Interpreter */
    printf("\n【示例2】Config Script Interpreter — 配置脚本解释器\n");
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
        if (cmd.type == CMD_SET) printf("  SET %s = %d\n", cmd.target, cmd.value);
        else if (cmd.type == CMD_ENABLE) printf("  ENABLE %s\n", cmd.target);
        else if (cmd.type == CMD_DISABLE) printf("  DISABLE %s\n", cmd.target);
        else if (cmd.type == CMD_PRINT) printf("  PRINT %s\n", cmd.target);
    }

    printf("\n=================================================================\n");
    printf(" Interpreter 模式核心：词法分析 + 语法分析 + 递归求值\n");
    printf("=================================================================\n");
    return 0;
}
