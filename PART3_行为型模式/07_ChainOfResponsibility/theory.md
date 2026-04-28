# Chain of Responsibility Pattern — 责任链模式

## 1. 模式定义

**责任链模式（Chain of Responsibility Pattern）**：通过将多个对象连成一条链，并沿着这条链传递请求，直到有一个对象处理它为止。

> 核心思想：**请求发送者与接收者解耦**，请求沿着链传递。

---

## 2. 《Head First 设计模式》核心内容

```cpp
class Handler {
protected:
    Handler* successor;
public:
    void setSuccessor(Handler* h) { successor = h; }
    virtual void handleRequest(int request) = 0;
};

class ConcreteHandler : public Handler {
public:
    void handleRequest(int request) override {
        if (request <= threshold) { /* handle */ }
        else if (successor) successor->handleRequest(request);
    }
};
```

---

## 3. Linux 内核实例

### IRQ Handler Chain

当一个 IRQ 被多个设备共享时，内核依次调用所有注册的处理函数。

---

## 4. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 将请求沿链传递，直到被某个 Handler 处理 |
| **核心** | Handler 决定处理 or 传递给下一个 |
| **Linux 场景** | 共享 IRQ Handler 链、DMA channel |
