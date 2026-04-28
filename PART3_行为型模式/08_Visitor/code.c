/**
 * ===========================================================================
 * Visitor Pattern — 访问者模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 第一部分：File System（文件系统遍历）
 * ============================================================================*/

typedef struct _IVisitor IVisitor;

typedef struct _IElement {
    const char* (*getName)(struct _IElement*);
    int (*isDirectory)(struct _IElement*);
    void (*accept)(struct _IElement*, IVisitor*);
    struct _IElement** children;
    int child_count;
} IElement;

typedef struct _IVisitor {
    void (*visitFile)(IVisitor*, IElement*);
    void (*visitDirectory)(IVisitor*, IElement*);
} IVisitor;

typedef struct _File {
    IElement base;
    int size_bytes;
} File;

static const char* File_getName(IElement* base) { (void)base; return "file"; }
static int File_isDirectory(IElement* base) { (void)base; return 0; }
static void File_accept(IElement* base, IVisitor* visitor) { visitor->visitFile(visitor, base); }

static File* File_new(int size) {
    File* f = calloc(1, sizeof(File));
    f->base.getName = File_getName;
    f->base.isDirectory = File_isDirectory;
    f->base.accept = File_accept;
    f->size_bytes = size;
    return f;
}

typedef struct _Directory {
    IElement base;
    char name[32];
} Directory;

static const char* Directory_getName(IElement* base) { return ((Directory*)base)->name; }
static int Directory_isDirectory(IElement* base) { (void)base; return 1; }

static void Directory_accept(IElement* base, IVisitor* visitor) {
    visitor->visitDirectory(visitor, base);
    Directory* d = (Directory*)base;
    for (int i = 0; i < d->base.child_count; i++)
        d->base.children[i]->accept(d->base.children[i], visitor);
}

static Directory* Directory_new(const char* name) {
    Directory* d = calloc(1, sizeof(Directory));
    d->base.getName = Directory_getName;
    d->base.isDirectory = Directory_isDirectory;
    d->base.accept = Directory_accept;
    strncpy(d->name, name, 31);
    return d;
}

typedef struct _ListVisitor {
    IVisitor base;
    int depth;
} ListVisitor;

static void ListVisitor_visitFile(IVisitor* base, IElement* elem) {
    ListVisitor* v = (ListVisitor*)base;
    for (int i = 0; i < v->depth; i++) printf("  ");
    File* f = (File*)elem;
    printf("file: %d bytes\n", f->size_bytes);
}

static void ListVisitor_visitDirectory(IVisitor* base, IElement* elem) {
    ListVisitor* v = (ListVisitor*)base;
    for (int i = 0; i < v->depth; i++) printf("  ");
    Directory* d = (Directory*)elem;
    printf("DIR: %s/\n", d->name);
    v->depth++;
}

static ListVisitor* ListVisitor_new(void) {
    ListVisitor* v = calloc(1, sizeof(ListVisitor));
    v->base.visitFile = ListVisitor_visitFile;
    v->base.visitDirectory = ListVisitor_visitDirectory;
    return v;
}

/* ============================================================================
 * 第二部分：AST Visitor（抽象语法树遍历）
 * ============================================================================*/

typedef enum { EXPR_NUM, EXPR_ADD, EXPR_MUL } ExprType;

typedef struct _ASTNode {
    ExprType type;
    union {
        int num;
        struct { struct _ASTNode* left; struct _ASTNode* right; } bin;
    } data;
} ASTNode;

static int interpret_ast(ASTNode* node) {
    switch (node->type) {
    case EXPR_NUM: return node->data.num;
    case EXPR_ADD: return interpret_ast(node->data.bin.left) + interpret_ast(node->data.bin.right);
    case EXPR_MUL: return interpret_ast(node->data.bin.left) * interpret_ast(node->data.bin.right);
    }
    return 0;
}

static void print_ast(ASTNode* node) {
    if (node->type == EXPR_NUM) { printf("%d", node->data.num); return; }
    printf("(");
    if (node->type == EXPR_ADD) {
        print_ast(node->data.bin.left); printf(" + "); print_ast(node->data.bin.right);
    } else {
        print_ast(node->data.bin.left); printf(" * "); print_ast(node->data.bin.right);
    }
    printf(")");
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                    Visitor Pattern — 访问者模式\n");
    printf("=================================================================\n\n");

    /* 示例1：File System */
    printf("【示例1】File System — 文件系统遍历\n");
    printf("-----------------------------------------------------------------\n");
    Directory* root = Directory_new("root");
    Directory* usr = Directory_new("usr");
    File* bin = File_new(4096);
    File* lib = File_new(8192);
    usr->base.children = (IElement*[]){ &bin->base, &lib->base };
    usr->base.child_count = 2;
    root->base.children = (IElement*[]){ &usr->base };
    root->base.child_count = 1;

    ListVisitor* lv = ListVisitor_new();
    root->base.accept(&root->base, &lv->base);
    free(lv); free(bin); free(lib); free(usr); free(root);

    /* 示例2：AST Visitor */
    printf("\n【示例2】AST Visitor — 抽象语法树遍历\n");
    printf("-----------------------------------------------------------------\n");
    ASTNode n3 = { .type = EXPR_NUM, .data.num = 3 };
    ASTNode n5 = { .type = EXPR_NUM, .data.num = 5 };
    ASTNode n2 = { .type = EXPR_NUM, .data.num = 2 };
    ASTNode add = { .type = EXPR_ADD, .data.bin.left = &n3, .data.bin.right = &n5 };
    ASTNode mul = { .type = EXPR_MUL, .data.bin.left = &add, .data.bin.right = &n2 };
    printf("  AST: "); print_ast(&mul); printf("\n");
    printf("  Result: %d\n", interpret_ast(&mul));

    printf("\n=================================================================\n");
    printf(" Visitor 模式核心：Double Dispatch，数据结构和操作分离\n");
    printf("=================================================================\n");
    return 0;
}
