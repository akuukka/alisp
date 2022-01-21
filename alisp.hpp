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

struct VoidVariable : std::runtime_error
{
    VoidVariable(std::string vname) : std::runtime_error("void-variable " + vname) {}
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
struct Object;
struct Function;

template <typename T> T getValue(const Object &sym);

struct Object
{
    virtual std::string toString() const = 0;
    virtual bool isList() const { return false; }
    virtual bool isInt() const { return false; }
    virtual bool isFloat() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool operator!() const { return false; }
    virtual ~Object() {}

    virtual bool operator==(std::int64_t value) const { return false; };
    virtual Object* resolveVariable() { return this; }
    virtual Function* resolveFunction() { return nullptr; }
    virtual std::unique_ptr<Object> clone() const = 0;

    friend std::ostream &operator<<(std::ostream &os, const Object &sym);

    template <typename T> T value() const { return getValue<T>(*this); }
};

inline std::ostream &operator<<(std::ostream &os, const Object &sym) {
    os << sym.toString();
    return os;
}

inline std::ostream &operator<<(std::ostream &os,
                                const std::unique_ptr<Object> &sym) {
    if (sym) {
        os << sym->toString();
    }
    else {
        os << "nullptr";
    }
    return os;
}

struct ConsCell
{
    std::unique_ptr<Object> obj;
    std::unique_ptr<ConsCell> cdr;

    std::string toString() const
    {
        if (!obj && !cdr) {
            return "nil";
        }
        std::string s = "(";
        const ConsCell *t = this;
        while (t) {
            s += t->obj ? t->obj->toString() : "";
            t = t->cdr.get();
            if (t) {
                s += " ";
            }
        }
        s += ")";
        return s;
    }

    bool operator!() const { return !obj; }
};

struct TrueObject : Object {
    std::string toString() const override { return "t"; }

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<TrueObject>();
    }
};

struct FArgs;

struct Function
{
    std::string name;
    int minArgs = 0;
    int maxArgs = 0xffff;
    std::unique_ptr<Object> (*func)(FArgs&);
};

struct Symbol
{
    Machine* parent;
    std::string name;
    std::unique_ptr<Object> variable;
    std::unique_ptr<Function> function;
};

struct StringObject : Object {
    std::string value;
    Machine *parent;

    StringObject(std::string value, Machine *p) : value(value), parent(p) {}

    std::string toString() const override { return "\"" + value + "\""; }

    bool isString() const override { return true; }

    std::unique_ptr<Object> clone() const override {
        return std::make_unique<StringObject>(value, parent);
    }
};

struct IntObject : Object {
    std::int64_t value;

    IntObject(std::int64_t value) : value(value) {}

    std::string toString() const override { return std::to_string(value); }
    bool isInt() const override { return true; }
    bool operator==(std::int64_t value) const override {
        return this->value == value;
    };

    std::unique_ptr<Object> clone() const override {
        return std::make_unique<IntObject>(value);
    }
};

struct FloatObject : Object {
    double value;

    FloatObject(double value) : value(value) {}

    std::string toString() const override { return std::to_string(value); }

    bool isFloat() const override { return true; }

    bool operator==(std::int64_t value) const override {
        return this->value == value;
    };

    std::unique_ptr<Object> clone() const override {
        return std::make_unique<FloatObject>(value);
    }
};

struct ListObject : Object {
    ConsCell car; // todo: remove unique_ptr
    bool quoted = false;

    std::string toString() const override {
        return (quoted ? "'" : "") + car.toString();
    }

    bool isList() const override { return true; }

    bool operator!() const override { return !car; }

    std::unique_ptr<Object> clone() const override {
        auto copy = std::make_unique<ListObject>();
        auto last = &copy->car;
        auto p = &this->car;
        bool first = true;
        while (p) {
            if (!first) {
                last->cdr = std::make_unique<ConsCell>();
                last = last->cdr.get();
            }
            first = false;
            if (p->obj) {
                last->obj = p->obj->clone();
            }
            p = p->cdr.get();
        }
        return copy;
    }
};

struct NamedObject : Object
{
    Machine* parent;
    std::string name;

    std::string toString() const override
    {
        return name;
    }

    Object* resolveVariable() override;
    Function* resolveFunction() override;

    NamedObject(Machine *parent) : parent(parent) {}

    std::unique_ptr<Object> clone() const override
    {
        auto sym = std::make_unique<NamedObject>(parent);
        sym->name = name;
        return sym;
    }
};

struct SymbolObject : Object
{
    Symbol* sym = nullptr;

    std::string toString() const override
    {
        return "'" + sym->name;
    }

    std::unique_ptr<Object> clone() const override
    {
        auto sym = std::make_unique<SymbolObject>();
        *sym = *this;
        return sym;
    }
};

// Remember nil = ()
std::unique_ptr<Object> makeNil() { return std::make_unique<ListObject>(); }

std::unique_ptr<Object> makeTrue() { return std::make_unique<TrueObject>(); }

std::unique_ptr<ListObject> makeList()
{
    return std::make_unique<ListObject>();
}

std::unique_ptr<IntObject> makeInt(std::int64_t value) {
    return std::make_unique<IntObject>(value);
}

std::unique_ptr<FloatObject> makeFloat(double value) {
    return std::make_unique<FloatObject>(value);
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

std::unique_ptr<Object> eval(const std::unique_ptr<Object> &list);

struct FArgs
{
    ConsCell* cc;
    
    FArgs(ConsCell& cc) : cc(&cc) {}

    std::unique_ptr<Object> get() {
        if (!cc) {
            return nullptr;
        }
        auto sym = eval(cc->obj);
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

        std::unique_ptr<Object> operator*() { return eval(cc->obj); }
    };

    Iterator begin() {
        return Iterator{cc};
    }

    Iterator end() {
        return Iterator{nullptr};
    }

};

std::unique_ptr<Object> eval(const std::unique_ptr<Object>& obj)
{
    // If it's a list, evaluation means function call. Otherwise, return a copy of
    // the symbol itself.
    if (!obj->isList()) {
        auto var = obj->resolveVariable();
        if (!var) {
            throw exceptions::VoidVariable(obj->toString());
        }
        return var->clone();
    }
    auto plist = dynamic_cast<ListObject *>(obj.get());
    if (plist->quoted) {
        return obj->clone();
    }
    const auto &c = plist->car;
    if (!c) {
        // Remember: () => nil
        return makeNil();
    }
    const Function *f = c.obj->resolveFunction();
    if (f) {
        const int argc = countArgs(c.cdr.get());
        if (argc < f->minArgs || argc > f->maxArgs) {
            throw exceptions::WrongNumberOfArguments(argc);
        }
        FArgs args(*c.cdr);
        return f->func(args);
    }
    throw exceptions::VoidFunction(c.obj->toString());
}

void cons(std::unique_ptr<Object> sym, ConsCell& list)
{
    if (!list) {
        list.obj = std::move(sym);
        return;
    }
    ConsCell cdr = std::move(list);
    list.obj = std::move(sym);
    list.cdr = std::make_unique<ConsCell>();
    list.cdr->obj = std::move(cdr.obj);
    list.cdr->cdr = std::move(cdr.cdr);
}

void cons(std::unique_ptr<Object> sym, const std::unique_ptr<ListObject> &list)
{
    cons(std::move(sym), list->car);
}

bool isPartOfSymName(const char c)
{
    if (c=='+') return true;
    if (c=='*') return true;
    if (c=='/') return true;
    if (c=='-') return true;
    if (c>='a' && c<='z') return true;
    if (c>='A' && c<='Z') return true;
    return false;
}

bool isWhiteSpace(const char c)
{
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

std::unique_ptr<Object> parseNumericalSymbol(const char *&str)
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
inline double getValue(const Object &sym)
{
    auto s = dynamic_cast<const FloatObject *>(&sym);
    return s ? s->value : 0.0;
}

template <>
inline std::int64_t getValue(const Object &sym)
{
    auto s = dynamic_cast<const IntObject *>(&sym);
    return s ? s->value : 0.0;
}

class Machine
{
    std::map<std::string, Symbol> m_syms;
    std::function<void(std::string)> m_msgHandler;

    std::unique_ptr<Object> parseNamedObject(const char*& str, bool quoted)
    {
        std::string name;
        while (*str && isPartOfSymName(*str)) {
            name += *str;
            str++;
        }
        if (quoted) {
            auto obj = std::make_unique<SymbolObject>();
            obj->sym = &m_syms[name];
            obj->sym->name = name;
            return obj;
        }
        else {
            auto obj = std::make_unique<NamedObject>(this);
            obj->name = name;
            return obj;
        }
    }
    
    std::unique_ptr<StringObject> parseString(const char *&str) {
        auto sym = std::make_unique<StringObject>("", this);
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

    std::unique_ptr<Object> parseNext(const char *&expr)
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
            else if (isPartOfSymName(c))
            {
                return parseNamedObject(expr, quoted);
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
                    if (lastConsCell->obj) {
                        assert(!lastConsCell->cdr);
                        lastConsCell->cdr = std::make_unique<ConsCell>();
                        lastConsCell = lastConsCell->cdr.get();
                    }
                    lastConsCell->obj = std::move(sym);
                }
                if (!*expr) {
                    throw exceptions::SyntaxError("End of file during parsing");
                }
                expr++;
                return l;
            } else {
                std::stringstream os;
                os << "Unexpected character: " << c;
                throw std::runtime_error(os.str());
            }
        }
        return nullptr;
    }

public:
    std::unique_ptr<Object> parse(const char *expr) {
        auto r = parseNext(expr);
        if (!onlyWhitespace(expr)) {
            throw std::runtime_error("Unexpected: " + std::string(expr));
        }
        return r;
    }

    std::unique_ptr<Object> evaluate(const char *expr)
    {
        return eval(parse(expr));
    }

    Object* resolveVariable(NamedObject* sym)
    {
        if (m_syms.count(sym->name)) {
            return m_syms[sym->name].variable.get();
        }
        return nullptr;
    }

    Function* resolveFunction(NamedObject* sym)
    {
        if (m_syms.count(sym->name)) {
            return m_syms[sym->name].function.get();
        }
        return nullptr;
    }

    void makeFunc(const char *name, int minArgs, int maxArgs,
                  std::unique_ptr<Object> (*f)(FArgs &))
    {
        auto func = std::make_unique<Function>();
        func->name = name;
        func->minArgs = minArgs;
        func->maxArgs = maxArgs;
        func->func = f;
        m_syms[name].name = name;
        m_syms[name].function = std::move(func);
        m_syms[name].parent = this;
    }

    Machine()
    {
        setVariable("nil", makeNil());
        setVariable("t", makeTrue());
        
        makeFunc("null", 1, 1, [](FArgs &args) {
            std::unique_ptr<Object> r;
            auto sym = eval(args.cc->obj);
            if (!(*sym)) {
                r = makeTrue();
            } else {
                r = makeNil();
            }
            return r;
        });
        makeFunc("car", 1, 1, [](FArgs &args) {
            auto arg = eval(args.cc->obj);
            if (!arg->isList()) {
                throw exceptions::WrongTypeArgument(args.cc->obj->toString());
            }
            auto list = dynamic_cast<ListObject *>(arg.get());
            if (!list->car.obj) {
                return makeNil();
            }
            return list->car.obj->clone();
        });
        makeFunc("stringp", 1, 1, [](FArgs& args) {
            return (args.get()->isString()) ? makeTrue() : makeNil();
        });
        makeFunc("numberp", 1, 1, [](FArgs& args) {
            auto arg = args.get();
            return arg->isInt() || arg->isFloat() ? makeTrue() : makeNil();
        });
        makeFunc("symbolp", 1, 1, [](FArgs& args) {
            return dynamic_cast<SymbolObject*>(args.get().get()) ? makeTrue() : makeNil();
        });
        makeFunc("symbol-name", 1, 1, [](FArgs& args) {
            const auto obj = args.get();
            const auto sym = dynamic_cast<SymbolObject*>(obj.get());
            if (!sym) {
                throw exceptions::WrongTypeArgument(obj->toString());
            }
            std::unique_ptr<Object> r = std::make_unique<StringObject>(sym->sym->name,
                                                                       sym->sym->parent);
            return r;
        });
        makeFunc("message", 1, 0xffff, [](FArgs& args) {
            auto arg = args.get();
            if (!arg->isString()) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            auto strSym = dynamic_cast<StringObject*>(arg.get());
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
            return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
                : static_cast<std::unique_ptr<Object>>(makeInt(i));
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
            return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
                : static_cast<std::unique_ptr<Object>>(makeInt(i));
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
            return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
                : static_cast<std::unique_ptr<Object>>(makeInt(i));
        });
        makeFunc("cdr", 1, 1, [](FArgs& args) {
            auto sym = args.get();
            if (!sym->isList()) {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
            auto list = dynamic_cast<ListObject*>(sym.get());
            std::unique_ptr<ListObject> cdr = makeList();
            if (list->car.cdr) {
                cdr->car.obj = std::move(list->car.cdr->obj);
                cdr->car.cdr = std::move(list->car.cdr->cdr);
            }
            std::unique_ptr<Object> ret;
            ret = std::move(cdr);
            return ret;
        });
        makeFunc("progn", 0, 0xfffff, [](FArgs &args) {
            std::unique_ptr<Object> ret = makeNil();
            for (auto obj : args) {
                ret = std::move(obj);
            }
            return ret;
        });
        makeFunc("setq", 2, 2, [](FArgs &args) {
            const NamedObject* name =
                dynamic_cast<NamedObject*>(args.cc->obj.get());
            if (!name) {
                throw exceptions::WrongTypeArgument(args.cc->obj->toString());
            }
            auto value = eval(args.cc->cdr->obj);
            name->parent->setVariable(name->name, value->clone());
            return value;
        });
    }

    void setVariable(std::string name, std::unique_ptr<Object> obj)
    {
        assert(obj);
        m_syms[name].parent = this;
        m_syms[name].variable = std::move(obj);
        m_syms[name].name = std::move(name);
    }

    void setMessageHandler(std::function<void(std::string)> handler)
    {
        m_msgHandler = handler;
    }
};

Object* NamedObject::resolveVariable()
{
    return parent->resolveVariable(this);
}

Function* NamedObject::resolveFunction()
{
    return parent->resolveFunction(this);
}

}
