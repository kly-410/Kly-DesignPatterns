#ifndef ITRAVEL_STRATEGY_H
#define ITRAVEL_STRATEGY_H

/*
 * ITRAVEL_STRATEGY — 出行策略接口
 * 
 * 策略接口：所有出行策略必须实现 travel 函数
 * 简化版：不传目的地，策略内部自己决定去哪里
 */

typedef struct _ITravelStrategy ITravelStrategy;

struct _ITravelStrategy
{
    void (*travel)(ITravelStrategy *strategy);
};

#endif // ITRAVEL_STRATEGY_H
