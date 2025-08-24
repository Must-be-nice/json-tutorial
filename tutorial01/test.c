#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

// 全局变量：记录测试结果状态
static int main_ret = 0; // 程序最终返回值（0表示全部通过，1表示有失败）
static int test_count = 0;// 总测试用例数
static int test_pass = 0;// 通过的测试用例数

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)
//若宏的替换文本有多行，每行末尾需加 \（反斜杠）连接（最后一行除外）。
//在宏定义中使用 do { ... } while(0) 包裹代码，核心目的不是为了 “循环执行”（它确实只执行一次），
//而是为了让宏在各种场景下都能符合 C 语言的语法规则，避免因宏的使用方式不同而导致编译错误或逻辑错误。
// 避免分支语句中的逻辑错误
// 例如定义一个宏：
// #define LOG(msg) printf("Log: %s\n", msg); fflush(stdout);
// 当在 if 语句中使用时：
// if (flag)
//     LOG("success");  // 展开后会导致 fflush 不受 if 控制
//BASE 表示这是一个基础宏（通常被其他宏间接调用）。

// fprintf(stderr, ...)
// fprintf 是格式化输出函数，第一个参数指定输出目标，这里是 stderr（标准错误流）。
// 与 stdout（标准输出流）不同，stderr 通常用于输出错误信息，且默认不缓冲（即时显示），适合调试和错误提示。
// printf：只能向标准输出流（stdout） 输出内容，通常对应屏幕（控制台）。
// 函数原型：int printf(const char *format, ...);
// 示例：printf("Hello World"); // 直接打印到屏幕
// fprintf：可以向指定的输出流输出内容，输出目标由第一个参数指定。
// 函数原型：int fprintf(FILE *stream, const char *format, ...);
// FILE *file = fopen("output.txt", "w");
// fprintf(file, "Hello File");  输出到文件
// fprintf(stderr, "Error!");    输出到标准错误流（通常也是屏幕，但用途不同）

// __FILE__：C 语言预定义宏，自动替换为当前源代码的文件名（字符串类型）。
// __LINE__：C 语言预定义宏，自动替换为当前代码所在的行号（整数类型）。
// expect：测试期望的结果（如预期的返回码 LEPT_PARSE_OK）。
// actual：测试实际得到的结果（如解析器实际返回的状态码）。

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
//EXPECT_EQ_INT：针对整数的断言宏，复用EXPECT_EQ_BASE，指定比较方式为(expect) == (actual)，格式化字符串为%d（整数格式）

// static限制全局变量的作用域仅为当前源文件（.c 文件），其他源文件无法访问。
// 验证解析器能否正确识别null关键字，并返回正确的状态码和类型。
static void test_parse_null() {
    lept_value v;
    v.type = LEPT_FALSE;// 初始化类型（避免未定义行为）
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null")); 
    //解析后的值类型应是LEPT_NULL（对应JSON的null）
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

//验证解析器在输入为空或只有空白字符时，能否正确返回 “缺少值” 的错误。
static void test_parse_expect_value() {
    lept_value v;

    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, ""));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, " "));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

//验证解析器对无效输入（如不完整关键字、非法字符）的处理能力。
static void test_parse_invalid_value() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "nul"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "?"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

//JSON 规范要求整个文档只能有一个根值（如null、对象、数组等）。此测试验证解析器能否检测到根值后多余的字符。
static void test_parse_root_not_singular() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

//true/false 单元测试
static void test_parse_true(){
    lept_value v;
    v.type=LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK,lept_parse(&v,"true"));
    EXPECT_EQ_INT(LEPT_TRUE,lept_get_type(&v));
}
static void test_parse_false(){
    lept_value v;
    v.type=LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK,lept_parse(&v,"false"));
    EXPECT_EQ_INT(LEPT_FALSE,lept_get_type(&v));
}

static void test_parse() {
    test_parse_null();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_true();
    test_parse_false();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
//(%3.2f%%)：
//%3.2f  以浮点数格式输出“通过率”，其中：
//.2 表示保留2位小数；
//3 表示整个数字（包括整数和小数部分）至少占3个字符宽度（不足时补空格，通常用于对齐）。
//%%  输出一个 `%` 符号（`%` 是格式化符的特殊字符，需用 `%%` 转义）。
