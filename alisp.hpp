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


struct Error : std::runtime_error
{
    Error(std::string msg) : std::runtime_error(msg) {}
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

struct SyntaxError : std::runtime_error
{
    SyntaxError(std::string err) : std::runtime_error(err) { }
};

struct WrongTypeArgument : std::runtime_error
{
    WrongTypeArgument(std::string arg) :
        std::runtime_error("wrong-type-argument: " + arg)
    {

    }
};

struct ArithError : std::runtime_error
{
    ArithError(std::string err) :
        std::runtime_error("arith-error: " + err)
    {

    }
};

} // namespace exceptions

class Machine;
struct Symbol;

template <typename T>
T getValue(const Symbol& sym);

struct Symbol
{
    virtual std::string toString() const = 0;
    virtual bool isList() const { return false; }
    virtual bool isInt() const { return false; }
    virtual bool isFloat() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool operator!() const { return false; }
    virtual ~Symbol() {}

    virtual bool operator==(std::int64_t value) const { return false; };
    virtual Symbol* resolve() {
        return this;
    }
    virtual std::unique_ptr<Symbol> clone() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Symbol& sym);

    template<typename T>
    T value() const
    {
        return getValue<T>(*this);
    }        
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

struct FArgs;

struct FunctionSymbol : Symbol
{
    std::string name;
    int minArgs = 0;
    int maxArgs = 0xffff;
    std::unique_ptr<Symbol>(*func)(FArgs&);
    
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

struct StringSymbol : Symbol {
    std::string value;
    Machine* parent;

    StringSymbol(std::string value, Machine* p) : value(value), parent(p) {}
    
    std::string toString() const override {
        return "\"" + value + "\"";
    }
    
    bool isString() const override { return true; }

    std::unique_ptr<Symbol> clone() const override
    {
        return std::make_unique<StringSymbol>(value, parent);
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
    ConsCell car; // todo: remove unique_ptr
    bool quoted = false;

    std::string toString() const override {
        return (quoted ? "'" : "") + car.toString();
    }

    bool isList() const override { return true; }

    bool operator!() const override
    {
        return !car;
    }
    
    std::unique_ptr<Symbol> clone() const override
    {
        auto copy = std::make_unique<ListSymbol>();
        auto last = &copy->car;
        auto p = &this->car;
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
std::unique_ptr<Symbol> makeNil()
{
    return std::make_unique<ListSymbol>();
}

std::unique_ptr<Symbol> makeTrue()
{
    return std::make_unique<TrueSymbol>();
}

std::unique_ptr<ListSymbol> makeList()
{
    return std::make_unique<ListSymbol>();
}

std::unique_ptr<IntSymbol> makeInt(std::int64_t value)
{
    return std::make_unique<IntSymbol>(value);
}

std::unique_ptr<FloatSymbol> makeFloat(double value)
{
    return std::make_unique<FloatSymbol>(value);
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

std::unique_ptr<Symbol> eval(const std::unique_ptr<Symbol>& list);

struct FArgs
{
    ConsCell* cc;
    
    FArgs(ConsCell& cc) : cc(&cc) {}
    
    std::unique_ptr<Symbol> get()
    {
        if (!cc) {
            return nullptr;
        }
        auto sym = eval(cc->sym);
        cc = cc->cdr.get();
        return sym;
    }

    bool hasNext() const
    {
        return cc;
    }

    struct Iterator {
        ConsCell* cc;

        bool operator!=(const Iterator& o) const
        {
            return cc != o.cc;
        }

        void operator++()
        {
            cc = cc->cdr.get();
        }

        std::unique_ptr<Symbol> operator*()
        {
            return eval(cc->sym);
        }
    };

    Iterator begin() {
        return Iterator{cc};
    }

    Iterator end() {
        return Iterator{nullptr};
    }

};

std::unique_ptr<Symbol> eval(const ConsCell& c)
{
    if (!c) {
        // Remember: () => nil
        return makeNil();
    }
    auto sym = c.sym->resolve();
    if (!sym) {
        throw exceptions::VoidFunction(c.sym->toString());
    }
    const FunctionSymbol* f = dynamic_cast<const FunctionSymbol*>(sym);
    if (f) {
        const int argc = countArgs(c.cdr.get());
        if (argc < f->minArgs || argc > f->maxArgs) {
            throw exceptions::WrongNumberOfArguments(argc);
        }
        FArgs args(*c.cdr);
        return f->func(args);
    }
    throw exceptions::VoidFunction(sym->toString());
}

std::unique_ptr<Symbol> eval(const std::unique_ptr<ListSymbol>& list)
{
    assert(list->car);
    return eval(list->car);
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
        return eval(plist->car);
    }
    return list->resolve()->clone();
}

std::unique_ptr<FunctionSymbol> makeFunctionNull()
{
    std::unique_ptr<FunctionSymbol> f = std::make_unique<FunctionSymbol>();
    f->name = "null";
    f->minArgs = 1;
    f->maxArgs = 1;
    f->func = [](FArgs& args) {
        std::unique_ptr<Symbol> r;
        auto sym = eval(args.cc->sym);
        if (!(*sym)) {
            r = makeTrue();
        }
        else {
            r = makeNil();
        }
        return r;
    };
    return f;
}

std::unique_ptr<FunctionSymbol> makeFunctionCar()
{
    std::unique_ptr<FunctionSymbol> f = std::make_unique<FunctionSymbol>();
    f->name = "car";
    f->minArgs = 1;
    f->maxArgs = 1;
    f->func = [](FArgs& args) {
        auto arg = eval(args.cc->sym);
        if (!arg->isList()) {
            throw exceptions::WrongTypeArgument(args.cc->sym->toString());
        }
        auto list = dynamic_cast<ListSymbol*>(arg.get());
        if (!list->car.sym) {
            return makeNil();
        }
        return list->car.sym->clone();
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
    cons(std::move(sym), list->car);
}

bool isPartOfSymName(const char c) {
    if (c=='+') return true;
    if (c=='*') return true;
    if (c=='/') return true;
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

template<>
inline double getValue(const Symbol& sym)
{
    auto s = dynamic_cast<const FloatSymbol*>(&sym);
    return s ? s->value : 0.0;
}

template<>
inline std::int64_t getValue(const Symbol& sym)
{
    auto s = dynamic_cast<const IntSymbol*>(&sym);
    return s ? s->value : 0.0;
}

class Machine
{
    std::map<std::string, std::unique_ptr<Symbol>> m_syms;
    std::function<void(std::string)> m_msgHandler;

    std::unique_ptr<Symbol> parseNamedSymbol(const char *&str)
    {
        auto sym = std::make_unique<NamedSymbol>(this);
        while (*str && isPartOfSymName(*str)) {
            sym->name += *str;
            str++;
        }
        return sym;
    }

    std::unique_ptr<StringSymbol> parseString(const char *&str)
    {
        auto sym = std::make_unique<StringSymbol>("", this);
        while (*str && *str != '"') {
            sym->value += *str;
            str++;
        }
        if (!*str) {
            throw std::runtime_error("unexpected end of file");
        }
        str++;
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
            else if (c == '\"') {
                return parseString(++expr);
            }
            else if (isPartOfSymName(c)) {
                return parseNamedSymbol(expr);
            }
            else if (c == '\'') {
                quoted = true;
                expr++;
            }
            else if (c == '(') {
                auto l = makeList();
                l->quoted = quoted;
                quoted = false;
                auto lastConsCell = &l->car;
                assert(lastConsCell);
                expr++;
                while (*expr != ')' && *expr) {
                    auto sym = parseNext(expr);
                    skipWhitespace(expr);
                    if (lastConsCell->sym) {
                        assert(!lastConsCell->cdr);
                        lastConsCell->cdr = std::make_unique<ConsCell>();
                        lastConsCell = lastConsCell->cdr.get();
                    }
                    lastConsCell->sym = std::move(sym);
                }
                if (!*expr) {
                    throw exceptions::SyntaxError("End of file during parsing");
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
        return nullptr;
    }

    void makeFunc(const char* name,
                  int minArgs,
                  int maxArgs,
                  std::unique_ptr<Symbol>(*f)(FArgs&))
    {
        auto func = std::make_unique<FunctionSymbol>();
        func->name = name;
        func->minArgs = minArgs;
        func->maxArgs = maxArgs;
        func->func = f;
        m_syms[name] = std::move(func);
    }

    Machine()
    {
        m_syms["nil"] = makeNil();
        m_syms["t"] = makeTrue();
        m_syms["null"] = makeFunctionNull();
        m_syms["car"] = makeFunctionCar();
        makeFunc("stringp", 1, 1, [](FArgs& args) {
            return (args.get()->isString()) ? makeTrue() : makeNil();
        });
        makeFunc("numberp", 1, 1, [](FArgs& args) {
            auto arg = args.get();
            return arg->isInt() || arg->isFloat() ? makeTrue() : makeNil();
        });
        makeFunc("message", 1, 0xffff, [](FArgs& args) {
            auto arg = args.get();
            if (!arg->isString()) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            auto strSym = dynamic_cast<StringSymbol*>(arg.get());
            std::string str = strSym->value;
            for (size_t i = 0; i < str.size(); i++) {
                if (str[i] == '%') {
                    if (str[i+1] == '%') {
                        str.erase(i, 1);
                    }
                    else if (str[i+1] == 'd') {
                        const auto nextSym = args.get();
                        std::int64_t intVal;
                        if (nextSym->isInt()) {
                            intVal = nextSym->value<std::int64_t>();
                        }
                        else if (nextSym->isFloat()) {
                            intVal = static_cast<std::int64_t>(nextSym->value<double>());
                        }
                        else {
                            throw exceptions::Error("Format specifier doesn’t match argument type");
                        }
                        str = str.substr(0, i) + std::to_string(intVal) + str.substr(i+2);
                    }
                    else {
                        throw exceptions::Error("Invalid format string");
                    }
                }
            }
            if (strSym->parent->m_msgHandler) {
                strSym->parent->m_msgHandler(str);
            }
            else {
                std::cout << str << std::endl;
            }
            return arg;
        });
        makeFunc("/", 1, 0xffff, [](FArgs& args) {
            std::int64_t i = 0;
            double f = 0;
            bool first = true;
            bool fp = false;
            for (auto sym : args) {
                if (sym->isFloat()) {
                    const double v = sym->value<double>();
                    if (v == 0) {
                        throw exceptions::ArithError("Division by zero");
                    }
                    fp = true;
                    if (first) {
                        i = v;
                        f = v;
                    }
                    else {
                        i /= v;
                        f /= v;
                    }
                }
                else if (sym->isInt()) {
                    const std::int64_t v = sym->value<std::int64_t>();
                    if (v == 0) {
                        throw exceptions::ArithError("Division by zero");
                    }
                    if (first) {
                        i = v;
                        f = v;
                    }
                    else {
                        i /= v;
                        f /= v;
                    }
                }
                else {
                    throw std::runtime_error("Invalid operand " + sym->toString());
                }
                first = false;
            }
            return fp ? static_cast<std::unique_ptr<Symbol>>(makeFloat(f)) :
                static_cast<std::unique_ptr<Symbol>>(makeInt(i));
        });
        makeFunc("+", 0, 0xffff, [](FArgs& args) {
            std::int64_t i = 0;
            double f = 0;
            bool fp = false;
            for (auto sym : args) {
                if (sym->isFloat()) {
                    const double v = sym->value<double>();
                    fp = true;
                    i += v;
                    f += v;
                }
                else if (sym->isInt()) {
                    const std::int64_t v = sym->value<std::int64_t>();
                    i += v;
                    f += v;
                }
                else {
                    throw std::runtime_error("Invalid operand " + sym->toString());
                }
            }
            return fp ? static_cast<std::unique_ptr<Symbol>>(makeFloat(f)) :
                static_cast<std::unique_ptr<Symbol>>(makeInt(i));
        });
        makeFunc("*", 0, 0xffff, [](FArgs& args) {
            std::int64_t i = 1;
            double f = 1;
            bool fp = false;
            for (auto sym : args) {
                if (sym->isFloat()) {
                    const double v = sym->value<double>();
                    fp = true;
                    i *= v;
                    f *= v;
                }
                else if (sym->isInt()) {
                    const std::int64_t v = sym->value<std::int64_t>();
                    i *= v;
                    f *= v;
                }
                else {
                    throw std::runtime_error("Invalid operand " + sym->toString());
                }
            }
            return fp ? static_cast<std::unique_ptr<Symbol>>(makeFloat(f)) :
                static_cast<std::unique_ptr<Symbol>>(makeInt(i));
        });
        makeFunc("cdr", 1, 1, [](FArgs& args) {
            auto sym = args.get();
            if (!sym->isList()) {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
            auto list = dynamic_cast<ListSymbol*>(sym.get());
            std::unique_ptr<ListSymbol> cdr = makeList();
            if (list->car.cdr) {
                cdr->car.sym = std::move(list->car.cdr->sym);
                cdr->car.cdr = std::move(list->car.cdr->cdr);
            }
            std::unique_ptr<Symbol> ret;
            ret = std::move(cdr);
            return ret;
        });
        makeFunc("progn", 0, 0xfffff, [](FArgs& args) {
            std::unique_ptr<Symbol> ret = makeNil();
            for (auto sym : args) {
                ret = std::move(sym);
            }
            return ret;
        });
        makeFunc("setq", 2, 2, [](FArgs& args) {
            const NamedSymbol* name = dynamic_cast<NamedSymbol*>(args.cc->sym.get());
            if (!name) {
                throw exceptions::WrongTypeArgument(args.cc->sym->toString());
            }
            auto value = eval(args.cc->cdr->sym);
            name->parent->setVariable(name->name, value->clone());
            return value;
        });
    }

    void setVariable(std::string name, std::unique_ptr<Symbol> sym)
    {
        m_syms[name] = std::move(sym);
    }

    void setMessageHandler(std::function<void(std::string)> handler)
    {
        m_msgHandler = handler;
    }
};

Symbol* NamedSymbol::resolve()
{
    return parent->resolve(this);
}

}
