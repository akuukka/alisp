#include "alisp.hpp"
#include <cassert>

void test()
{
    // Empty list
    {
        alisp::ConsCell c;
        assert(c.toString() == "nil");
        auto res = alisp::eval(c);
        auto ls = dynamic_cast<alisp::ListSymbol*>(res.get());
        assert(ls);
        assert(ls->car);
        const auto& l = *ls->car;
        assert(!l);
    }
    // Addition alone (expect same result as emacs: 0)
    {
        alisp::ConsCell c;
        alisp::cons(alisp::makeFunctionAddition(), c);
        assert(c.toString() == "(+)");
        auto res = alisp::eval(c);
        assert(res);
        assert(dynamic_cast<alisp::IntSymbol*>(res.get()));
        assert(dynamic_cast<alisp::IntSymbol*>(res.get())->value == 0);
    }
    // Simple integer addition
    {
        alisp::ConsCell c;
        alisp::cons(alisp::makeInt(1), c);
        assert(c.toString() == "(1)");
        alisp::cons(alisp::makeInt(1), c);
        alisp::cons(alisp::makeFunctionAddition(), c);
        assert(c.toString() == "(+ 1 1)");
        auto res = alisp::eval(c);
        assert(res);
        assert(dynamic_cast<alisp::IntSymbol*>(res.get()));
        assert(dynamic_cast<alisp::IntSymbol*>(res.get())->value == 2);
    }
    // Nested lists
    {
        alisp::ConsCell c;
        auto inner = alisp::makeList(nullptr);
        alisp::cons(std::move(inner), c);
        assert(c.toString() == "(nil)");
    }
    // Nested integer addition
    {
        auto inner = alisp::makeList(nullptr);
        alisp::cons(alisp::makeInt(3), *inner->car);
        alisp::cons(alisp::makeInt(2), *inner->car);
        alisp::cons(alisp::makeFunctionAddition(), *inner->car);

        alisp::ConsCell c;        
        alisp::cons(std::move(inner), c);
        alisp::cons(alisp::makeInt(1), c);
        alisp::cons(alisp::makeFunctionAddition(), c);
        
        assert(c.toString() == "(+ 1 (+ 2 3))");
        std::cout << c.toString() << " => " << alisp::eval(c)->toString() << std::endl;
    }
}

int main(int, char**)
{
    test();
    // alisp::run(" (+ 1 1)");
    return 0;
}
