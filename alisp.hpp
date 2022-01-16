#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <cassert>
#include <functional>

namespace alisp {
namespace exceptions {

struct UnableToEvaluate : std::runtime_error
{
    UnableToEvaluate(std::string msg) : std::runtime_error(msg) {}
};

}

struct Symbol {
    virtual std::string toString() const = 0;
    virtual ~Symbol() {}
};

using NilSymbol = Symbol;

struct ConsCell {
    std::unique_ptr<Symbol> sym;
    std::unique_ptr<ConsCell> cdr;

    std::string toString() const {
        if (!sym && !cdr) {
            return "nil";
        }
        std::string s = "(";
        const ConsCell* t = this;
        while (t) {
            s += t->sym ? t->sym->toString() : "";
            t = t->cdr.get();
            if (t) {
                s += " ";
            }
        }
        s += ")";
        return s;
    }

    bool operator!() const {
        return !sym;
    }
};

using List = ConsCell;

struct FunctionSymbol : Symbol {
    std::string name;
    std::function<std::unique_ptr<Symbol>(ConsCell*)> func;
    std::string toString() const override {
        return name;
    }
};

struct IntSymbol : Symbol {
    std::int64_t value;

    IntSymbol(std::int64_t value) : value(value) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
};

struct FloatSymbol : Symbol {
    double value;

    FloatSymbol(double value) : value(value) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
};

struct ListSymbol : Symbol {
    std::unique_ptr<ConsCell> car;

    std::string toString() const override {
        return car ? car->toString() : "nil";
    }
};

// Remember nil = ()
std::unique_ptr<ListSymbol> makeNil()
{
    auto r = std::make_unique<ListSymbol>();
    r->car = std::make_unique<ConsCell>();
    return r;
}

std::unique_ptr<ListSymbol> makeList(std::unique_ptr<ConsCell> car)
{
    if (!car) {
        return makeNil();
    }
    auto r = std::make_unique<ListSymbol>();
    r->car = std::move(car);
    return r;
}

std::unique_ptr<IntSymbol> makeInt(std::int64_t value)
{
    return std::make_unique<IntSymbol>(value);
}

std::unique_ptr<FloatSymbol> makeFloat(double value)
{
    return std::make_unique<FloatSymbol>(value);
}

std::unique_ptr<Symbol> eval(const ConsCell& c)
{
    if (!c) {
        // Remember: () => nil
        return makeNil();
    }
    const FunctionSymbol* f = dynamic_cast<const FunctionSymbol*>(c.sym.get());
    if (!f) {
        throw exceptions::UnableToEvaluate("Invalid function: " + c.sym->toString());
    }
    return f->func(c.cdr.get());
}

std::unique_ptr<FunctionSymbol> makeFunctionAddition()
{
    std::unique_ptr<FunctionSymbol> f = std::make_unique<FunctionSymbol>();
    f->name = "+";
    f->func = [](ConsCell* p) {
        std::unique_ptr<Symbol> r;
        double floatSum = 0;
        std::int64_t intSum = 0;
        bool fp = false;
        while (p) {
            if (auto i = dynamic_cast<IntSymbol*>(p->sym.get())) {
                intSum += i->value;
                floatSum += i->value;
            }
            else if (auto f = dynamic_cast<FloatSymbol*>(p->sym.get())) {
                floatSum += f->value;
                intSum += f->value;
                fp = true;
            }
            else if (auto l = dynamic_cast<ListSymbol*>(p->sym.get())) {
                auto res = eval(*l->car);
                if (auto i = dynamic_cast<IntSymbol*>(res.get())) {
                    intSum += i->value;
                    floatSum += i->value;
                }
                else if (auto f = dynamic_cast<FloatSymbol*>(res.get())) {
                    floatSum += f->value;
                    intSum += f->value;
                    fp = true;
                }
                else {
                    throw std::runtime_error("Function return type is wrong.");
                }
            }
            else {
                throw std::runtime_error("Wrong type argument: " + p->toString());
            }
            p = p->cdr.get();
        }
        if (fp) {
            r = makeFloat(floatSum);
        }
        r = makeInt(intSum);
        return r;
    };
    return f;
}

std::unique_ptr<FunctionSymbol> makeFunctionMultiplication()
{
    std::unique_ptr<FunctionSymbol> f = std::make_unique<FunctionSymbol>();
    f->name = "*";
    f->func = [](ConsCell* p) {
        std::unique_ptr<Symbol> r;
        double floatMul = 1;
        std::int64_t intMul = 1;
        bool fp = false;
        while (p) {
            if (auto i = dynamic_cast<IntSymbol*>(p->sym.get())) {
              intMul *= i->value;
              floatMul *= i->value;
            }
            else if (auto f = dynamic_cast<FloatSymbol*>(p->sym.get())) {
              floatMul *= f->value;
              intMul *= f->value;
              fp = true;
            }
            else if (auto l = dynamic_cast<ListSymbol*>(p->sym.get())) {
                auto res = eval(*l->car);
                if (auto i = dynamic_cast<IntSymbol*>(res.get())) {
                  intMul *= i->value;
                  floatMul *= i->value;
                }
                else if (auto f = dynamic_cast<FloatSymbol*>(res.get())) {
                  floatMul *= f->value;
                  intMul *= f->value;
                  fp = true;
                }
                else {
                    throw std::runtime_error("Function return type is wrong.");
                }
            }
            else {
                throw std::runtime_error("Wrong type argument: " + p->toString());
            }
            p = p->cdr.get();
        }
        if (fp) {
          r = makeFloat(floatMul);
        }
        r = makeInt(intMul);
        return r;
    };
    return f;
}

void cons(std::unique_ptr<Symbol> sym, List& list)
{
    if (!list) {
        list.sym = std::move(sym);
        return;
    }
    ConsCell cdr = std::move(list);
    list.sym = std::move(sym);
    list.cdr = std::make_unique<ConsCell>();
    list.cdr->sym = std::move(cdr.sym);
    list.cdr->cdr = std::move(cdr.cdr);
}

bool isPartOfSymName(const char c) {
    if (c=='+') return true;
    if (c=='-') return true;
    if (c>='a' && c<='z') return true;
    return false;
}

bool isWhiteSpace(const char c) {
    return c == ' ';
}

std::string parseSymbolName(const char*& str)
{
    std::string r;
    while (isPartOfSymName(*str)) {
        r += *str;
        str++;
    }
    return r;
}

List parseList(const char*& str)
{
    assert(*str == '(');
    str++;
    List l;
    std::cout << "Parse list: " << str << std::endl;
    while (*str != ')') {
        const char c = *str;
        std::cout << c << std::endl;
        if (isWhiteSpace(c)) {
            str++;
        }
        else if (isPartOfSymName(c)) {
            const std::string name = parseSymbolName(str);
            std::cout << name << std::endl;
            if (name == "plus" || name == "+") {
                std::cout << "add oper " << std::endl;
                std::unique_ptr<FunctionSymbol> f = std::make_unique<FunctionSymbol>();
                
            }
        }
        else {
            throw std::runtime_error("Unexpected character.");
        }

    }
    return l;
}

void parseProgram(const char* str)
{
    std::vector<List> prog;
    while (*str) {
        const char c = *str;
        if (c == '(') {
            prog.push_back(parseList(str));
        }
        str++;
    }
}

void run(const char* str)
{
    parseProgram(str);
}

}
