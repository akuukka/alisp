#include "alisp.hpp"
#include <cassert>

void testEmptyList()
{
    alisp::Machine m;
    
    alisp::ConsCell c;
    assert(c.toString() == "nil");
    auto res = alisp::eval(c);
    auto ls = dynamic_cast<alisp::ListSymbol*>(res.get());
    assert(ls);
    assert(ls->car);
    const auto& l = *ls->car;
    assert(!l);

    auto el = alisp::makeList();
    assert(el->toString() == "nil");

    auto t3 = m.parse("()");
    assert(t3 && t3->isList());
    assert(t3->toString() == "nil");
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
    auto inner = alisp::makeList(nullptr);
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
    expect<alisp::exceptions::VoidFunction>([&]() {
        m.evaluate("(nil)");
    });
}

void test()
{
    testEmptyList();
    testSimpleEvaluations();
    testAddition();
    testMultiplication();
    testNestedLists();
}

int main(int, char**)
{
    test();
    return 0;
}
