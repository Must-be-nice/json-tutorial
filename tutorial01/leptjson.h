#ifndef LEPTJSON_H__
#define LEPTJSON_H__
// #ifndef 是 “if not defined” 的缩写，意思是 “如果 LEPTJSON_H__ 这个宏没有被定义”
// 作用：防止头文件重复包含
// 当一个头文件被多个源文件（或其他头文件）多次 #include 时：
// 第一次包含：LEPTJSON_H__ 未定义，会执行 #define LEPTJSON_H__ 并包含头文件内容。
// 后续包含：LEPTJSON_H__ 已被定义，#ifndef 条件为假，会跳过头文件内容，不重复包含。

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//创建了一个名为lept_type 的类型
// 枚举类型（enum）是一种用户自定义的类型，包含一组命名的整数常量
// 这里定义了七种 JSON 数据类型：空值、布尔值（真和假）、数字、字符串、数组和对象

typedef struct {
    lept_type type;
}lept_value;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,//若一个 JSON 只含有空白，传回 
    LEPT_PARSE_INVALID_VALUE,//若值不是那三种字面值，传回
    LEPT_PARSE_ROOT_NOT_SINGULAR //若一个值之后，在空白之后还有其他字符，传回
};
//在 C 语言的枚举（enum）中，当某个枚举成员没有显式赋值时，
//它的值会自动继承前一个成员的值并加1。这是枚举的默认行为，用于简化连续值的定义。

int lept_parse(lept_value* v, const char* json);
// 作用：解析 JSON 字符串并将结果存储在 lept_value 结构中
//   - lept_value* v：指向 lept_value 结构的指针，用于存储解析结果
//   - const char* json：指向要解析的 JSON 字符串的指针
// 返回值：一个整数，表示解析结果的状态（如成功或各种错误类型）

lept_type lept_get_type(const lept_value* v);
// 作用：获取 lept_value 结构中存储的 JSON 数据类型


#endif /* LEPTJSON_H__ */
// 头文件结束标志,用于结束 #ifndef 条件编译块，标记 “如果宏未定义则执行的代码范围” 到此结束。
// 作用：防止头文件被多次包含，避免重复定义和编译错误
// 当头文件被多次包含时，只有第一次包含会生效，后续的包含会被忽略
// 这样可以提高编译效率并减少潜在的错误


/*下面是此单元的 JSON 语法子集，使用 RFC7159 中的 ABNF 表示：
JSON-text = ws value ws
ws = *(%x20 / %x09 / %x0A / %x0D)
value = null / false / true 
null  = "null"
false = "false"
true  = "true"
1. JSON-text = ws value ws
含义：一个完整的 JSON 文本（JSON-text）由三部分按顺序组成：
开头的 ws（空白）；
中间的 value（JSON 值）；
结尾的 ws（空白）。
举例：
合法的 JSON 文本可以是：
null （前后有空格）；
\ttrue\n（前有制表符 \t，后有换行符 \n）；
false（前后无空白，因为 * 允许零个空白）。
2. ws = *(%x20 / %x09 / %x0A / %x0D)
含义：ws（空白）是由零或多个以下字符组成的序列：
%x20：空格符（空格键输入的空格）；
%x09：水平制表符（\t）；
%x0A：换行符（\n，LF）；
%x0D：回车符（\r，CR）。
作用：JSON 允许在值的前后添加任意空白（包括无空白），不影响语义（例如 true 和 true 是等价的）。
3. value = null / false / true
含义：当前语法子集里，value（JSON 值）只能是以下三种之一：
null：JSON 中的空值；
false：JSON 中的布尔值 “假”；
true：JSON 中的布尔值 “真”。
说明：这是一个简化的子集，完整的 JSON 语法中 value 还包括数字、字符串、数组、对象（对应之前 lept_type 枚举的 7 种类型），这里只聚焦于最简单的三种字面量。
4. null = "null"、false = "false"、true = "true"
含义：明确三种值的字面量形式（必须严格匹配）：
null 必须精确写作字符串 "null"（小写，无其他字符）；
false 必须精确写作 "false"；
true 必须精确写作 "true"。
注意：JSON 对大小写敏感，Null、TRUE 等都属于语法错误（不符合此规则）。*/