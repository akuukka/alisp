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
    // std::cout << expr << std::endl;
    const std::string out = m.evaluate(expr)->toString();
    if (out != res) {
        std::cerr << "Expected '" << expr << "' to output '" << res << "' but ";
        std::cerr << " got '" << out << "' instead.\n";
        exit(1);
    }
    // std::cout << " => " << out << std::endl;
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
    ASSERT_OUTPUT_EQ(m, "'(1 2 . 3)", "(1 2 . 3)");
    ASSERT_OUTPUT_EQ(m, "()", "nil");
    ASSERT_OUTPUT_EQ(m, "'(1)", "(1)"); 
    ASSERT_OUTPUT_EQ(m, "'(1 2 3)", "(1 2 3)"); 
    ASSERT_OUTPUT_EQ(m, "(consp '(1 2))", "t"); 
    ASSERT_OUTPUT_EQ(m, "(consp 1)", "nil"); 
    ASSERT_OUTPUT_EQ(m, "(consp nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(atom '(1 2))", "nil"); 
    ASSERT_OUTPUT_EQ(m, "(atom 1)", "t"); 
    ASSERT_OUTPUT_EQ(m, "(atom nil)", "t");
    ASSERT_OUTPUT_EQ(m, "(listp '(1 2))", "t"); 
    ASSERT_OUTPUT_EQ(m, "(listp 1)", "nil"); 
    ASSERT_OUTPUT_EQ(m, "(listp nil)", "t");
    ASSERT_OUTPUT_EQ(m, "(listp '())", "t");
    ASSERT_OUTPUT_EQ(m, "(listp ())", "t");
    ASSERT_OUTPUT_EQ(m, "(nlistp 1)", "t"); 
    ASSERT_OUTPUT_EQ(m, "(nlistp nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(nlistp '())", "nil");
    ASSERT_OUTPUT_EQ(m, "(nlistp ())", "nil");
    ASSERT_OUTPUT_EQ(m, "'(a . b)", "(a . b)");
    ASSERT_OUTPUT_EQ(m, "(car '(a . b))", "a");
    ASSERT_OUTPUT_EQ(m, "(cdr '(a . b))", "b");
    ASSERT_OUTPUT_EQ(m, "(consp '(a . b))", "t");
    ASSERT_OUTPUT_EQ(m, "(car '(a b . c))", "a");
    ASSERT_OUTPUT_EQ(m, "(cdr '(a b . c))", "(b . c)");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p nil)", "0");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p '(1 2 3 4))", "4");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p '(a b . c))", "nil");
    ASSERT_OUTPUT_EQ(m, "(progn (setq x '(\"a\" \"b\")) (setq y (cons x x))"
                     "(eq (car (car y)) (car (cdr y))))", "t");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'y)", "((\"a\" \"b\") \"a\" \"b\")");
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

void testQuote()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "'()", "nil");
    ASSERT_OUTPUT_EQ(m, "'(1 2 3)", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(quote (+ 1 2))", "(+ 1 2)");
    ASSERT_OUTPUT_EQ(m, "(quote foo)", "foo");
    ASSERT_OUTPUT_EQ(m, "'foo", "foo");
    ASSERT_OUTPUT_EQ(m, "''foo", "'foo");
    ASSERT_OUTPUT_EQ(m, "'(quote foo)", "'foo");
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
    ASSERT_OUTPUT_EQ(m, "(cdr '(a b c))", "(b c)");
    ASSERT_OUTPUT_EQ(m, "(cdr '(a))", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr '())", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr (cdr '(a b c)))", "(c)");
    ASSERT_EXCEPTION(m, "(cdr 1)", alisp::exceptions::WrongTypeArgument);
    // ASSERT_OUTPUT_EQ(m, "(cdr-safe '(a b c))", "(b c)");
}

void testPrognFunction()
{
    alisp::Machine m;
    std::set<std::string> expectedMsgs = {
        "A", "B", "C", "D"
    };
    m.setMessageHandler([&](std::string msg) {
        expectedMsgs.erase(msg);
    });
    ASSERT_OUTPUT_EQ(m, "(progn (message \"A\") (message \"B\") 2)", "2");
    ASSERT_OUTPUT_EQ(m, "(progn)", "nil");
    ASSERT_OUTPUT_EQ(m, "(prog1 5 (message \"C\") (message \"D\") 2)", "5");
    assert(expectedMsgs.empty());
}

void testVariables()
{
    alisp::Machine m;
    m.evaluate("(setq x 2)");
    assert(m.evaluate("(/ 1.0 x)")->value<double>() == 0.5);
    assert(m.evaluate("(+ 1 x)")->value<std::int64_t>() == 3);
    ASSERT_OUTPUT_EQ(m, "(set 'y 15)", "15");
}

void testSymbols()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "'('a 'b)", "('a 'b)");
    ASSERT_OUTPUT_EQ(m, "'('a'b)", "('a 'b)");
    assert(m.evaluate("(symbolp 'abc)")->toString() == "t");
    assert(m.evaluate("(symbol-name 'abc)")->toString() == "\"abc\"");
    assert(expect<alisp::exceptions::VoidVariable>([&]() {
        m.evaluate("(symbolp abc)");
    }));
    assert(expect<alisp::exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(symbol-name 2)");
    }));
    ASSERT_OUTPUT_EQ(m, "(make-symbol \"test\")", "test");
    ASSERT_OUTPUT_EQ(m, "(symbolp (make-symbol \"test\"))", "t");
    ASSERT_EXCEPTION(m, "(+ 1 (make-symbol \"newint\"))", alisp::exceptions::WrongTypeArgument);
    ASSERT_EQ(m.evaluate("(progn (setq sym (make-symbol \"foo\"))(symbol-name sym))"), "\"foo\"");
    ASSERT_EQ(m.evaluate("(eq sym 'foo)"), "nil");
    ASSERT_EQ(m.evaluate("'t"), "t");
    assert(expect<alisp::exceptions::VoidVariable>([&]() {
        m.evaluate("(eq 'a a)");
    }));
    ASSERT_EQ(m.evaluate("(symbolp (car (list 'a)))"), "t");
    ASSERT_EXCEPTION(m, "(progn (setq testint (make-symbol \"abracadabra\"))"
                     "(+ 1 (eval testint)))", alisp::exceptions::VoidVariable);
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
    ASSERT_OUTPUT_EQ(m, "(setq sym (intern \"foo\"))", "foo");
    ASSERT_OUTPUT_EQ(m, "(eq sym 'foo)", "t");
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
    ASSERT_OUTPUT_EQ(m, "-1", "-1");
    ASSERT_OUTPUT_EQ(m, "(% 5 2)", "1");
    ASSERT_EXCEPTION(m, "(% 5 2.0)", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(+ 1 1)", "2");
    ASSERT_OUTPUT_EQ(m, "(+)", "0");
    ASSERT_OUTPUT_EQ(m, "(* 3 4)", "12");
    ASSERT_OUTPUT_EQ(m, "(*)", "1");
    ASSERT_OUTPUT_EQ(m, "(+ 1 -1)", "0");
    ASSERT_OUTPUT_EQ(m, "(1+ 0)", "1");
    ASSERT_OUTPUT_CONTAINS(m, "(1+ 0.0)", "1.0");
    ASSERT_EXCEPTION(m, "(1+ \"a\")", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_CONTAINS(m, "(+ +.1 -0.1)", "0.0");
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
    ASSERT_OUTPUT_EQ(m, "(cons 1 2)", "(1 . 2)");
}

void testListFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(list 'a 'b)", "(a b)");
    ASSERT_OUTPUT_EQ(m, "(list 1 2 3 4 5)", "(1 2 3 4 5)");
    ASSERT_OUTPUT_EQ(m, "(list 1 2 '(3 4 5) 'foo)", "(1 2 (3 4 5) foo)");
    ASSERT_OUTPUT_EQ(m, "(list)", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr (list 'a 'b 'c))", "(b c)");
}

void testEvalFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(eval 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(setq foo 'bar)", "bar");
    ASSERT_OUTPUT_EQ(m, "(setq bar 'baz)", "baz");
    ASSERT_OUTPUT_EQ(m, "(eval 'foo)", "bar");
    ASSERT_OUTPUT_EQ(m, "(eval foo)", "baz");
}

void testMacros()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(setq l '(a b))", "(a b)");
    ASSERT_OUTPUT_EQ(m, "(push 'c l)", "(c a b)");
    ASSERT_OUTPUT_EQ(m, "(push 'd l)", "(d c a b)");
    ASSERT_OUTPUT_EQ(m, "(defmacro inc (var) (list 'setq var (list '1+ var)))", "inc");
    ASSERT_OUTPUT_EQ(m, "(setq x 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(inc x)", "2");
    ASSERT_EXCEPTION(m, "(inc 1)", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(setq li '(1 2 3))", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(pop li)", "1");
    ASSERT_OUTPUT_EQ(m, "li", "(2 3)");
    ASSERT_OUTPUT_EQ(m, "(pop nil)", "nil");
    ASSERT_EXCEPTION(m, "(pop 1)", alisp::exceptions::WrongTypeArgument);
}

void testDeepCopy()
{
    alisp::Machine m;
    // Cloning
    auto storage = m.evaluate("'(1 2 3 4)");
    auto list = dynamic_cast<alisp::ConsCellObject*>(storage.get());
    assert(list);
    auto cloned = list->clone();
    list->cc->car = alisp::makeInt(5);
    ASSERT_EQ(list->toString(), "(5 2 3 4)");
    ASSERT_EQ(cloned->toString(), "(5 2 3 4)");
    // Deepcopying
    auto copied = list->deepCopy();
    list->cc->car = alisp::makeInt(1);
    ASSERT_EQ(list->toString(), "(1 2 3 4)");
    ASSERT_EQ(copied->toString(), "(5 2 3 4)");
    // Should work dotted pairs as well
    storage = m.evaluate("'(1 2 . 3)");
    list = dynamic_cast<alisp::ConsCellObject*>(storage.get());
    assert(list);
    ASSERT_EQ(list->toString(), "(1 2 . 3)");
    copied = list->deepCopy();
    ASSERT_EQ(copied->toString(), "(1 2 . 3)");
}

void testFunctions()
{
    alisp::Machine m;
    std::set<std::string> expectedMsgs = {
        "foo", "abc"
    };
    m.setMessageHandler([&](std::string msg) { expectedMsgs.erase(msg); });
    ASSERT_OUTPUT_EQ(m, "(defun foo () (message \"foo\") 5)", "foo");
    ASSERT_OUTPUT_EQ(m, "(defun foo2 (msg) (message msg) msg)", "foo2");
    ASSERT_OUTPUT_EQ(m, "(foo)", "5");
    ASSERT_EXCEPTION(m, "(foo2)", alisp::exceptions::WrongNumberOfArguments);
    ASSERT_OUTPUT_EQ(m, "(foo2 \"abc\")", "\"abc\"");
    ASSERT_OUTPUT_EQ(m, "(cadr '(1 2 3))", "2");
    ASSERT_OUTPUT_EQ(m, "(cadr nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr-safe '(1 2 3))", "(2 3)");
    ASSERT_OUTPUT_EQ(m, "(cdr-safe 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(car-safe '(1 2 3))", "1");
    ASSERT_OUTPUT_EQ(m, "(car-safe 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdar '((1 4) 2 3))", "(4)");    
    ASSERT_OUTPUT_EQ(m, "(cdar nil)", "nil");
    ASSERT_EXCEPTION(m, "(cdar '(1 2 3))", alisp::exceptions::WrongTypeArgument);    
    ASSERT_OUTPUT_EQ(m, "(caar '((8) 2 3))", "8");
    ASSERT_OUTPUT_EQ(m, "(caar 'nil)", "nil");
    assert(expectedMsgs.empty());
    /*
(defun test (z)
  (set 'z (+ z 1))
  (message "%d" z)
  z)
(test 1) ; 1
(message "%d" z) ; 2
     */
}

void testLet()
{
    alisp::Machine m;
    m.setMessageHandler([&](std::string msg) {});
    ASSERT_OUTPUT_EQ(m, "(let ((x 1) (y (+ 1 2))) (message \"%d\" x) (+ x y))", "4");
}

void testIf()
{
    alisp::Machine m;
    m.setMessageHandler([&](std::string msg) { assert(msg != "problem"); });
    ASSERT_OUTPUT_EQ(m, "(if t 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(if (eq 1 1) 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(if nil 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(if nil (message \"problem\") 2 3 4)", "4");
    ASSERT_OUTPUT_EQ(m, "(if nil 1 2 3)", "3");
}

void test()
{
    testFunctions();
    testIf();
    testLet();
    testDeepCopy();
    testListBasics();
    testQuote();
    testMacros();
    testBasicArithmetic();
    testSymbols();
    testEvalFunction();
    testCarFunction();
    testCdrFunction();
    testConsFunction();
    testListFunction();
    testNthFunction();
    testStrings();
    testDescribeVariableFunction();
    testInternFunction();
    testEqFunction();
    testVariables();
    testDivision();
    testSyntaxErrorDetection();
    testSimpleEvaluations();
    testNullFunction();
    testPrognFunction();

    /*
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
