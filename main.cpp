#include "alisp.hpp"
#include <cassert>
#include <set>

void testEmptyList()
{
    alisp::Machine m;
    
    alisp::ConsCell c;
    assert(c.toString() == "nil");
    auto res = alisp::eval(c);
    auto ls = dynamic_cast<alisp::ListSymbol*>(res.get());
    assert(ls);
    const auto& l = ls->car;
    assert(!l);

    auto el = alisp::makeList();
    assert(el->toString() == "nil");

    auto t3 = m.parse("()");
    assert(t3 && t3->isList());
    assert(t3->toString() == "nil");

    assert(m.evaluate("()")->toString() == "nil");
}

void testAddition()
{
    // Addition alone (expect same result as emacs: 0)
    {
        auto l = alisp::makeList();
        alisp::cons(alisp::makeFunctionAddition(), l);
        assert(l->toString() == "(+)");
        auto res = alisp::eval(l);
        assert(res && res->isInt());
        assert(res->toString() == "0");
    }
    // Simple integer addition
    {
        auto l = alisp::makeList();
        alisp::cons(alisp::makeInt(2), l);
        alisp::cons(alisp::makeInt(1), l);
        alisp::cons(alisp::makeFunctionAddition(), l);
        assert(l->toString() == "(+ 1 2)");
        auto res = alisp::eval(l);
        assert(res && res->isInt() && res->toString() == "3");
    }
    // Test case 1
    {
      alisp::Machine m;
      auto syms = m.parse("(+ 3 90)");
      assert(syms->toString() == "(+ 3 90)");
      assert(syms->isList());
      auto res = alisp::eval(syms);
      assert(res->toString() == "93");
    }
    // Test case 2
    {
      alisp::Machine m;
      const auto expr = "(* 3 (+ (+ 1 1) (+ 0 3)))";
      auto syms = m.parse(expr);
      assert(syms->toString() == expr);
      assert(syms->isList());
      auto res = alisp::eval(syms);
      assert(res->toString() == "15");
    }
}

void testMultiplication()
{
    alisp::Machine m;
    assert(alisp::eval(m.parse("(*)"))->toString() == "1");
}

void testNestedLists()
{
    alisp::ConsCell c;
    auto inner = alisp::makeList();
    alisp::cons(std::move(inner), c);
    assert(c.toString() == "(nil)");
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
    auto syms = m.parse("'(1)");
    assert(syms->toString() == "'(1)");
    auto res = alisp::eval(syms);
    assert(res->toString() == "(1)");

    syms = m.parse("'(test)");
    res = alisp::eval(syms);
    assert(res->toString() == "(test)");

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
    assert(m.evaluate("\"abc\"")->toString() == "\"abc\"");
    assert(m.evaluate("(stringp (car '(\"a\")))")->toString() == "t");
    assert(m.evaluate("(stringp \"abc\")")->toString() == "t");
    assert(m.evaluate("(stringp 1)")->toString() == "nil");
    assert(m.evaluate("(stringp ())")->toString() == "nil");
    std::set<std::string> expectedMsgs = {
        "test"
    };
    m.setMessageHandler([&](std::string msg) {
        expectedMsgs.erase(msg);
    });
    m.evaluate("(message \"test\")");
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

void test()
{
    testDivision();
    testSyntaxErrorDetection();
    testStrings();
    testCloning();
    testEmptyList();
    testQuotedList();
    testSimpleEvaluations();
    testAddition();
    testMultiplication();
    testNestedLists();
    testNullFunction();
    testCarFunction();
}

int main(int, char**)
{
    test();
    return 0;
}
