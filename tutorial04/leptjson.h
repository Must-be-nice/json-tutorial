#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct {
    union {
        struct { char* s; size_t len; }s;  /* string: null-terminated string, string length */
        double n;                          /* number */
    }u;
    lept_type type;
}lept_value;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,//高代理项而欠缺低代理项，或是低代理项不在合法码点范围
    LEPT_PARSE_INVALID_UNICODE_SURROGATE//如果 \u 后不是 4 位十六进位数字
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v);

lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

#endif /* LEPTJSON_H__ */
//由于 UTF-8 的普及性，大部分的 JSON 也通常会以 UTF-8 存储。我们的 JSON 库也会只支持 UTF-8。（RapidJSON 同时支持 UTF-8、UTF-16LE/BE、UTF-32LE/BE、ASCII。）

// C 标准库没有关于 Unicode 的处理功能（C++11 有），我们会实现 JSON 库所需的字符编码处理功能。

// 对于非转义（unescaped）的字符，只要它们不少于 32（0 ~ 31 是不合法的编码单元），我们可以直接复制至结果，这一点我们稍后再说明。我们假设输入是以合法 UTF-8 编码。

// 而对于 JSON字符串中的 \uXXXX 是以 16 进制表示码点 U+0000 至 U+FFFF，我们需要：

// 解析 4 位十六进制整数为码点；
// 由于字符串是以 UTF-8 存储，我们要把这个码点编码成 UTF-8。
// 同学可能会发现，4 位的 16 进制数字只能表示 0 至 0xFFFF，但之前我们说 UCS 的码点是从 0 至 0x10FFFF，那怎么能表示多出来的码点？

// 其实，U+0000 至 U+FFFF 这组 Unicode 字符称为基本多文种平面（basic multilingual plane, BMP），还有另外 16 个平面。
// 那么 BMP 以外的字符，JSON 会使用代理对（surrogate pair）表示 \uXXXX\uYYYY。在 BMP 中，保留了 2048 个代理码点。如果第一个码点是 U+D800 至 U+DBFF，
// 我们便知道它的代码对的高代理项（high surrogate），之后应该伴随一个 U+DC00 至 U+DFFF 的低代理项（low surrogate）。然后，我们用下列公式把代理对 (H, L) 变换成真实的码点：

// codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
// 举个例子，高音谱号字符 𝄞 → U+1D11E 不是 BMP 之内的字符。在 JSON 中可写成转义序列 \uD834\uDD1E，我们解析第一个 \uD834 得到码点 U+D834，
// 我们发现它是 U+D800 至 U+DBFF 内的码点，所以它是高代理项。
// 然后我们解析下一个转义序列 \uDD1E 得到码点 U+DD1E，它在 U+DC00 至 U+DFFF 之内，是合法的低代理项。我们计算其码点：
// H = 0xD834, L = 0xDD1E
// codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
//           = 0x10000 + (0xD834 - 0xD800) × 0x400 + (0xDD1E − 0xDC00)
//           = 0x10000 + 0x34 × 0x400 + 0x11E
//           = 0x10000 + 0xD000 + 0x11E
//           = 0x1D11E
// 这样就得出这转义序列的码点，然后我们再把它编码成 UTF-8。
// 如果只有高代理项而欠缺低代理项，或是低代理项不在合法码点范围，我们都返回 LEPT_PARSE_INVALID_UNICODE_SURROGATE 错误。
// 如果 \u 后不是 4 位十六进位数字，则返回 LEPT_PARSE_INVALID_UNICODE_HEX 错误。

// BMP 外字符的解析 - 存储流程是：
// 两个\u代理对 → 计算真实码点 → 编码为UTF-8多字节 → 写入char字符串（字节数组）