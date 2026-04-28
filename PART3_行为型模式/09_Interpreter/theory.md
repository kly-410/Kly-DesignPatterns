# Interpreter Pattern — 解释器模式

## 1. 模式定义

**解释器模式（Interpreter Pattern）**：给定一个语言，定义它的文法的一种表示，以及一个解释器。

> 核心思想：**文法 → 对象结构 → 递归求值**。

---

## 2. 《Head First 设计模式》核心内容

```cpp
class Expression {
public:
    virtual int interpret(context) = 0;
};

class AddExpression : public Expression {
    Expression* left;
    Expression* right;
public:
    int interpret(context) override {
        return left->interpret() + right->interpret();
    }
};
```

---

## 3. Linux 内核实例

Shell 命令解析使用递归下降解析器。

---

## 4. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 定义文法表示，解释语言中的句子 |
| **核心** | 表达式树 + 递归求值 |
| **Linux 场景** | Shell 解析、proc 文件 |
