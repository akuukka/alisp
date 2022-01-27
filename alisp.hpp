#include <cstdint>
#include <string>
#include <variant>
#include <any>
#include <limits>
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
#include <optional>
#include <set>
#include "AtScopeExit.h"
#include "Init.hpp"
#include "Template.hpp"
#include "Exception.hpp"

namespace alisp {

class Machine;
struct Object;
struct ConsCellObject;
struct Function;

template <typename T>
std::optional<T> getValue(const Object &sym);

template <typename T, typename T2 = void>
struct Converter
{
    std::optional<T> operator()(const Object& obj)
    {
        return getValue<T>(obj);
    }
};

template <typename T>
struct Converter<T, typename std::enable_if<IsInstantiationOf<std::variant, T>::value>::type>
{
    template<size_t I>
    typename std::enable_if<I == std::variant_size_v<T>, void>::type tryGetValue(const Object& o,
                                                                                std::optional<T>& v)
    {
        
    }

    template<size_t I>
    typename std::enable_if<I < std::variant_size_v<T>, void>::type tryGetValue(const Object& o,
                                                                                std::optional<T>& v)
    {
        using TT = std::variant_alternative_t<I, T>;
        std::optional<TT> opt = getValue<TT>(o);
        if (opt) {
            T variant;
            variant = *opt;
            v = variant;
            return;
        }
        tryGetValue<I+1>(o, v);
    }
    
    std::optional<T> operator()(const Object& obj)
    {
        std::optional<T> t;
        tryGetValue<0>(obj, t);
        return t;
    }
};

struct Object
{
    virtual std::string toString() const = 0;
    virtual bool isList() const { return false; }
    virtual bool isNil() const { return false; }
    virtual bool isInt() const { return false; }
    virtual bool isFloat() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool operator!() const { return false; }
    virtual ~Object() {}

    virtual Function* resolveFunction() { return nullptr; }
    virtual std::unique_ptr<Object> clone() const = 0;
    virtual bool equals(const Object& o) const
    {
        return false;
    }

    virtual ConsCellObject* asList() { return nullptr; }
    virtual const ConsCellObject* asList() const { return nullptr; }

    friend std::ostream &operator<<(std::ostream &os, const Object &sym);

    template <typename T> T value() const
    {
        const std::optional<T> opt = Converter<T>()(*this);
        if (opt) {
            return *opt;
        }
        return T();
    }

    template <typename T> std::optional<T> valueOrNull() const
    {
        Converter<T> conv;
        return conv(*this);
    }

    virtual std::unique_ptr<Object> eval()
    {
        return clone();
    }
};

struct Sequence
{
    virtual size_t length() const = 0;
};

inline std::ostream &operator<<(std::ostream &os, const Object &sym)
{
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
    std::unique_ptr<Object> car;
    std::unique_ptr<Object> cdr;

    const ConsCell* next() const;
    ConsCell* next();
    bool operator!() const { return !car; }

    struct Iterator
    {
        ConsCell* ptr;
        bool operator!=(const Iterator& rhs) const { return ptr != rhs.ptr; }
        Iterator& operator++()
        {
            ptr = ptr->next();
            return *this;
        }

        Object& operator*() {
            return *ptr->car.get();
        }
    };

    Iterator begin() { return !*this ? end() : Iterator{this}; }
    Iterator end() { return Iterator{nullptr}; }

    void traverse(const std::function<bool(const ConsCell*)>& f) const;
    bool isCyclical() const;
};

struct StringObject : Object, Sequence
{
    std::shared_ptr<std::string> value;

    StringObject(std::string value) : value(std::make_shared<std::string>(value)) {}
    StringObject(const StringObject& o) : value(o.value) {}

    std::string toString() const override { return "\"" + *value + "\""; }

    bool isString() const override { return true; }

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<StringObject>(*this);
    }

    bool equals(const Object& o) const override
    {
        const StringObject* str = dynamic_cast<const StringObject*>(&o);
        return str && str->value == value;
    }

    size_t length() const override { return value->size(); }
};

template<typename T>
struct ValueObject : Object
{
    T value;
    ValueObject(T t) : value(t) {}
    bool equals(const Object& o) const override
    {
        const ValueObject<T>* op = dynamic_cast<const ValueObject<T>*>(&o);
        return op && op->value == value;
    }
    std::string toString() const override { return std::to_string(value); }
};

struct IntObject : ValueObject<std::int64_t>
{
    IntObject(std::int64_t value) : ValueObject<std::int64_t>(value) {}
    bool isInt() const override { return true; }
    std::unique_ptr<Object> clone() const override { return std::make_unique<IntObject>(value); }
};

struct FloatObject : ValueObject<double>
{
    FloatObject(double value) : ValueObject<double>(value) {}
    bool isFloat() const override { return true; }
    std::unique_ptr<Object> clone() const override { return std::make_unique<FloatObject>(value); }
};

struct ConsCellObject : Object, Sequence
{
    std::shared_ptr<ConsCell> cc;

    ConsCellObject() { cc = std::make_shared<ConsCell>(); }

    ConsCellObject(std::unique_ptr<Object> car, std::unique_ptr<Object> cdr) :
        ConsCellObject()
    {
        this->cc->car = std::move(car);
        if (!cdr || !(*cdr)) {
            return;
        }
        this->cc->cdr = std::move(cdr);
    }

    std::string toString() const override;
    bool isList() const override { return true; }
    bool isNil() const override { return !(*this); }
    bool operator!() const override { return !(*cc); }
    ConsCellObject* asList() override { return this; }
    const ConsCellObject* asList() const override { return this; }
    Object* car() const { return cc->car.get();  };
    Object* cdr() const { return cc->cdr.get();  };

    std::unique_ptr<Object> clone() const override
    {
        auto copy = std::make_unique<ConsCellObject>();
        copy->cc = cc;
        return copy;
    }

    bool equals(const Object &o) const override
    {
        const ConsCellObject *op = dynamic_cast<const ConsCellObject *>(&o);
        if (op && !(*this) && !(*op)) {
            return true;
        }
        return this->cc == op->cc;
    }

    size_t length() const override
    {
        if (!*this) {
            return 0;
        }
        std::set<const ConsCell*> visited;
        size_t l = 1;
        auto p = cc.get();
        visited.insert(p);
        while ((p = p->next())) {
            if (visited.count(p)) {
                throw exceptions::Error("Cyclical list length");
            }
            visited.insert(p);
            l++;
        }
        return l;
    }

    std::unique_ptr<Object> eval() override;

    ConsCell::Iterator begin() const
    {
        return cc->begin();
    }
    
    ConsCell::Iterator end() const
    {
        return cc->end();
    }

    std::unique_ptr<ConsCellObject> deepCopy() const
    {
        std::unique_ptr<ConsCellObject> copy = std::make_unique<ConsCellObject>();
        ConsCell *origPtr = cc.get();
        ConsCell *copyPtr = copy->cc.get();
        assert(origPtr && copyPtr);
        while (origPtr) {
            if (origPtr->car) {
                if (origPtr->car->isList()) {
                    const auto list = dynamic_cast<ConsCellObject*>(origPtr->car.get());
                    copyPtr->car = list->deepCopy();
                }
                else {
                    copyPtr->car = origPtr->car->clone();
                }
            }
            auto cdr = origPtr->cdr.get();
            origPtr = origPtr->next();
            if (origPtr) {
                copyPtr->cdr = std::make_unique<ConsCellObject>();
                copyPtr = copyPtr->next();
            }
            else if (cdr) {
                assert(!cdr->isList());
                copyPtr->cdr = cdr->clone(); 
            }
        }
        return copy;
    }
};

struct FArgs;

struct Function
{
    std::string name;
    int minArgs = 0;
    int maxArgs = 0xffff;
    bool isMacro = false;
    std::unique_ptr<ConsCellObject> macroCode;
    std::function<std::unique_ptr<Object>(FArgs&)> func;
};

struct Symbol
{
    Machine* parent = nullptr;
    bool constant = false;
    std::string name;
    std::unique_ptr<Object> variable;
    std::unique_ptr<Function> function;
};

struct SymbolObject : Object
{
    std::shared_ptr<Symbol> sym;
    std::string name;
    Machine* parent;

    SymbolObject(Machine* parent,
                 std::shared_ptr<Symbol> sym = nullptr,
                 std::string name = "") :
        parent(parent),
        sym(sym),
        name(name)
    {

    }

    const std::string getSymbolName()
    {
        return sym ? sym->name : name;
    };

    Symbol* getSymbol() const;
    Symbol* getSymbolOrNull() const;

    Function* resolveFunction() override;

    std::string toString() const override
    {
        // Interned symbols with empty string as name have special printed form of ##. See:
        // https://www.gnu.org/software/emacs/manual/html_node/elisp/Special-Read-Syntax.html
        std::string n = sym ? sym->name : name;
        return n.size() ? n : "##";
    }

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<SymbolObject>(parent, sym, name);
    }

    std::unique_ptr<Object> eval() override;

    bool equals(const Object& o) const override
    {
        const SymbolObject* op = dynamic_cast<const SymbolObject*>(&o);
        if (!op) {
            return false;
        }
        const Symbol* lhs = getSymbolOrNull();
        const Symbol* rhs = op->getSymbolOrNull();
        return lhs == rhs;
    }
};

// Remember nil = ()
std::unique_ptr<Object> makeNil() { return std::make_unique<ConsCellObject>(); }

std::unique_ptr<ConsCellObject> makeList() {
    return std::make_unique<ConsCellObject>();
}

std::unique_ptr<IntObject> makeInt(std::int64_t value) {
    return std::make_unique<IntObject>(value);
}

std::unique_ptr<FloatObject> makeFloat(double value) {
    return std::make_unique<FloatObject>(value);
}

int countArgs(const ConsCell* cc)
{
    if (!cc || !(*cc)) {
        return 0;
    }
    int i = 0;
    while (cc) {
        i++;
        cc = cc->next();
    }
    return i;
}

struct FArgs
{
    ConsCell* cc;
    Machine& m;
    
    FArgs(ConsCell& cc, Machine& m) : cc(&cc), m(m) {}

    std::unique_ptr<Object> get();
    
    void skip()
    {
        cc = cc->next();
    }

    bool hasNext() const
    {
        return cc;
    }

    struct Iterator
    {
        Machine* m;
        ConsCell* cc;

        bool operator!=(const Iterator& o) const
        {
            return cc != o.cc;
        }

        void operator++()
        {
            cc = cc->next();
        }

        std::unique_ptr<Object> operator*();
    };

    Iterator begin()
    {
        return Iterator{&m, cc};
    }

    Iterator end()
    {
        return Iterator{&m, nullptr};
    }
};

std::unique_ptr<Object> ConsCellObject::eval()
{
    auto &c = *cc;
    if (!c) {
        return std::make_unique<ConsCellObject>();
    }
    const Function* f = c.car->resolveFunction();
    if (f) {
        auto &m = *dynamic_cast<const SymbolObject*>(c.car.get())->parent;
        const int argc = countArgs(c.next());
        if (argc < f->minArgs || argc > f->maxArgs) {
            throw exceptions::WrongNumberOfArguments(argc);
        }
        FArgs args(*c.next(), m);
        return f->func(args);
    }
    throw exceptions::VoidFunction(c.car->toString());
}

template<size_t I, typename ...Args>
inline typename std::enable_if<I == sizeof...(Args), void>::type writeToTuple(std::tuple<Args...>&,
                                                                              FArgs&)
{}

template <size_t I, typename... Args>
inline constexpr typename std::enable_if<I == std::tuple_size_v<std::tuple<Args...>>,
                                         bool>::type
tupleOptCheck()
{
    return true;
}

template <size_t I, typename... Args>
inline constexpr typename std::enable_if<I < std::tuple_size_v<std::tuple<Args...>>,
                                             bool>::type
tupleOptCheck()
{
    using T = typename std::tuple_element<I, std::tuple<Args...>>::type;
    constexpr bool isOptionalParam = OptCheck<T>::value;
    return isOptionalParam;
}
    
    template<size_t I, typename ...Args>
    inline typename std::enable_if<I < sizeof...(Args), void>::type writeToTuple(std::tuple<Args...>& t,
                                                                                 FArgs& args)
    {
        using T = typename std::tuple_element<I, std::tuple<Args...>>::type;
        constexpr bool isOptionalParam = OptCheck<T>::value;
        static_assert(!isOptionalParam || tupleOptCheck<I+1, Args...>(),
                      "Non-optional function param given after optional one. ");
        std::optional<typename OptCheck<T>::BaseType> opt;
        std::unique_ptr<Object> arg;
        bool conversionFailed = false;
        if (args.hasNext()) {
            arg = args.get();
            opt = arg->valueOrNull<typename OptCheck<T>::BaseType>();
            if (!opt) {
                if (isOptionalParam && arg->isNil()) {
                    // nil => std::nullopt makes sense
                }
                else {
                    conversionFailed = true;
                }
            }
        }
        if (!opt && (!isOptionalParam || conversionFailed)) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        if (opt) {
            std::get<I>(t) = std::move(*opt);
        }
        writeToTuple<I + 1>(t, args);
    }

        template<typename ...Args>
        std::tuple<Args...> toTuple(FArgs& args)
        {
            std::tuple<Args...> tuple;
            writeToTuple<0>(tuple, args);
            return tuple;
        }

void cons(std::unique_ptr<Object> sym, ConsCell& list)
{
    if (!list) {
        list.car = std::move(sym);
        return;
    }
    ConsCell cdr = std::move(list);
    list.car = std::move(sym);
    list.cdr = std::make_unique<ConsCellObject>();
    auto &lo = dynamic_cast<ConsCellObject &>(*list.cdr.get());
    lo.cc->car = std::move(cdr.car);
    lo.cc->cdr = std::move(cdr.cdr);
}

void cons(std::unique_ptr<Object> sym,
          const std::unique_ptr<ConsCellObject> &list) {
    cons(std::move(sym), *list->cc);
}

bool isPartOfSymName(const char c)
{
    if (c=='.') return true;
    if (c=='+') return true;
    if (c=='%') return true;
    if (c=='*') return true;
    if (c=='/') return true;
    if (c=='-') return true;
    if (c>='a' && c<='z') return true;
    if (c>='A' && c<='Z') return true;
    if (c>='0' && c<='9') return true;
    return false;
}

bool isWhiteSpace(const char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
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
inline std::optional<std::shared_ptr<Object>> getValue(const Object& sym)
{
    return std::shared_ptr<Object>(sym.clone().release());
}

template<>
inline std::optional<double> getValue(const Object &sym)
{
    auto s = dynamic_cast<const FloatObject*>(&sym);
    if (s) {
        return s->value;
    }
    return std::nullopt;
}

template <> inline std::optional<ConsCellObject> getValue(const Object& sym)
{
    auto s = dynamic_cast<const ConsCellObject*>(&sym);
    if (s) {
        return *s;
    }
    return std::nullopt;
}

template<>
inline std::optional<bool> getValue(const Object &sym)
{
    return !!sym;
}

template<>
inline std::optional<std::any> getValue(const Object& sym)
{
    auto s = dynamic_cast<const ConsCellObject*>(&sym);
    if (s) {
        std::shared_ptr<ConsCell> cc = s->cc;
        return cc;
    }
    return std::any();
}

template<>
inline std::optional<std::int64_t> getValue(const Object &sym)
{
    auto s = dynamic_cast<const IntObject*>(&sym);
    if (s) {
        return s->value;
    }
    return std::nullopt;
}

template<>
inline std::optional<std::string> getValue(const Object& sym)
{
    auto s = dynamic_cast<const StringObject*>(&sym);
    if (s) {
        return *s->value;
    }
    return std::nullopt;
}

template<>
inline std::optional<const Symbol*> getValue(const Object& sym)
{
    auto s = dynamic_cast<const SymbolObject*>(&sym);
    if (s) {
        return s->sym.get();
    }
    return std::nullopt;
}

template<>
inline std::optional<const Sequence*> getValue(const Object& sym)
{
    if (sym.isString() || sym.isList()) {
        return dynamic_cast<const Sequence*>(&sym);
    }
    return std::nullopt;
}

template <typename... Args>
inline size_t getMinArgs()
{
    return countNonOpts<0, Args...>();
}

class Machine
{
    std::map<std::string, std::shared_ptr<Symbol>> m_syms;
    std::function<void(std::string)> m_msgHandler;

    std::map<std::string, std::vector<std::unique_ptr<Object>>> m_locals;

    void pushLocalVariable(std::string name, std::unique_ptr<Object> obj)
    {
        // std::cout << "Push " << name << "=" << obj->toString() << std::endl;
        m_locals[name].push_back(std::move(obj));
    }

    bool popLocalVariable(std::string name)
    {
        assert(m_locals[name].size());
        // std::cout << "popped " << name << " with value=" <<
        // m_locals[name].back()->toString() << std::endl;
        m_locals[name].pop_back();
        if (m_locals[name].empty()) {
            m_locals.erase(name);
        }
        return true;
    }

    template <typename T>
    std::unique_ptr<Object> makeObject(T);

    template<> std::unique_ptr<Object> makeObject(std::int64_t i) { return makeInt(i); }
    template<> std::unique_ptr<Object> makeObject(int i) { return makeInt(i); }
    template<> std::unique_ptr<Object> makeObject(size_t i) { return makeInt((std::int64_t)i); }
    template<> std::unique_ptr<Object> makeObject(std::string str)
    {
        return std::make_unique<StringObject>(str);
    }
    template<> std::unique_ptr<Object> makeObject(bool value)
    {
        return value ? makeTrue() : makeNil();
    }
    template<> std::unique_ptr<Object> makeObject(std::unique_ptr<Object> o)
    {
        return std::move(o);
    }
    template<> std::unique_ptr<Object> makeObject(std::shared_ptr<Object> o)
    {
        return std::unique_ptr<Object>(o->clone());
    }
    
    template<typename R, typename ...Args>
    void makeFuncInternal(const char* name, std::function<R(Args...)> f)
    {
        std::function<std::unique_ptr<Object>(FArgs&)> w = [=](FArgs& args) {
            std::tuple<Args...> t = toTuple<Args...>(args);
            return makeObject<R>(std::apply(f, t));
        };

        auto func = std::make_unique<Function>();
        func->name = name;
        func->minArgs = getMinArgs<Args...>();
        func->maxArgs = sizeof...(Args);
        func->func = w;
        getSymbol(name)->function = std::move(func);
    }

    std::unique_ptr<Object> makeTrue()
    {
        return std::make_unique<SymbolObject>(this, nullptr, "t");
    }

    std::string parseNextName(const char*& str)
    {
        std::string name;
        while (*str && isPartOfSymName(*str)) {
            name += *str;
            str++;
        }
        return name;
    }

    std::unique_ptr<Object> getNumericConstant(const std::string& str) const
    {
        size_t dotCount = 0;
        size_t digits = 0;
        for (size_t i = 0;i < str.size(); i++) {
            const char c = str[i];
            if (c == '.') {
                dotCount++;
                if (dotCount == 2) {
                    return nullptr;
                }
            }
            else if (c == '+') {
                if (i > 0) {
                    return nullptr;
                }
            }
            else if (c == '-') {
                if (i > 0) {
                    return nullptr;
                }
            }
            else if (c >= '0' && c <= '9') {
                digits++;
            }
            else {
                return nullptr;
            }
        }
        if (!digits) {
            return nullptr;
        }
        std::stringstream ss(str);
        if (dotCount) {
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

    std::unique_ptr<Object> parseNamedObject(const char*& str)
    {
        const std::string next = parseNextName(str);
        auto num = getNumericConstant(next);
        if (num) {
            return num;
        }
        if (next == "nil") {
            // It's optimal to return nil already at this point.
            // Note that even the GNU elisp manual says:
            // 'After the Lisp reader has read either `()' or `nil', there is no way to determine
            //  which representation was actually written by the programmer.'
            return makeNil();
        }
        return std::make_unique<SymbolObject>(this, nullptr, next);
    }
    
    std::unique_ptr<StringObject> parseString(const char *&str)
    {
        auto sym = std::make_unique<StringObject>("");
        while (*str && *str != '"') {
            *sym->value += *str;
            str++;
        }
        if (!*str) {
            throw std::runtime_error("unexpected end of file");
        }
        str++;
        return sym;
    }

    std::unique_ptr<Object> quote(std::unique_ptr<Object> obj)
    {
        std::unique_ptr<ConsCellObject> list = std::make_unique<ConsCellObject>();
        list->cc->car = std::make_unique<SymbolObject>(this, nullptr, "quote");
        std::unique_ptr<ConsCellObject> cdr = std::make_unique<ConsCellObject>();
        cdr->cc->car = std::move(obj);
        list->cc->cdr = std::move(cdr);
        return list;
    }

    std::unique_ptr<Object> parseNext(const char *&expr)
    {
        while (*expr) {
            const char c = *expr;
            const char n = *(expr+1);
            if (isWhiteSpace(c)) {
                expr++;
                continue;
            }
            if (c == '\"') {
                return parseString(++expr);
            }
            else if (isPartOfSymName(c))
            {
                return parseNamedObject(expr);
            }
            else if (c == '\'') {
                expr++;
                return quote(parseNext(expr));
            }
            else if (c == '(') {
                auto l = makeList();
                bool dot = false;
                auto lastConsCell = l->cc.get();
                assert(lastConsCell);
                expr++;
                skipWhitespace(expr);
                while (*expr != ')' && *expr) {
                    assert(!dot);
                    if (*expr == '.') {
                        auto old = expr;
                        const std::string nextName = parseNextName(expr);
                        if (nextName == ".") {
                            dot = true;
                        }
                        else {
                            expr = old;                            
                        }
                    }
                    auto sym = parseNext(expr);
                    skipWhitespace(expr);
                    if (dot) {
                        assert(lastConsCell->car);
                        lastConsCell->cdr = std::move(sym);
                    }
                    else {
                        if (lastConsCell->car) {
                            assert(!lastConsCell->cdr);
                            lastConsCell->cdr = std::make_unique<ConsCellObject>();
                            assert(lastConsCell != lastConsCell->next());
                            lastConsCell = lastConsCell->next();
                        }
                        lastConsCell->car = std::move(sym);
                    }
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
                throw exceptions::SyntaxError(os.str());
            }
        }
        return nullptr;
    }

    void renameSymbols(ConsCellObject& obj, std::map<std::string, std::unique_ptr<Object>>& conv)
    {
        auto p = obj.cc.get();
        while (p) {
            auto& obj = *p->car.get();
            SymbolObject* sym = dynamic_cast<SymbolObject*>(&obj);
            if (sym && conv.count(sym->name)) {
                p->car = quote(conv[sym->name]->clone());
            }
            ConsCellObject* cc = dynamic_cast<ConsCellObject*>(&obj);
            if (cc) {
                renameSymbols(*cc, conv);
            }
            p = p->next();
        }
    }

public:
    std::unique_ptr<Object> parse(const char *expr)
    {
        auto r = parseNext(expr);
        if (onlyWhitespace(expr)) {
            return r;
        }

        auto prog = makeList();
        prog->cc->car = std::make_unique<SymbolObject>(this, nullptr, "progn");

        auto consCell = makeList();
        consCell->cc->car = std::move(r);
        auto lastCell = consCell->cc.get();
    
        while (!onlyWhitespace(expr)) {
            auto n = parseNext(expr);
            lastCell->cdr = std::make_unique<ConsCellObject>(std::move(n), nullptr);
            lastCell = lastCell->next();
        }
        prog->cc->cdr = std::move(consCell);
        return prog;
    }

    std::unique_ptr<Object> evaluate(const char *expr)
    {
        return parse(expr)->eval();
    }

    Object* resolveVariable(const std::string& name)
    {
        if (m_locals.count(name)) {
            return m_locals[name].back().get();
        }
        if (m_syms.count(name)) {
            return m_syms[name]->variable.get();
        }
        return nullptr;
    }

    Function* resolveFunction(const std::string& name)
    {
        if (m_syms.count(name)) {
            return m_syms[name]->function.get();
        }
        return nullptr;
    }

    void makeFunc(const char *name, int minArgs, int maxArgs,
                  const std::function<std::unique_ptr<Object>(FArgs &)>& f)
    {
        auto func = std::make_unique<Function>();
        func->name = name;
        func->minArgs = minArgs;
        func->maxArgs = maxArgs;
        func->func = std::move(f);
        getSymbol(name)->function = std::move(func);
    }

    Machine()
    {
        setVariable("nil", makeNil(), true);
        setVariable("t", std::make_unique<SymbolObject>(this, getSymbol("t"), ""), true);
        
        defun("atom", [](std::any obj) {
            if (obj.type() != typeid(std::shared_ptr<ConsCell>)) return true;
            std::shared_ptr<ConsCell> cc = std::any_cast<std::shared_ptr<ConsCell>>(obj);
            return !(cc && !!(*cc));
        });
        defun("null", [](bool isNil) { return !isNil; });
        defun("setcar", [](ConsCellObject obj, std::shared_ptr<Object> newcar) {
            return obj.cc->car = newcar->clone(), newcar;
        });
        defun("setcdr", [](ConsCellObject obj, std::shared_ptr<Object> newcdr) {
            return obj.cc->cdr = newcdr->clone(), newcdr;
        });
        defun("car", [](ConsCellObject obj) { 
            return obj.cc->car ? obj.cc->car->clone() : makeNil();
        });
        defun("cdr", [](ConsCellObject obj) {
            return obj.cc->cdr ? obj.cc->cdr->clone() : makeNil();
        });
        defun("1+", [](std::variant<double, std::int64_t> obj) -> std::unique_ptr<Object> {
            try {
                const std::int64_t i = std::get<std::int64_t>(obj);
                return makeInt(i+1);
            }
            catch (std::bad_variant_access&) {
                const double f = std::get<double>(obj);
                return makeFloat(f+1.0);
            }
        });
        defun("consp", [](std::any obj) {
            if (obj.type() != typeid(std::shared_ptr<ConsCell>)) return false;
            std::shared_ptr<ConsCell> cc = std::any_cast<std::shared_ptr<ConsCell>>(obj);
            return cc && !!(*cc);
        });
        defun("listp", [](std::any o) { return o.type() == typeid(std::shared_ptr<ConsCell>); });
        defun("nlistp", [](std::any o) { return o.type() != typeid(std::shared_ptr<ConsCell>); });
        defun("proper-list-p", [](std::any obj) {
            if (obj.type() != typeid(std::shared_ptr<ConsCell>)) return makeNil();
            std::shared_ptr<ConsCell> cc = std::any_cast<std::shared_ptr<ConsCell>>(obj);
            std::unique_ptr<Object> r;
            auto p = cc.get();
            if (cc->isCyclical()) {
                return makeNil();
            }
            std::int64_t count = p->car ? 1 : 0;
            while (p->cdr && p->cdr->isList()) {
                count++;
                p = dynamic_cast<ConsCellObject*>(p->cdr.get())->cc.get();
                if (p->cdr && !p->cdr->isList()) {
                    return makeNil();
                }
            }
            r = makeInt(count);
            return r;
        });
        makeFunc("defmacro", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
            const SymbolObject* nameSym = dynamic_cast<SymbolObject*>(args.cc->car.get());
            if (!nameSym || nameSym->name.empty()) {
                throw exceptions::WrongTypeArgument(args.cc->car->toString());
            }
            std::string macroName = nameSym->name;
            args.skip();
            ConsCellObject argList = dynamic_cast<ConsCellObject&>(*args.cc->car);
            const int argc = countArgs(argList.cc.get());
            args.skip();
            ConsCellObject code = dynamic_cast<ConsCellObject&>(*args.cc->car);
            makeFunc(macroName.c_str(), argc, argc,
                     [this, macroName, argList, code](FArgs& a) mutable {
                         size_t i = 0;
                         std::map<std::string, std::unique_ptr<Object>> conv;
                         for (const auto& obj : argList) {
                             const SymbolObject* from =
                                 dynamic_cast<const SymbolObject*>(&obj);
                             conv[from->name] = a.cc->car.get()->clone();
                             a.skip();
                         }
                         auto copied = code.deepCopy();
                         renameSymbols(*copied, conv);
                         return copied->eval()->eval();
                     });
            auto sym = getSymbol(macroName);
            sym->function->isMacro = true;
            return std::make_unique<SymbolObject>(this, nullptr, std::move(macroName));
        });
        makeFunc("if", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
            if (!!*args.get()) {
                return args.get();
            }
            args.skip();
            while (auto res = args.get()) {
                if (!args.hasNext()) {
                    return res;
                }
            }
            return makeNil();
        });
        auto let = [this](FArgs& args, bool star) {
            std::vector<std::string> varList;
            std::vector<std::pair<std::string, std::unique_ptr<Object>>> pushList;
            for (auto& arg : *args.cc->car->asList()) {
                assert(arg.isList());
                auto list = arg.asList();
                auto cc = list->cc.get();
                const auto sym = dynamic_cast<const SymbolObject*>(cc->car.get());
                assert(sym && sym->name.size());
                if (star) {
                    pushLocalVariable(sym->name, cc->cdr->asList()->cc->car->eval());
                    varList.push_back(sym->name);
                }
                else {
                    pushList.emplace_back(sym->name, cc->cdr->asList()->cc->car->eval());
                }
            }
            for (auto& push : pushList) {
                pushLocalVariable(push.first, std::move(push.second));
                varList.push_back(push.first);
            }
            AtScopeExit onExit([this, varList]() {
                for (auto it = varList.rbegin(); it != varList.rend(); ++it) {
                    popLocalVariable(*it);
                }
            });
            args.skip();
            std::unique_ptr<Object> res = makeNil();
            for (auto& obj : *args.cc) {
                res = obj.eval();
            }
            return res;
        };
        defun("length", [](const Sequence* ptr) { return ptr->length(); });
        makeFunc("let", 2, std::numeric_limits<int>::max(),
                 std::bind(let, std::placeholders::_1, false));
        makeFunc("let*", 2, std::numeric_limits<int>::max(),
                 std::bind(let, std::placeholders::_1, true));
        defun("make-list", [this](std::int64_t n, std::shared_ptr<Object> ptr) {
            std::unique_ptr<Object> r = makeNil();
            for (std::int64_t i=0; i < n; i++) {
                r = std::make_unique<ConsCellObject>(ptr->clone(), r->clone());
            }
            return r;
        });
        makeFunc("defun", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
            const SymbolObject* nameSym = dynamic_cast<SymbolObject*>(args.cc->car.get());
            if (!nameSym || nameSym->name.empty()) {
                throw exceptions::WrongTypeArgument(args.cc->car->toString());
            }
            std::string funcName = nameSym->name;
            args.skip();
            std::vector<std::string> argList;
            int argc = 0;
            for (auto& arg : *args.cc->car->asList()) {
                const auto sym = dynamic_cast<const SymbolObject*>(&arg);
                if (!sym) {
                    throw exceptions::Error("Malformed arglist: " + args.cc->car->toString());
                }
                argList.push_back(sym->name);
                argc++;
            }
            std::shared_ptr<Object> code;
            code.reset(args.cc->cdr.release());
            // std::cout << funcName << " defined as: " << *code << " (argc=" << argc << ")\n";
            makeFunc(funcName.c_str(), argc, argc,
                     [this, funcName, argList, code](FArgs& a) {
                         /*if (argList.size()) {
                             std::cout << funcName << " being called with args="
                                       << a.cc->toString() << std::endl;
                                       }*/
                         size_t i = 0;
                         for (size_t i = 0; i < argList.size(); i++) {
                             pushLocalVariable(argList[i], std::move(a.get()));
                         }
                         AtScopeExit onExit([this, argList]() {
                             for (auto it = argList.rbegin(); it != argList.rend(); ++it) {
                                 popLocalVariable(*it);
                             }
                         });
                         assert(code->asList());
                         std::unique_ptr<Object> ret = makeNil();
                         for (auto& obj : *code->asList()) {
                             //std::cout << "exec: " << obj.toString() << std::endl;
                             ret = obj.eval();
                             //std::cout << " => " << ret->toString() << std::endl;
                         }
                         return ret;
                     });
            return std::make_unique<SymbolObject>(this, nullptr, std::move(funcName));
        });
        makeFunc("quote", 1, 1, [](FArgs& args) {
            return args.cc->car && !args.cc->car->isNil() ? args.cc->car->clone() : makeNil();
        });
        makeFunc("stringp", 1, 1, [this](FArgs& args) {
            return (args.get()->isString()) ? makeTrue() : makeNil();
        });
        makeFunc("numberp", 1, 1, [this](FArgs& args) {
            auto arg = args.get();
            return arg->isInt() || arg->isFloat() ? makeTrue() : makeNil();
        });
        makeFunc("make-symbol", 1, 1, [this](FArgs& args) {
            const auto arg = args.get();
            if (!arg->isString()) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>();
            symbol->name = arg->value<std::string>();
            return std::make_unique<SymbolObject>(this, symbol, "");
        });
        makeFunc("symbolp", 1, 1, [this](FArgs& args) {
            return dynamic_cast<SymbolObject*>(args.get().get()) ? makeTrue() : makeNil();
        });
        makeFunc("symbol-name", 1, 1, [](FArgs& args) {
            const auto obj = args.get();
            const auto sym = dynamic_cast<SymbolObject*>(obj.get());
            if (!sym) {
                throw exceptions::WrongTypeArgument(obj->toString());
            }
            return std::make_unique<StringObject>(sym->getSymbolName());
        });
        makeFunc("eval", 1, 1, [](FArgs& args) { return args.get()->eval(); });
        makeFunc("message", 1, 0xffff, [this](FArgs& args) {
            auto arg = args.get();
            if (!arg->isString()) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            auto strSym = dynamic_cast<StringObject*>(arg.get());
            std::string str = *strSym->value;
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
            if (m_msgHandler) {
                m_msgHandler(str);
            }
            else {
                std::cout << str << std::endl;
            }
            return std::make_unique<StringObject>(str);
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
                    throw exceptions::WrongTypeArgument(sym->toString());
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
                    throw exceptions::WrongTypeArgument(sym->toString());
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
                    throw exceptions::WrongTypeArgument(sym->toString());
                }
            }
            return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
                : static_cast<std::unique_ptr<Object>>(makeInt(i));
        });
        makeFunc("progn", 0, 0xfffff, [](FArgs& args) {
            std::unique_ptr<Object> ret = makeNil();
            for (auto obj : args) {
                ret = std::move(obj);
            }
            return ret;
        });
        makeFunc("prog1", 0, 0xfffff, [](FArgs& args) {
            std::unique_ptr<Object> ret;
            for (auto obj : args) {
                assert(obj);
                if (!ret) {
                    ret = std::move(obj);
                }
            }
            if (!ret) ret = makeNil();
            return ret;
        });
        defun("intern", [this](std::string name) -> std::unique_ptr<Object> {
            return std::make_unique<SymbolObject>(this, getSymbol(name));
        });
        makeFunc("unintern", 1, 1, [this](FArgs& args) {
            const auto arg = args.get();
            const auto sym = dynamic_cast<SymbolObject*>(arg.get());
            if (!sym) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            const bool uninterned = m_syms.erase(sym->sym->name);
            return uninterned ? makeTrue() : makeNil();
        });
        makeFunc("intern-soft", 1, 1, [this](FArgs& args) {
            const auto arg = args.get();
            if (!arg->isString()) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            std::unique_ptr<Object> r;
            if (!m_syms.count(arg->value<std::string>())) {
                r = makeNil();
            }
            else {
                r = std::make_unique<SymbolObject>(this, getSymbol(arg->value<std::string>()));
            }
            return r;
        });
        makeFunc("set", 2, 2, [this](FArgs& args) {
            const SymbolObject nil(this, nullptr, "nil");
            const auto& p1 = args.get();
            const SymbolObject* name = p1->isNil() ? &nil : dynamic_cast<SymbolObject*>(p1.get());
            if (!name) {
                throw exceptions::WrongTypeArgument(p1->toString());
            }
            auto sym = name->getSymbol();
            assert(sym);
            if (sym->constant) {
                throw exceptions::Error("setting-constant " + name->toString());
            }
            sym->variable = std::move(args.get());
            return sym->variable->clone();
        });
        makeFunc("eq", 2, 2, [this](FArgs& args) {
            return args.get()->equals(*args.get()) ? makeTrue() : makeNil();
        });
        makeFunc("describe-variable", 1, 1, [this](FArgs& args) {
            const auto arg = args.get();
            std::string descr = "You did not specify a variable.";
            if (auto sym = dynamic_cast<SymbolObject*>(arg.get())) {
                const Object* var = nullptr;
                if (sym->sym) {
                    var = sym->sym->variable.get();
                }
                else {
                    assert(sym->name.size());
                    var = resolveVariable(sym->name);
                }
                if (!var) {
                    descr = arg->toString() + " is void as a variable.";
                }
                else {
                    descr = arg->toString() + "'s value is " + var->toString();
                }
            }
            else if (auto list = dynamic_cast<ConsCellObject *>(arg.get())) {
                if (!*list) {
                    descr = arg->toString() + "'s value is " + arg->toString();
                }
            }
            return std::make_unique<StringObject>(descr);
        });
        defun("%", [](std::int64_t in1, std::int64_t in2) { return in1 % in2; });
        defun("concat", [](std::string str1, std::string str2) { return str1 + str2; });
        defun("nth", [](std::int64_t index, ConsCellObject list) {
            auto p = list.cc.get();
            auto obj = list.cc->car.get();
            for (size_t i = 0; i < index; i++) {
                p = p->next();
                if (!p) {
                    return makeNil();
                }
                obj = p->car.get();
            }
            return obj->clone();
        });
        makeFunc("cons", 2, 2, [](FArgs &args) {
            return std::make_unique<ConsCellObject>(args.get(), args.get());
        });
        makeFunc("list", 0, std::numeric_limits<int>::max(), [](FArgs& args) {
            auto newList = makeList();
            ConsCell *lastCc = newList->cc.get();
            bool first = true;
            for (auto obj : args) {
                if (!first) {
                    lastCc->cdr = std::make_unique<ConsCellObject>();
                    lastCc = lastCc->next();
                }
                lastCc->car = obj->clone();
                first = false;
            }
            return newList;
        });
        defun("substring", [](std::string str,
                              std::optional<std::int64_t> start,
                              std::optional<std::int64_t> end) {
            if (start && *start < 0) {
                *start = static_cast<std::int64_t>(str.size()) + *start;
            }
            if (end && *end < 0) {
                *end = static_cast<std::int64_t>(str.size()) + *end;
            }
            return !start ? str : (!end ? str.substr(*start) : str.substr(*start, *end - *start));
        });
        makeFunc("pop", 1, 1, [this](FArgs& args) {
            auto arg = args.get();
            ConsCellObject *list = dynamic_cast<ConsCellObject *>(arg.get());
            if (!list) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            auto ret = list->cc->car->clone();
            return ret;
        });
        evaluate(getInitCode());
    }

    template<typename F>
    void defun(const char* name, F&& f)
    {
        makeFuncInternal(name, lambda_to_func(f));
    }

    void setVariable(std::string name, std::unique_ptr<Object> obj, bool constant = false)
    {
        assert(obj);
        auto s = getSymbol(name);
        s->variable = std::move(obj);
        s->constant = constant;
    }

    void setMessageHandler(std::function<void(std::string)> handler)
    {
        m_msgHandler = handler;
    }
    
    std::shared_ptr<Symbol> getSymbolOrNull(std::string name)
    {
        if (!m_syms.count(name)) {
            return nullptr;
        }
        return m_syms[name];
    }

    std::shared_ptr<Symbol> getSymbol(std::string name)
    {
        if (!m_syms.count(name)) {
            m_syms[name] = std::make_shared<Symbol>();
            m_syms[name]->name = name;
            m_syms[name]->parent = this;
        }
        return m_syms[name];
    }
};

std::unique_ptr<Object> SymbolObject::eval() 
{
    const auto var = sym ? sym->variable.get() : parent->resolveVariable(name);
    if (!var) {
        throw exceptions::VoidVariable(toString());
    }
    return var->clone();
}

Function* SymbolObject::resolveFunction()
{
    if (sym) {
        return sym->function.get();
    }
    assert(name.size());
    return parent->resolveFunction(name);
}

std::unique_ptr<Object> FArgs::get()
{
    if (!cc) {
        return nullptr;
    }
    auto sym = cc->car->eval();
    cc = cc->next();
    return sym;
}

std::unique_ptr<Object> FArgs::Iterator::operator*() { return cc->car->eval(); }

const ConsCell* ConsCell::next() const
{
    auto cc = dynamic_cast<ConsCellObject *>(this->cdr.get());
    if (cc) {
        return cc->cc.get();
    }
    return nullptr;
}

ConsCell* ConsCell::next()
{
    auto cc = dynamic_cast<ConsCellObject *>(this->cdr.get());
    if (cc) {
        return cc->cc.get();
    }
    return nullptr;
}

Symbol* SymbolObject::getSymbolOrNull() const
{
    return sym ? sym.get() : parent->getSymbolOrNull(name).get();
}

Symbol* SymbolObject::getSymbol() const
{
    return sym ? sym.get() : parent->getSymbol(name).get();
}

std::string ConsCellObject::toString() const
{
    if (!cc->car && !cc->cdr) {
        return "nil";
    }

    std::vector<const ConsCell*> cellPtrs;
    auto p = cc.get();
    bool infinite = false;
    while (p) {
        auto it = std::find(cellPtrs.begin(), cellPtrs.end(), p);
        if (it != cellPtrs.end()) {
            infinite = true;
            break;
        }
        cellPtrs.push_back(p);
        p = p->next();
    }

    const SymbolObject* carSym = dynamic_cast<const SymbolObject*>(car());
    const bool quote = carSym && carSym->name == "quote";
    if (quote) {
        return "'" + (cc->next() ? cc->next()->car->toString() : std::string(""));
    }

    auto carToString = [&](const Object* car) -> std::string {
        if (car->isList()) {
            const ConsCell* car2 = car->asList()->cc.get();
            if (car2->isCyclical()) {
                std::int64_t i = -1;
                car2->traverse([&](const ConsCell* cell) {
                    auto it = std::find(cellPtrs.begin(), cellPtrs.end(), car2);
                    if (it != cellPtrs.end()) {
                        i = std::distance(cellPtrs.begin(), it);
                        return false;
                    }
                    return true;
                });
                return "#" + std::to_string(i);
            }
        }
        return car->toString();
    };
        
    std::string s = "(";
    const ConsCell *t = cc.get();
    std::map<const ConsCell*, size_t> visited;
    std::int64_t index = 0;
    std::optional<std::int64_t> loopback;
    while (t) {
        if (!t->next() && t->cdr) {
            s += t->car->toString() + " . " + t->cdr->toString();
            break;
        }
        else if (infinite && visited[t->next()] == (cellPtrs.size() > 1 ? 2 : 1)) {
            s += ". ";
            if (cellPtrs.size() == 1) {
                loopback = 0;
            }
            s += "#" + std::to_string(*loopback);
            break;
        }
        if (infinite && !loopback && visited[t->next()]) {
            loopback = index;
        }
        visited[t]++;
        index++;
        s += t->car ? carToString(t->car.get()) : "";
        t = t->next();
        if (t) {
            s += " ";
        }
    }
    s += ")";
    return s;
}

void ConsCell::traverse(const std::function<bool(const ConsCell*)>& f) const
{
    const ConsCell* cell = this;
    while (cell) {
        if (!f(cell)) {
            return;
        }
        if (cell->car->isList()) {
            cell->car->asList()->cc.get()->traverse(f);
        }
        cell = cell->next();
    }
}

bool ConsCell::isCyclical() const
{
    if (!*this) {
        return false;
    }
    std::set<const ConsCell*> visited;
    bool cycled = false;
    traverse([&](const ConsCell* cell) {
        if (visited.count(cell)) {
            cycled = true;
            return false;
        }
        visited.insert(cell);
        return true;
    });
    return cycled;
}

}
