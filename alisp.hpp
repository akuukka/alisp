#include <cstdint>
#include <map>
#include <cstdio>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <type_traits>
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

struct VoidFunction : std::runtime_error
{
    VoidFunction(std::string fname) : std::runtime_error("void-function " + fname) {}
};

struct WrongNumberOfArguments : std::runtime_error
{
    WrongNumberOfArguments(int num) :
        std::runtime_error("wrong-number-of-arguments: " + std::to_string(num))
    {

    }
};

} // namespace exceptions

class Machine;

struct Symbol
{
    virtual std::string toString() const = 0;
    virtual bool isList() const { return false; }
    virtual bool isInt() const { return false; }
    virtual bool isFloat() const { return false; }
    virtual bool operator!() const { return false; }
    virtual ~Symbol() {}

    virtual bool operator==(std::int64_t value) const { return false; };
    virtual Symbol* resolve() {
        return this;
    }
    virtual std::unique_ptr<Symbol> clone() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Symbol& sym);
};

inline std::ostream& operator<<(std::ostream& os, const Symbol& sym)
{
    os << sym.toString();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::unique_ptr<Symbol>& sym)
{
    if (sym) {
        os << sym->toString();
    }
    else {
        os << "nullptr";
    }
    return os;
}

using NilSymbol = Symbol;

struct ConsCell
{
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

struct TrueSymbol : Symbol {
    std::string toString() const override {
        return "t";
    }

    std::unique_ptr<Symbol> clone() const override
    {
        return std::make_unique<TrueSymbol>();
    }
};

struct FunctionSymbol : Symbol
{
    std::string name;
    std::unique_ptr<Symbol>(*func)(ConsCell*);
    
    std::string toString() const override {
        return name;
    }

    std::unique_ptr<Symbol> clone() const override
    {
        auto sym = std::make_unique<FunctionSymbol>();
        *sym = *this;
        return sym;
    }
};

struct IntSymbol : Symbol {
    std::int64_t value;

    IntSymbol(std::int64_t value) : value(value) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
    bool isInt() const override { return true; }
    bool operator==(std::int64_t value) const override {
        return this->value == value;
    };

    std::unique_ptr<Symbol> clone() const override
    {
        return std::make_unique<IntSymbol>(value);
    }
};

struct FloatSymbol : Symbol {
    double value;

    FloatSymbol(double value) : value(value) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
    
    bool isFloat() const override { return true; }
    
    bool operator==(std::int64_t value) const override {
        return this->value == value;
    };

    std::unique_ptr<Symbol> clone() const override
    {
        return std::make_unique<FloatSymbol>(value);
    }
};

struct ListSymbol : Symbol {
    std::unique_ptr<ConsCell> car; // todo: remove unique_ptr
    bool quoted = false;

    std::string toString() const override {
        return car ? (quoted ? "'" : "") + car->toString() : "nil";
    }

    bool isList() const override { return true; }

    bool operator!() const override
    {
        return !(*car);
    }
    
    std::unique_ptr<Symbol> clone() const override
    {
        auto copy = std::make_unique<ListSymbol>();
        if (this->car) {
            copy->car = std::make_unique<ConsCell>();
            auto last = copy->car.get();
            auto p = this->car.get();
            bool first = true;
            while (p) {
                if (!first) {
                    last->cdr = std::make_unique<ConsCell>();
                    last = last->cdr.get();
                }
                first = false;
                if (p->sym) {
                    last->sym = p->sym->clone();
                }
                p = p->cdr.get();
            }
        }
        return copy;
    }
};

struct NamedSymbol : Symbol
{
    Machine* parent;
    std::string name;

    std::string toString() const override
    {
        return name;
    }

    Symbol* resolve() override;

    NamedSymbol(Machine* parent) : parent(parent) {}

    std::unique_ptr<Symbol> clone() const override
    {
        auto sym = std::make_unique<NamedSymbol>(parent);
        sym->name = name;
        return sym;
    }
};

// Remember nil = ()
std::unique_ptr<ListSymbol> makeNil()
{
    auto r = std::make_unique<ListSymbol>();
    r->car = std::make_unique<ConsCell>();
    return r;
}

std::unique_ptr<TrueSymbol> makeTrue()
{
    return std::make_unique<TrueSymbol>();
}

std::unique_ptr<ListSymbol> makeList(std::unique_ptr<ConsCell> car =  nullptr)
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
    auto sym = c.sym->resolve();
    const FunctionSymbol* f = dynamic_cast<const FunctionSymbol*>(sym);
    if (f) {
        return f->func(c.cdr.get());
    }
    throw exceptions::VoidFunction(sym->toString());
}

std::unique_ptr<Symbol> eval(const std::unique_ptr<ListSymbol>& list)
{
    assert(list->car);
    return eval(*list->car);
}

std::unique_ptr<Symbol> eval(const std::unique_ptr<Symbol>& list)
{
    // If it's a list, evaluation means function call. Otherwise, return a copy of
    // the symbol itself.
    if (list->isList()) {
        auto plist = dynamic_cast<ListSymbol*>(list.get());
        if (plist->quoted) {
            return list->clone();
        }
        return eval(*plist->car);
    }
    return list->clone();
}

int countArgs(const ConsCell* cc)
{
    int i = 0;
    while (cc) {
        i++;
        cc = cc->cdr.get();
    }
    return i;
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

std::unique_ptr<FunctionSymbol> makeFunctionNull()
{
    std::unique_ptr<FunctionSymbol> f = std::make_unique<FunctionSymbol>();
    f->name = "null";
    f->func = [](ConsCell* cc) {
        std::unique_ptr<Symbol> r;
        const int argc = countArgs(cc);
        if (argc != 1) {
            throw exceptions::WrongNumberOfArguments(argc);
        }
        if (!(*cc->sym)) {
            r = makeTrue();
        }
        else {
            r = makeNil();
        }
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

void cons(std::unique_ptr<Symbol> sym, const std::unique_ptr<ListSymbol>& list)
{
    cons(std::move(sym), *list->car);
}

bool isPartOfSymName(const char c) {
    if (c=='+') return true;
    if (c=='*') return true;
    if (c=='-') return true;
    if (c>='a' && c<='z') return true;
    if (c>='A' && c<='Z') return true;
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

std::unique_ptr<Symbol> parseNumericalSymbol(const char*& str)
{
    bool fp = false;
    int dots = 0;
    std::string r;
    while (((*str) >= '0' && (*str) <= '9') || (*str) == '.') {
        if ((*str) == '.') {
            dots++;
            if (dots >= 2) {
                throw std::runtime_error("Two dots!");
            }
        }
        r += *str;
        str++;
    }
    std::stringstream ss(r);
    if (dots) {
        double value;
        ss >> value;
        return makeFloat(value);
    }
    else {
        std::int64_t value;
        ss >> value;
        return makeInt(value);
    }
}

bool onlyWhitespace(const char* expr)
{
    while (*expr) {
        if (!isWhiteSpace(*expr)) {
            return false;
        }
        expr++;
    }
    return true;
}

void skipWhitespace(const char*& expr)
{
    while (*expr && isWhiteSpace(*expr)) {
        expr++;
    }
}

class Machine
{
    std::map<std::string, std::unique_ptr<Symbol>> m_syms;

    std::unique_ptr<Symbol> parseNamedSymbol(const char *&str) {
        auto sym = std::make_unique<NamedSymbol>(this);
        while (*str && isPartOfSymName(*str)) {
            sym->name += *str;
            str++;
        }
        return sym;
    }

    std::unique_ptr<Symbol> parseNext(const char*& expr)
    {
        bool quoted = false;
        // Skip whitespace until next symbol
        while (*expr) {
            const char c = *expr;
            if (isWhiteSpace(c)) {
                expr++;
                continue;
            }
            if (c >= '0' && c <= '9') {
                return parseNumericalSymbol(expr);
            }
            else if (isPartOfSymName(c)) {
                return parseNamedSymbol(expr);
            }
            else if (c == '\'') {
                quoted = true;
                expr++;
            }
            else if (c == '(') {
                auto l = makeList(nullptr);
                l->quoted = quoted;
                quoted = false;
                auto lastConsCell = l->car.get();
                assert(lastConsCell);
                expr++;
                while (*expr != ')') {
                    auto sym = parseNext(expr);
                    skipWhitespace(expr);
                    if (lastConsCell->sym) {
                        assert(!lastConsCell->cdr);
                        lastConsCell->cdr = std::make_unique<ConsCell>();
                        lastConsCell = lastConsCell->cdr.get();
                    }
                    lastConsCell->sym = std::move(sym);
                }
                expr++;
                return l;
            }
            else {
                std::stringstream os;
                os << "Unexpected character: " << c;
                throw std::runtime_error(os.str());
            }
        }    
        return nullptr;
    }
public:
    std::unique_ptr<Symbol> parse(const char* expr)
    {
        auto r = parseNext(expr);
        if (!onlyWhitespace(expr)) {
            throw std::runtime_error("Unexpected: " + std::string(expr));
        }
        return r;
    }

    std::unique_ptr<Symbol> evaluate(const char* expr)
    {
        return eval(parse(expr));
    }

    Symbol* resolve(NamedSymbol* sym)
    {
        if (m_syms.count(sym->name)) {
            return m_syms[sym->name].get();
        }
        throw std::runtime_error("Unable to resolve name: " + sym->name);
    }

    Machine()
    {
        m_syms["+"] = makeFunctionAddition();
        m_syms["*"] = makeFunctionMultiplication();
        m_syms["nil"] = makeNil();
        m_syms["t"] = makeTrue();
        m_syms["null"] = makeFunctionNull();
    }
};

Symbol* NamedSymbol::resolve()
{
    return parent->resolve(this);
}

}
