#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct {
    double n;
    lept_type type;
}lept_value;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG
};

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

double lept_get_number(const lept_value* v);

#endif /* LEPTJSON_H__ */

// JSON NUMBER 语法规则可概括为：
// 由可选负号 - 开头，必选整数部分（要么是单个 0，要么是非零数字开头后跟任意数字，不允许前导零），
// 可接可选小数部分（. 加至少一位数字），
// 可接可选指数部分（e 或 E 开头，后跟可选正负号及至少一位数字）；
// 不允许正号、八 / 十六进制、特殊值（如 Infinity（无穷大）、NaN（非数字））及数字分隔符。