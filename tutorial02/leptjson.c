#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>  
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;


static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

//重构合并 `lept_parse_null()`、`lept_parse_false()`、`lept_parse_true()` 为 `lept_parse_literal()`。
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type){
    size_t i;
    EXPECT(c, literal[0]);
    for(i=0;i<strlen(literal)-1;i++)
        if(c->json[i]!=literal[i+1]) //EXPECT(c, literal[0]);已经把第一个字母匹配了，c->json[0]指向第二个字母
            return LEPT_PARSE_INVALID_VALUE;
    c->json=c->json+i;
    v->type=type;
    return LEPT_PARSE_OK;
}
//注意在 C 语言中，数组长度、索引值最好使用 size_t 类型，而不是 int 或 unsigned。
//当 int（有符号）与 size_t（无符号）比较时，编译器会将 int 隐式转换为 size_t，若 int 为负数，
//转换后会变成极大的正数，导致逻辑错误（例如 i < strlen(s) 中，若 i 是负的 int，会被误判为 “真”）。

/*解析 JSON 数字的函数 lept_parse_number()，
它使用 C 标准库函数 strtod() 来将字符串转换为双精度浮点数（double）。
static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    \TODO validate number 
    v->n = strtod(c->json, &end);  // 转换字符串为double
    if (c->json == end)  // 若未转换任何字符（转换失败）
        return LEPT_PARSE_INVALID_VALUE;
    c->json = end;  // 更新解析指针到未转换的位置
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}
double strtod(const char *nptr, char **endptr);
1.nptr：指向待转换的字符串（以 null 结尾）。
2.endptr：输出参数，用于存储转换结束后第一个未被转换的字符的地址。如果转换失败，endptr 会指向原始字符串的起始位置（即 nptr）。
返回值:
转换成功时，返回转换后的 double 类型数值。
转换失败时（如字符串无法解析为数字），返回 0.0，并将 endptr 指向 nptr。
若转换结果超出 double 类型的表示范围，会设置 errno 为 ERANGE，并返回 HUGE_VAL（正溢出）或 -HUGE_VAL（负溢出）*/

//上一个仅通过 strtod 转换结果判断是否有有效数字，不主动校验 JSON 数字的语法规范。
//会接受 strtod 支持但不符合 JSON 标准的格式（如 0123 前导零、.123 无整数部分、123. 无小数部分、INF/NAN 等）。
//现改为手动验证数字格式，再调用 strtod 转换，确保符合 JSON 规范。
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p=c->json;
    if(*p=='-') p++;
    if(*p=='0'){
        p++;
        //if(ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE; //这句处理0123为无效数字的情况
        //0x123 这会检查解析完数字后，后面还有字符（x123），
        //lept_parse_number认为没有到'\0',就返回 LEPT_PARSE_ROOT_NOT_SINGULAR。
    }
    else{
        if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;//处理负号后的第一个字符不是数字的情况或者第一个是+
        for(p++;ISDIGIT(*p);p++);
    }
    if(*p=='.'){
        p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++;ISDIGIT(*p);p++);
    }
    if(*p=='e' || *p=='E'){
        p++;
        if(*p=='+' || *p=='-') p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++;ISDIGIT(*p);p++);
    }
    errno=0;
    v->n=strtod(c->json,NULL);
    if(errno==ERANGE&&(v->n==HUGE_VAL||v->n==-HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type=LEPT_NUMBER;
    c->json=p;
    return LEPT_PARSE_OK;
}
//对于0123 把123传给c->json 然后会因为不是‘\0’ 判断为 LEPT_PARSE_ROOT_NOT_SINGULAR

// HUGE_VAL 在 <math.h> 中定义的宏，表示一个 “大到超出 double 类型正常范围” 的特殊值（通常对应正无穷大）。
// 变体：HUGE_VALF：对应 float 类型的 “巨大值”（单精度）。HUGE_VALL：对应 long double 类型的 “巨大值”（长双精度）。
// 用途：当浮点数运算或转换溢出（结果超出类型能表示的最大值）时，函数会返回 HUGE_VAL（正溢出）或 -HUGE_VAL（负溢出）。
// 示例：strtod("1e400", NULL) 会返回 HUGE_VAL（因 1e400 超过 double 最大值）。
// ERANGE ：在 <errno.h> 中定义的宏（一个整数常量），表示 “范围错误”（Out of range）。
// 触发场景：当函数的输入或计算结果超出其处理范围时，errno 会被设置为 ERANGE，

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        // case 't':  return lept_parse_true(c, v);
        // case 'f':  return lept_parse_false(c, v);
        // case 'n':  return lept_parse_null(c, v);
        case 't':  return lept_parse_literal(c, v,"true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v,"false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v,"null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

//仅当 type == LEPT_NUMBER 时，n 才表示 JSON 数字的数值。所以获取该值的 API 是这么实现的：
double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
