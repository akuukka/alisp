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

void test()
{
    testEmptyList();
    testAddition();
    // Multiplication alone (expect same result as emacs: 0)
    {
        auto l = alisp::makeList();
        alisp::cons(alisp::makeFunctionMultiplication(), l);
        auto res = alisp::eval(l);
        assert(res);
        assert(res->isInt());
        assert(res->toString() == "1");
    }
    // Nested lists
    {
        alisp::ConsCell c;
        auto inner = alisp::makeList(nullptr);
        alisp::cons(std::move(inner), c);
        assert(c.toString() == "(nil)");
    }
    // Nested integer addition and multiplication
    {
        auto inner = alisp::makeList(nullptr);
        alisp::cons(alisp::makeInt(3), *inner->car);
        alisp::cons(alisp::makeInt(2), *inner->car);
        alisp::cons(alisp::makeFunctionMultiplication(), *inner->car);

        alisp::ConsCell c;        
        alisp::cons(std::move(inner), c);
        alisp::cons(alisp::makeInt(1), c);
        alisp::cons(alisp::makeFunctionAddition(), c);
        
        assert(c.toString() == "(+ 1 (* 2 3))");
        const auto res = alisp::eval(c);
        assert(dynamic_cast<alisp::IntSymbol*>(res.get()));
        assert(dynamic_cast<alisp::IntSymbol*>(res.get())->value == 7);
    }
    // Parse simple expressions
    {
        alisp::Machine m;
        auto t1 = m.parse("   123 ");
        assert(t1);
        assert(t1->isInt());
        assert(t1->toString() == "123");
        assert(*t1 == 123);

        auto t2 = m.parse("123.5 ");
        assert(t2);
        assert(t2->isFloat());
        assert(dynamic_cast<alisp::FloatSymbol*>(t2.get())->value == 123.5);
    }
}

int main(int, char**)
{
    test();
    return 0;
}
