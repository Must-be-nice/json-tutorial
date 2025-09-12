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
// JSON 不一样 —— 它允许字符串里出现空字符（比如 "\u0000" 就是 JSON 里的空字符，解析后是 '\0'）。
// 比如 JSON 字符串 "Hello\u0000World"，解析后实际是 H e l l o \0 W o r l d（共 11 个字符）。
// 如果用 C 语言的 “空结尾字符串” 存这个结果，问题就来了：
// C 程序看到中间的 '\0'，会误以为 “字符串到这就结束了”，后面的 World 会被直接忽略，只能读出 Hello，这就错了！
// 既然 '\0' 靠不住，那我们就 “主动记录字符串的真实长度”—— 不管中间有没有 '\0'，只要知道长度，就能准确读取所有字符。
// typedef struct {
//     union {  // 匿名union：没有名字（去掉了u）
//         struct { char* s; size_t len; };  // 匿名struct：没有名字（去掉了s）
//         double n;
//     };  // 匿名union的结尾，不用写名字
//     lept_type type;
// } lept_value;
//  访问时直接用 v->s、v->len、v->n，超简洁！(原来要写 v->u.n, v->u.s.s 和 v->u.s.len)

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,//字符串缺少闭合的双引号
    LEPT_PARSE_INVALID_STRING_ESCAPE,//字符串中存在无效的转义序列（JSON 只支持 9 种转义，如 \a 是非法的）
    LEPT_PARSE_INVALID_STRING_CHAR//字符串中存在无效的字符（比如 ASCII 0x00-0x1F 范围内的控制字符是非法的）
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v);

lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)//由于 lept_free() 实际上也会把 v 变成 null 值，我们只用一个宏来提供 lept_set_null() 这个 API。

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

#endif /* LEPTJSON_H__ */

// 转义组合	实际表示的内容	生活例子（用在字符串里）
// \"	双引号 "	想写 a"b → 写成 "a\"b"
// \\	反斜线 \	想写 C:\文件夹 → 写成 "C:\\文件夹"
// \n	换行（按一下回车的效果）	想写 “第一行 \n 第二行” → 显示时会自动换行
// \t	制表符（按一下 Tab 的效果）	想对齐文字 → 用 "姓名\t年龄" 会空出一段距离
// \r	回车（老设备里用，比如记事本）	现在用得少，知道就行
// \b	退格（按一下 Backspace 的效果）	比如想删一个字符 → 用 "abc\bd" 会变成 "abd"
// \/	正斜线 /	想写 https://www.baidu.com → 写成 "https:\/\/www.baidu.com"（其实不转义也能用，但规范里建议转义）
// \uXXXX	特殊符号（比如 emoji、生僻字）	比如 \u263A 代表笑脸 😊（XXXX 是 16 进制代码，暂时不用深究，知道有这功能就行）