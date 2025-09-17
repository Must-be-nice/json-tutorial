#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;

static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
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

static const char* lept_parse_hex4(const char* p, unsigned* u) {
    /* \TODO */
    *u=0;
    for(int i=0;i<4;i++){
        char ch=*p++;
        *u<<=4;
        if(ch>='0'&&ch<='9') *u|=ch-'0';
        else if(ch>='a'&&ch<='f') *u|=(unsigned)ch-'a'+10;
        else if(ch>='A'&&ch<='F') *u|=(unsigned)ch-'A'+10;
        else return NULL;
    }
    return p;
}
// 循环读取 4 个字符，每次处理一个十六进制字符：
// 将当前结果左移 4 位（相当于乘以 16）
// 将字符转换为对应的十六进制数值（0-15）
// 通过按位或运算将新数值拼接到结果中
// 若遇到非十六进制字符（非 0-9、A-F、a-f），返回 NULL 表示失败

// static const char* lept_parse_hex4(const char* p, unsigned* u){
//     // // 先检查第一个字符是否为合法十六进制字符（非空白且在0-9/A-F/a-f范围内）
//     if (!((p[0] >= '0' && p[0] <= '9') || 
//           (p[0] >= 'A' && p[0] <= 'F') || 
//           (p[0] >= 'a' && p[0] <= 'f'))) {
//         return NULL;
//     }
//     char* end;
//     *u = (unsigned)strtol(p, &end, 16);
//     return end == p + 4 ? end : NULL;
// }
// strtol 将字符串 nptr 按照指定的进制（base）转换为长整数，
// 并通过 endptr 返回转换终止的位置（即第一个无法转换的字符地址）。
//但这个实现会错误地接受 "\u 123" 这种不合法的 JSON，因为 strtol() 会跳过开始的空白。
//所以还需要检测第一个字符是否 [0-9A-Fa-f]

static void lept_encode_utf8(lept_context* c, unsigned u) {
    /* \TODO */
    if(u<=0x7F)
        PUTC(c,u&0xFF);
    else if(u<=0x7FF){
        PUTC(c,(unsigned char)0xC0|((u>>6)&0x1F));
        PUTC(c,(unsigned char)0x80|(u&0x3F));
    }
    else if(u<=0xFFFF){
        PUTC(c,(unsigned char)0xE0|(u>>12)&0x0F);
        PUTC(c,(unsigned char)0x80|((u>>6)&0x3F));
        PUTC(c,(unsigned char)0x80|(u&0x3F));
    }
    else{
        assert(u<=0x10FFFF);//U+10FFFF是目前Unicode的最大码点
        PUTC(c,(unsigned char)0xF0|((u>>18)&0x07));
        PUTC(c,(unsigned char)0x80|((u>>12)&0x3F));
        PUTC(c,(unsigned char)0x80|((u>>6)&0x3F));
        PUTC(c,(unsigned char)0x80|(u&0x3F));
    }
}
// UTF-8 采用 “前缀标识 + 数据位” 的编码方式，不同范围的码点使用不同长度的字节序列：
// 1 字节：用于 ASCII 字符（0x00 ~ 0x7F），前缀为 0xxxxxxx；
// 2 字节：用于 0x080 ~ 0x7FF，前缀为 110xxxxx 10xxxxxx；(这个范围里最大的码点只有 11 位二进制，并不是 12 位。)
// 3 字节：用于 0x0800 ~ 0xFFFF，前缀为 1110xxxx 10xxxxxx 10xxxxxx；
// 4 字节：用于 0x010000 ~ 0x10FFFF，前缀为 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx。
// 其中，x 表示实际数据位，函数通过位运算提取码点的对应比特位，拼接上前缀后生成 UTF-8 字节。

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)\
//简单的重构，把返回错误码的处理抽取为宏

static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                len = c->top - head;
                lept_set_string(v, (const char*)lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    case 'u':
                        if (!(p = lept_parse_hex4(p, &u)))
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        /* \TODO surrogate handling */
                        if(u>=0xD800&&u<=0xDBFF){
                            if(*p++!='\\')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if(*p++!='u')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = lept_parse_hex4(p, &u2)))
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            u = 0x10000 + (u-0xD800) * 0x400 + (u2-0xDC00);
                            //位运算版本：u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        lept_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
            default:
                if ((unsigned char)ch < 0x20)
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
                PUTC(c, ch);
        }
    }
}
//处理 \uXXXX 形式的 Unicode 转义序列：
// lept_parse_hex4(p, &u) 解析后续 4 位十六进制数字（0-9、a-f、A-F），转换为 Unicode 码点 u（如 \u4E2D 解析为 u=0x4E2D）。
// 若 4 位字符不是十六进制数，返回 “无效 Unicode 十六进制” 错误。
// lept_encode_utf8 将码点 u 转换为 UTF-8 编码的字节序列，写入缓冲区（供后续作为字符串存储）。
// TODO 注释说明：当前未处理 “代理对”（超出 BMP 的码点，需两个 \uXXXX 表示），仅支持 BMP 内的码点（U+0000~U+FFFF）。

//背景：Unicode 中，U+10000~U+10FFFF 的字符无法用单个 \uXXXX 表示（XXXX 最大为 FFFF），需用 两个 16 位代理项 组合表示：
// 高代理项（High Surrogate）：U+D800~U+DBFF；
// 低代理项（Low Surrogate）：U+DC00~U+DFFF。
//我们用下列公式把代理对 (H, L) 变换成真实的码点：
//codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\"':  return lept_parse_string(c, v);
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
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
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

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
