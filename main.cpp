#include "alisp.hpp"
#include <cassert>
#include <cstdint>
#include <set>

void ASSERT_EQ(std::string a, std::string b)
{
    if (a == b) {
        return;
    }
    std::cerr << "Was expecting " << b << " but got " << a << std::endl;
    assert(false);
}

void ASSERT_EQ(const std::unique_ptr<alisp::Object>& a, std::string b)
{
    ASSERT_EQ(a->toString(), b);
}

void ASSERT_OUTPUT_EQ(alisp::Machine& m, const char* expr, const char* res)
{
    const std::string out = m.evaluate(expr)->toString();
    if (out != res) {
        std::cerr << "Expected '" << expr << "' to output '" << res << "' but ";
        std::cerr << " got '" << out << "' instead.\n";
        exit(1);
    }
}

void ASSERT_OUTPUT_CONTAINS(alisp::Machine& m, const char* expr, const char* res)
{
    const std::string out = m.evaluate(expr)->toString();
    if (out.find(res) == std::string::npos) {
        std::cerr << "Expected the output of '" << expr << "' to contain '" << res << "' but ";
        std::cerr << " it didn't. The output was '" << out << "'.\n";
        exit(1);
    }
}

#define ASSERT_EXCEPTION(m, expr, exception)                            \
{                                                                       \
    bool ok = false;                                                    \
    try {                                                               \
        (m).evaluate((expr));                                           \
    }                                                                   \
    catch (exception & ex) {                                            \
        ok = true;                                                      \
    }                                                                   \
    if (!ok) {                                                           \
        std::cerr << "Expected '" << expr << "' to throw ";             \
        std::cerr << #exception << "\n";                                \
        exit(1);                                                        \
    }                                                                   \
}

void testListBasics()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "()", "nil");
    ASSERT_OUTPUT_EQ(m, "'(1)", "(1)"); 
    ASSERT_OUTPUT_EQ(m, "'(1 2 3)", "(1 2 3)"); 
}

template <typename E>
bool expect(std::function<void(void)> code)
{
    try {
        code();
    }
    catch (E&) {
        return true;
    }
    return false;
}

void testSimpleEvaluations()
{
    alisp::Machine m;
    const auto intSym = m.evaluate("1");
    assert(intSym->isInt() && *intSym == 1);

    assert(m.evaluate("()")->toString() == "nil");
    assert(expect<alisp::exceptions::VoidFunction>([&]() {
        m.evaluate("(nil)");
    }));
}

void testNullFunction()
{
    alisp::Machine m;
    assert(m.evaluate("(null nil)")->toString() == "t");
    assert(m.evaluate("(null ())")->toString() == "t");
    assert(expect<alisp::exceptions::VoidFunction>([&]() {
        m.evaluate("(null (test))");
    }));
    assert(expect<alisp::exceptions::WrongNumberOfArguments>([&]() { m.evaluate("(null)"); }));
    assert(expect<alisp::exceptions::WrongNumberOfArguments>([&]() { m.evaluate("(null 1 2)"); }));
    assert(m.evaluate("(null '(1))")->toString() == "nil");
    assert(m.evaluate("(null '())")->toString() == "t");

    assert(m.evaluate("(null (null t))")->toString() == "t");
    assert(m.evaluate("(null (null (null nil)))")->toString() == "t");
}

void testQuotedList()
{
    alisp::Machine m;
    assert(m.evaluate("'()")->toString() == "nil");
}

void testCloning()
{
    auto l1 = alisp::makeList();
    alisp::cons(alisp::makeInt(3), l1);
    alisp::cons(alisp::makeInt(2), l1);
    alisp::cons(alisp::makeInt(1), l1);
    auto l1c = l1->clone();
    assert(l1->toString() == "(1 2 3)");
    assert(l1c->toString() == l1->toString());

    alisp::Machine m;
    auto e1 = "(2 (1 (1 2 3) 1) 2)";
    auto l2 = m.parse(e1);
    auto l2c = l2->clone();
    assert(l2c->toString() == e1);
}

void testCarFunction()
{
    alisp::Machine m;
    assert(expect<alisp::exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(car 1)");
    }));
    assert(expect<alisp::exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(car (+ 1 1))");
    }));
    assert(m.evaluate("(car nil)")->toString() == "nil");
    assert(m.evaluate("(car ())")->toString() == "nil");
    assert(m.evaluate("(car '())")->toString() == "nil");
    assert(m.evaluate("(car '(1 2))")->toString() == "1");
    assert(expect<alisp::exceptions::VoidFunction>([&]() {
        m.evaluate("(car (1 2))");
    }));
    assert(m.evaluate("(car '(1 2))")->toString() == "1");
    assert(m.evaluate("(car '((1 2)))")->toString() == "(1 2)");
}

void testSyntaxErrorDetection()
{
    alisp::Machine m;
    assert(expect<alisp::exceptions::SyntaxError>([&]() { m.evaluate("(car"); }));
}

void testStrings()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" 2)", "\"cdefg\"");
    ASSERT_EXCEPTION(m, "(substring \"abcdefg\" 2.0)", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" 0 3)", "\"abc\"");
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\")", "\"abcdefg\"");
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" -3 -1)", "\"ef\"");
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" -3 nil)", "\"efg\"");
    ASSERT_OUTPUT_EQ(m, "(concat \"ab\" \"cd\")", "\"abcd\"");
    assert(m.evaluate("\"abc\"")->toString() == "\"abc\"");
    assert(m.evaluate("(stringp (car '(\"a\")))")->toString() == "t");
    assert(m.evaluate("(stringp \"abc\")")->toString() == "t");
    assert(m.evaluate("(stringp 1)")->toString() == "nil");
    assert(m.evaluate("(stringp ())")->toString() == "nil");
    std::set<std::string> expectedMsgs = {
        "test", "a%b", "num: 50.%", "15"
    };
    m.setMessageHandler([&](std::string msg) {
        expectedMsgs.erase(msg);
    });
    m.evaluate("(message \"test\")");
    m.evaluate("(message \"a%%b\")");
    m.evaluate("(message \"%d\" 15)");
    m.evaluate("(message \"num: %d.%%\" 50)");
    assert(expectedMsgs.empty());
}

void testDivision()
{
    alisp::Machine m;
    assert(m.evaluate("(/ 10 2)")->toString() == "5");
    // Same as emacs: one float arg means that also preceding integer divisions
    // are handled as floating point operations.
    assert(std::abs(m.evaluate("(/ 10 3 3.0)")->value<double>() - 1.11111111) < 0.001);
    assert(expect<alisp::exceptions::ArithError>([&]() {
        m.evaluate("(/ 1 0)");
    }));
    assert(expect<alisp::exceptions::ArithError>([&]() {
        m.evaluate("(/ 1 0.0)");
    }));
}

void testCdrFunction()
{
    alisp::Machine m;
    assert(m.evaluate("(cdr '(a b c))")->toString() == "(b c)");
    assert(m.evaluate("(cdr '())")->toString() == "nil");
    assert(m.evaluate("(cdr '(a))")->toString() == "nil");
    assert(m.evaluate("(cdr (cdr '(a b c)))")->toString() == "(c)");
    assert(expect<alisp::exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(cdr 1)");
    }));
}

void testPrognFunction()
{
    alisp::Machine m;
    std::set<std::string> expectedMsgs = {
        "A", "B"
    };
    m.setMessageHandler([&](std::string msg) {
        expectedMsgs.erase(msg);
    });
    assert(m.evaluate("(progn (message \"A\") (message \"B\") 2)")->toString() == "2");
    assert(expectedMsgs.empty());
    assert(m.evaluate("(progn)")->toString() == "nil");
}

void testVariables()
{
    alisp::Machine m;
    m.evaluate("(setq x 2)");
    assert(m.evaluate("(/ 1.0 x)")->value<double>() == 0.5);
    assert(m.evaluate("(+ 1 x)")->value<std::int64_t>() == 3);
}

void testSymbols()
{
    alisp::Machine m;
    assert(m.evaluate("'('a 'b)")->toString() == "('a 'b)");
    assert(m.evaluate("'('a'b)")->toString() == "('a 'b)");
    assert(m.evaluate("(symbolp 'abc)")->toString() == "t");
    assert(m.evaluate("(symbol-name 'abc)")->toString() == "\"abc\"");
    assert(expect<alisp::exceptions::VoidVariable>([&]() {
        m.evaluate("(symbolp abc)");
    }));
    assert(expect<alisp::exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(symbol-name 2)");
    }));
    assert(m.evaluate("(make-symbol \"test\")")->toString() == "test");
    ASSERT_EQ(m.evaluate("(progn (setq sym (make-symbol \"foo\"))(symbol-name sym))"), "\"foo\"");
    ASSERT_EQ(m.evaluate("(eq sym 'foo)"), "nil");
    ASSERT_EQ(m.evaluate("'t"), "t");
    assert(expect<alisp::exceptions::VoidVariable>([&]() {
        m.evaluate("(eq 'a a)");
    }));
}

void testEqFunction()
{
    alisp::Machine m;
    ASSERT_EQ(m.evaluate("(progn (setq x \"a\")(eq x x))"), "t");
    ASSERT_EQ(m.evaluate("(progn (setq y 1)(eq y y))"), "t");
    ASSERT_EQ(m.evaluate("(eq \"a\" \"a\")"), "nil");
    ASSERT_EQ(m.evaluate("(eq 'a 'a)"), "t");
    ASSERT_EQ(m.evaluate("(eq 1 1)"), "t");
    ASSERT_EQ(m.evaluate("(eq 1 1.0)"), "nil");
    ASSERT_EQ(m.evaluate("(eq 1.0 1.0)"), "t");
    ASSERT_EQ(m.evaluate("(eq nil nil)"), "t");
    ASSERT_EQ(m.evaluate("(eq () nil)"), "t");
    ASSERT_EQ(m.evaluate("(eq '() nil)"), "t");
    ASSERT_EQ(m.evaluate("(progn (setq l '(a b))(eq l l))"), "t");
}

void testInternFunction()
{
    alisp::Machine m;
    m.setMessageHandler([](std::string msg){});
    ASSERT_EQ(m.evaluate("(setq sym (intern \"foo\"))"), "foo");
    ASSERT_EQ(m.evaluate("(eq sym 'foo)"), "t");
    ASSERT_EQ(m.evaluate("(intern-soft \"frazzle\")"), "nil");
    ASSERT_EQ(m.evaluate("(setq sym (intern \"frazzle\"))"), "frazzle");
    ASSERT_EQ(m.evaluate("(intern-soft \"frazzle\")"), "frazzle");
    ASSERT_EQ(m.evaluate("(eq sym 'frazzle)"), "t");
    ASSERT_EQ(m.evaluate("(intern-soft \"abc\")"), "nil");
    ASSERT_EQ(m.evaluate("(setq sym (intern \"abc\"))"), "abc");
    ASSERT_EQ(m.evaluate("(intern-soft \"abc\")"), "abc");
    ASSERT_EQ(m.evaluate("(unintern sym)"), "t");
    ASSERT_EQ(m.evaluate("(intern-soft \"abc\")"), "nil");

    // Ensure same functionality as emacs: if sym refers to abra and then abra is uninterned,
    // after that describe-variable still can show what sym pointed to.
    ASSERT_OUTPUT_EQ(m, "(setq sym (intern \"abra\"))", "abra");
    ASSERT_OUTPUT_EQ(m, "(setq abra 500)", "500");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'abra)", "abra's value is 500");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable sym)", "abra's value is 500");
    ASSERT_OUTPUT_EQ(m, "(message \"%d\" abra)", "\"500\"");
    ASSERT_EXCEPTION(m, "(message \"%d\" sym)", alisp::exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(unintern sym)", "t"); // this removes abra from objarray
    ASSERT_EXCEPTION(m, "(message \"%d\" abra)", alisp::exceptions::VoidVariable);
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable sym)", "abra's value is 500");
}

void testDescribeVariableFunction()
{
    alisp::Machine m;
    ASSERT_EXCEPTION(m, "(describe-variable a)", alisp::exceptions::VoidVariable);
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'a)", "a is void as a variable");
    m.evaluate("(setq a 12345)");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'a)", "12345");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable nil)", "nil's value is nil");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable t)", "t's value is t");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 't)", "t's value is t");
}

void testBasicArithmetic()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(% 5 2)", "1");
    ASSERT_EXCEPTION(m, "(% 5 2.0)", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(+ 1 1)", "2");
    ASSERT_OUTPUT_EQ(m, "(+)", "0");
    ASSERT_OUTPUT_EQ(m, "(* 3 4)", "12");
    ASSERT_OUTPUT_EQ(m, "(*)", "1");
    ASSERT_OUTPUT_EQ(m, "-1", "-1");
    ASSERT_OUTPUT_EQ(m, "(+ 1 -1)", "0");
}

void testPopFunction()
{
    alisp::Machine m;
    /*
(setq x '(1 2 3))
(setq y x)
(eq y x) ; t
(pop x)
(eq y x) ; nil
x
y
     */
    ASSERT_OUTPUT_EQ(m, "(setq x '(1 2 3))", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(setq y x)", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(eq y x)", "t");
    //ASSERT_OUTPUT_EQ(m, "(pop x)", "1");
    //ASSERT_OUTPUT_EQ(m, "y", "(1 2 3)");
    //ASSERT_OUTPUT_EQ(m, "x", "(2 3)");
}

void testNthFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(setq x '(\"a\" \"b\"))", "(\"a\" \"b\")");
    ASSERT_OUTPUT_EQ(m, "(nth 0 x)", "\"a\"");
    ASSERT_OUTPUT_EQ(m, "(nth 1 x)", "\"b\"");
    ASSERT_OUTPUT_EQ(m, "(nth 2 x)", "nil");
    ASSERT_OUTPUT_EQ(m, "(eq (nth 1 x) (nth 1 x))", "t");
    ASSERT_OUTPUT_EQ(m, "(setq y (cons \"c\" x))", "(\"c\" \"a\" \"b\")");
    ASSERT_OUTPUT_EQ(m, "(nth 1 x)", "\"b\"");
    ASSERT_OUTPUT_EQ(m, "(nth 2 y)", "\"b\"");
    ASSERT_OUTPUT_EQ(m, "(eq (nth 1 x) (nth 2 y) )", "t");
}

void testConsFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(cons 1 '(2 3))", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(cons 1 '())", "(1)");
}

void testListFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(list 1 2 3 4 5)", "(1 2 3 4 5)");
    ASSERT_OUTPUT_EQ(m, "(list 1 2 '(3 4 5) 'foo)", "(1 2 (3 4 5) foo)");
    ASSERT_OUTPUT_EQ(m, "(list)", "nil");
}

void test()
{
    testListBasics();
    // testConsFunction();
    /*
    testListFunction();
    testNthFunction();
    testPopFunction();
    testStrings();
    testBasicArithmetic();
    testDescribeVariableFunction();
    testInternFunction();
    testEqFunction();
    testVariables();
    testSymbols();
    testDivision();
    testSyntaxErrorDetection();
    testCloning();
    testQuotedList();
    testSimpleEvaluations();
    testNullFunction();
    testCarFunction();
    testCdrFunction();
    testPrognFunction();
    */
    // std::cout << m.evaluate("(+ +.1 -0.1)") << std::endl;
    /*
(symbol-name 2)
(setq sym (make-symbol "foo"))
(symbol-name sym) => foo

foo                 ; A symbol named ‘foo’.
FOO                 ; A symbol named ‘FOO’, different from ‘foo’.
1+                  ; A symbol named ‘1+’
                    ;   (not ‘+1’, which is an integer).
\+1                 ; A symbol named ‘+1’
                    ;   (not a very readable name).
\(*\ 1\ 2\)         ; A symbol named ‘(* 1 2)’ (a worse name).
+-/_*~!@$%^&=:<>{}  ; A symbol named ‘+-/_*~!@$%^&=:<>{}’.
                    ;   These characters need not be escaped.
     */
}

int main(int, char**)
{
    test();
    return 0;
}
