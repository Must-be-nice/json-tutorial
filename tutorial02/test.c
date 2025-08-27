#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

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

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
// 在 C 语言的格式化输出中，"%.17g" 是一个用于浮点数的格式控制字符串，它的作用是：
// g 表示采用最短表示法输出浮点数，会根据数值大小自动选择小数形式（如 123.45）或科学计数法（如 1.2345e+2），去掉不必要的尾部零。
// .17 表示最多保留 17 位有效数字（不是小数点后 17 位）。

static void test_parse_null() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_true() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}

// 宏：简化数字测试代码（解析JSON字符串，验证成功且值正确）
#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
    } while(0)

// 覆盖各种数字格式：整数、负数、小数、科学计数法、极小数（下溢为0）等
static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
    //“极小数（下溢为 0）” 指的是：当 JSON 中的数字过小（
    //超出双精度浮点数double能表示的最小非零正值范围）时，解析器会将其自动处理为0.0的现象

    //以下是一些边界值测试，确保解析器在极限情况下仍能正确处理
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

//test_parse_invalid_value() 中我们每次测试一个不合法的 JSON 值，
//都有 4 行相似的代码。我们可以把它用宏的方式把它们简化：
#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");

#if 1  // 条件编译：当前注释掉这部分测试（不执行）
    /* invalid number */
    // 以下是无效数字的测试用例
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");    // 正数不能带+号（JSON规范）
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");    // 同上
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123");  // 小数点前必须有数字
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");    // 小数点后必须有数字
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");   // 不支持无穷大
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");   // 不支持非数字
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
#endif
}
// f:\vscode_project\json-tutorial\tutorial02\test.c:105: expect: 2 actual: 0
// f:\vscode_project\json-tutorial\tutorial02\test.c:105: expect: 0 actual: 3
// strtod会忽略+号和前导小数点 .123会被转换为 0.123（自动补全整数部分的 0 (JSON 不允许这种格式，但 strtod 支持)
// INF（无穷大）和 NAN（非数字），虽然 NAN 的字面意思是 “非数字”，并且不区分大小写
// 但在 C 标准中，它们被定义为合法的浮点数值格式，
// strtod 会将其解析为对应的浮点值（HUGE_VAL 或 NaN），而非 “非数字字符”。

static void test_parse_root_not_singular() {
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");
    //测试 “根值后有多余字符” 的情况
#if 1
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); // 0后不能直接跟数字（应为.、E/e或结束）
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
#endif
}
// 0123被解析为0，后面跟着123，因此报错 
// strtod无法识别十六进制数字,会将0x0解析为0，并将解析指针end指向x，因此报错

// 超过双精度浮点数范围的数字，预期错误：LEPT_PARSE_NUMBER_TOO_BIG
static void test_parse_number_too_big() {
#if 1
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
