#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */

#define EXPECT(c, ch)       do { assert(*(c->json) == (ch)); c->json++; } while(0)
// 在 C 语言中，<assert.h> 头文件提供的 assert() 宏是一个非常实用的调试工具，主要用于在程序运行时检查必须为真的条件。
//如果条件不满足，它会触发错误并终止程序，同时输出详细的错误信息（如文件名、行号和失败的表达式），帮助开发者快速定位问题。
// 当程序运行到 assert(条件) 时，会先判断 “条件表达式” 是否为真（非 0）：
// 若为真：程序继续执行，assert 不产生任何影响。
// 若为假：程序立即终止，并在标准错误流（stderr）输出类似以下的信息：assertion failed: (条件), file 文件名.c, line 行号
// 这里的 assert(*c->json == (ch)) 用于验证 “当前解析的字符是否等于预期字符”。例如，解析 null 时，第一个字符必须是 'n'，EXPECT(c, 'n') 会通过 assert 检查这一点：
// 如果当前字符确实是 'n'：程序继续执行，指针后移。
// 如果不是 'n'：assert 触发错误，提示断言失败，同时显示文件名和行号，帮助开发者发现解析逻辑的问题（比如输入了非法的 JSON 格式）。

typedef struct {
    const char* json;
}lept_context;

//空白字符处理函数
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')// 跳过所有空白字符（空格、制表符、换行符、回车符）
        p++;
    c->json = p;// 更新解析指针到空白字符后的第一个有效字符
}

//解析 JSON 中的null关键字。
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');// 首先检查首字符是否为'n'（null的第一个字符）
    // 检查剩余字符是否为"ull"（组成完整的"null"）
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;// 不完整则返回"无效值"错误
    c->json += 3;  // 指针向后移动3位（跳过"ull"）
    v->type = LEPT_NULL;  // 将解析结果类型设为NULL
    return LEPT_PARSE_OK;  // 返回"解析成功"状态
}

//根据当前字符决定解析哪种 JSON 值（是null、true、false还是其他类型）。
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*(c->json)) {
        case 'n':  return lept_parse_null(c, v);  // 若当前字符是'n'，则解析为null
        case '\0': return LEPT_PARSE_EXPECT_VALUE; // 若已到字符串末尾，返回"预期值"错误
        default:   return LEPT_PARSE_INVALID_VALUE; // 其他字符，返回"无效值"错误
    }
}

//解析入口函数，协调整个解析流程。
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);  // 确保输出结果指针v不为NULL（避免空指针访问）
    c.json = json;      // 初始化解析上下文，指向JSON字符串起始位置
    v->type = LEPT_NULL;  // 初始化结果类型为NULL
    lept_parse_whitespace(&c);  // 跳过开头的空白字符
    return lept_parse_value(&c, v);  // 解析实际的值并返回结果状态
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);  // 确保输入指针v不为NULL
    return v->type;     // 返回值的类型（如LEPT_NULL）
}
