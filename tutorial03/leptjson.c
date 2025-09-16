#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#endif
#include <crtdbg.h>

// 宏定义：堆栈初始大小（可由用户在编译时自定义）
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack;       // 动态分配的缓冲区（堆栈的内存空间）
    size_t size;       // 当前堆栈的总容量（能存储多少字节）
    size_t top;        // 栈顶位置（已使用的字节数，初始为 0）
}lept_context;

// 4. 堆栈的核心操作：压入（lept_context_push）与弹出（lept_context_pop）
// 这两个函数实现了动态堆栈的 “存数据” 和 “取数据”，并处理空间不足时的扩展。
// 功能：向堆栈压入 size 字节的数据，返回数据的起始地址（为新数据 “预留空间” 并返回空间地址）
static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) { // 检查空间是否足够：当前栈顶 + 要压入的大小 >= 总容量 → 空间不足，需要扩展
        if (c->size == 0) // 首次分配：用初始大小
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)// 循环扩展：每次按 1.5 倍增大（直到容量足够）
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size); 
        // 重新分配内存（realloc：保留原有数据，扩展到新容量）
        // 注：realloc(NULL, size) 等价于 malloc(size)，所以首次分配无需特殊处理
    }
    ret = c->stack + c->top;// 计算压入数据的起始地址（栈顶当前位置）
    c->top += size;  // 栈顶后移（更新已使用大小）
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);// 确保栈顶位置 >= 要弹出的大小（否则栈为空或数据不足）
    c->top -= size;  // 栈顶前移（相当于“弹出”）
    return c->stack + c->top;  // 返回弹出数据的起始地址
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}

// static int lept_parse_string(lept_context* c, lept_value* v) {
//     size_t head = c->top, len;
//     const char* p;
//     EXPECT(c, '\"');
//     p = c->json;
//     for (;;) {//无限循环，直到遇到结尾 " 或错误才退出
//         char ch = *p++;
//         switch (ch) {
//             case '\"':// 读到结尾的 "，表示字符串解析完成
//                 len = c->top - head;
//                 lept_set_string(v, (const char*)lept_context_pop(c, len), len);
//                 c->json = p;
//                 return LEPT_PARSE_OK;
//             case '\0':// 读到空字符（JSON 字符串提前结束，没有正常的结尾 "）
//                 c->top = head;
//                 return LEPT_PARSE_MISS_QUOTATION_MARK;
//             default:
//                 PUTC(c, ch);
//         }
//     }
// }
// 注意：这段代码是简化版，实际解析器还需要处理：
// 转义字符（如 \"、\n、\\ 等）：需要识别 \ 开头的转义序列，转换为对应字符（如 \n 转为换行符）；
// 非法字符（如 ASCII 0-31 的控制字符）：JSON 不允许这些字符直接出现，需返回 LEPT_PARSE_INVALID_STRING_CHAR 错误。
//c->top = head; 就是把堆栈“回退”到解析字符串前的位置，清除本次解析的临时数据，保证堆栈状态正确。
//但在解析过程中，堆栈是复用的，可能会解析多个字符串或其它元素，每次都用同一个堆栈。
//如果不回退 c->top，下次解析时堆栈顶位置不对，可能导致数据错乱。

static int lept_parse_string(lept_context*c,lept_value* v){
    size_t head=c->top,len;
    const char* p;
    EXPECT(c,'\"');
    p=c->json;
    for(;;){
        char ch=*p++;
        switch(ch){
            case '\"':
                len=c->top-head;
                lept_set_string(v,(const char*)lept_context_pop(c,len),len);
                c->json=p;
                return LEPT_PARSE_OK;
            case '\0':
                c->top=head;
                return LEPT_PARSE_MISS_QUOTATION_MARK;
            case '\\':
                switch(*p++){
                    case '\\':PUTC(c,'\\');break;
                    case '\"':PUTC(c,'\"');break;
                    case '/': PUTC(c,'/'); break;
                    case 'b': PUTC(c,'\b'); break;
                    case 'f': PUTC(c,'\f'); break;
                    case 'n': PUTC(c,'\n'); break;  
                    case 'r': PUTC(c,'\r'); break;
                    case 't': PUTC(c,'\t'); break;
                    default:
                        c->top=head;
                        return LEPT_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            default:
                if((unsigned char)ch<0x20){
                    c->top=head;
                    return LEPT_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c,ch);
        }
    }    
}
//在 JSON 标准中，字符串不能包含 ASCII 码小于 0x20（即 0~31）的控制字符（如回车、换行、制表符等），否则就是非法字符串。
//允许的控制字符：\n、\r、\t、\b、\f（必须用反斜杠转义）
//目前解析器还没有处理 \uXXXX 这种 Unicode 转义序列。

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);  // 断言：解析结束后栈顶必须为 0（所有临时数据都已弹出）
    free(c.stack);       // 释放堆栈缓冲区（避免内存泄漏）
    return ret;
}

//作用是安全释放 lept_value 所占用的动态内存，避免内存泄漏
void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}
// 释放内存后，将 v 的类型强制设为 LEPT_NULL（空类型），有两个关键作用：
// 避免野指针访问：如果后续误操作访问 v 的原字符串成员（v->u.s.s），由于类型已改为 LEPT_NULL，可以通过类型检查（如 if (v->type == LEPT_STRING)）避免访问已释放的野指针。
// 明确状态：标记 v 处于 “空状态”，符合内存释放后的逻辑一致性。

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    /* \TODO */
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    /* \TODO */
    assert(v != NULL);
    lept_free(v);
    v->type = (b ? LEPT_TRUE : LEPT_FALSE);
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    /* \TODO */
    assert(v != NULL);
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

//作用是将指定的字符串数据赋值给 lept_value 结构体（用于存储 JSON 解析结果），并确保内存管理的安全性。
void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);//+1 是为了在末尾添加 '\0'，兼容 C 语言的空结尾字符串。
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
// 使用 memcpy 按字节拷贝：从源字符串 s 拷贝 len 个字节到新分配的内存 v->u.s.s。
// 为什么用 memcpy 而不是 strcpy？
// 因为 s 可能包含 '\0' 字符（JSON 字符串允许中间有 '\0'），strcpy 会遇到 '\0' 就停止拷贝，而 memcpy 严格按 len 字节拷贝，确保完整复制。
// 在拷贝的有效内容后添加 '\0'，让 v->u.s.s 同时满足：
// JSON 字符串的真实长度（len）；
// C 语言的空结尾字符串规范（方便不处理 '\0' 的场景直接使用，如 printf("%s", v->u.s.s)）。

// 性能优化的思考
// 这是本教程第一次的开放式问题，没有标准答案。以下列出一些我想到的。
// 一、如果整个字符串都没有转义符，我们不就是把字符复制了两次？第一次是从 json 到 stack，第二次是从 stack 到 v->u.s.s。
// 我们可以在 json 扫描 '\0'、'\"' 和 '\\' 3 个字符（ ch < 0x20 还是要检查），直至它们其中一个出现，才开始用现在的解析方法。
// 这样做的话，前半没转义的部分可以只复制一次。缺点是，代码变得复杂一些，我们也不能使用 lept_set_string()。
// 二、对于扫描没转义部分，我们可考虑用 SIMD 加速，如 RapidJSON 代码剖析（二）：使用 SSE4.2 优化字符串扫描 的做法。这类底层优化的缺点是不跨平台，需要设置编译选项等。
// 三、在 gcc/clang 上使用 __builtin_expect() 指令来处理低概率事件，例如需要对每个字符做 LEPT_PARSE_INVALID_STRING_CHAR 检测，
// 我们可以假设出现不合法字符是低概率事件，然后用这个指令告之编译器，那么编译器可能可生成较快的代码。然而，这类做法明显是不跨编译器，甚至是某个版本后的 gcc 才支持。