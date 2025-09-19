#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct lept_value lept_value;
//由于 lept_value 内使用了自身类型的指针，我们必须前向声明（forward declare）此类型。

struct lept_value {
    union {
        struct { lept_value* e; size_t size; }a;    /* array:  elements, element count */
        struct { char* s; size_t len; }s;           /* string: null-terminated string, string length */
        double n;                                   /* number */
    }u;
    lept_type type;
};
//lept_value* e 指向一个 lept_value 类型的数组（数组中的每个元素都是 lept_value 结构体）
// 这个数组中的每个 lept_value 元素可以存储 JSON 支持的任意类型

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
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET//语法错误（缺少分隔符）
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

#endif /* LEPTJSON_H__ */

// JSON 数组的语法规则非常明确，核心特征如下：
// 包裹符号：用英文方括号 [] 包裹所有元素；
// 元素分隔：多个元素之间用英文逗号 , 分隔；
// 元素类型：支持 JSON 的所有合法数据类型，包括：
// 基本类型：数字（整数 / 浮点数）、字符串（需用双引号 " 包裹）、布尔值（true/false）、null；
// 复杂类型：JSON 对象（{} 包裹）、其他 JSON 数组（嵌套数组）。
// 注意事项：
// 数组内不允许有 trailing comma（末尾逗号）（如 [1,2,] 是非法的）；
// 字符串必须用双引号（单引号 ' 非法，如 ['a'] 不合法）；
// 元素顺序会被保留（JSON 数组是有序结构）。

// JSON 数组存储零至多个元素，最简单就是使用 C 语言的数组。数组最大的好处是能以 O(1)用索引访问任意元素，
// 次要好处是内存布局紧凑，省内存之余还有高缓存一致性（cache coherence）。
// 但数组的缺点是不能快速插入元素，而且我们在解析 JSON 数组的时候，还不知道应该分配多大的数组才合适。

// 另一个选择是链表（linked list），它的最大优点是可快速地插入元素（开端、末端或中间），
// 但需要以 O(n)时间去经索引取得内容。如果我们只需顺序遍历，那么是没有问题的。
// 还有一个小缺点，就是相对数组而言，链表在存储每个元素时有额外内存开销（存储下一节点的指针），而且遍历时元素所在的内存可能不连续，令缓存不命中（cache miss）的机会上升。
//  当访问的数据不在缓存中，需要从外部存储加载数据时，就会发生 cache miss。
// 我见过一些 JSON 库选择了链表，而这里则选择了数组。我们将会通过之前在解析字符串时实现的堆栈，来解决解析 JSON 数组时未知数组大小的问题。