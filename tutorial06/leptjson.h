#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct lept_value lept_value;
typedef struct lept_member lept_member;
// 观察代码可知，struct lept_value 中包含 struct lept_member* 类型的成员（用于表示 JSON 对象的键值对），而 struct lept_member 中又包含 lept_value 类型的成员（用于表示键对应的值）。
// 这种两个结构体相互引用的场景下，必须先通过 typedef 为结构体声明别名，才能在对方的定义中使用该别名，否则编译器会因 “未声明的类型” 而报错。

struct lept_value {
    union {
        struct { lept_member* m; size_t size; }o;   /* object: members, member count */
        struct { lept_value* e; size_t size; }a;    /* array:  elements, element count */
        struct { char* s; size_t len; }s;           /* string: null-terminated string, string length */
        double n;                                   /* number */
    }u;
    lept_type type;
};

struct lept_member {
    char* k; size_t klen;   /* member key string, key string length */
    // klen 记录字符串的实际长度（不含结尾的 \0），避免每次获取长度时都需要调用 strlen() 计算，提升效率。
    lept_value v;           /* member value */
};

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    LEPT_PARSE_MISS_KEY, //缺少对象的键（key）
    LEPT_PARSE_MISS_COLON, //缺少键值对中的冒号
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET //缺少逗号或右大括号
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

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

size_t lept_get_object_size(const lept_value* v); //获取 JSON 对象中包含的键值对数量。
const char* lept_get_object_key(const lept_value* v, size_t index); //获取指定索引位置的键字符串。
size_t lept_get_object_key_length(const lept_value* v, size_t index); //获取指定索引位置的键字符串长度。
lept_value* lept_get_object_value(const lept_value* v, size_t index); //获取指定索引位置的值。

#endif /* LEPTJSON_H__ */
// JSON 对象数据格式的语法要求可概括为：
// 整体需用大括号 {} 包裹，内部由 “键值对” 组成；
// 键必须是双引号包裹的字符串，键与值之间用冒号 : 连接；
// 值可采用 JSON 支持的任意类型（字符串、数字、布尔值、null、对象、数组）；
// 多个键值对之间用逗号 , 分隔且末尾键值对不能加尾逗号；
// 键值对内部、逗号后可灵活添加空格或换行以优化格式，但需确保所有大括号成对闭合，
// 且实际应用中每个键都唯一（即不能重复）。