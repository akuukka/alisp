#include <cstdint>
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

template <typename T> std::optional<T> getValue(const Object &sym);

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

    virtual bool operator==(std::int64_t value) const { return false; };
    virtual Function* resolveFunction() { return nullptr; }
    virtual std::unique_ptr<Object> clone() const = 0;
    virtual bool equals(const Object& o) const
    {
        return false;
    }

    friend std::ostream &operator<<(std::ostream &os, const Object &sym);

    template <typename T> T value() const
    {
        const std::optional<T> opt = getValue<T>(*this);
        if (opt) {
            return *opt;
        }
        return T();
    }

    template <typename T> std::optional<T> valueOrNull() const
    {
        return getValue<T>(*this);
    }

    virtual std::unique_ptr<Object> eval()
    {
        return clone();
    }
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

    std::string toString() const
    {
        if (!car && !cdr) {
            return "nil";
        }
        std::string s = "(";
        const ConsCell *t = this;
        while (t) {
            s += t->car ? t->car->toString() : "";
            if (!t->next() && t->cdr) {
                s += " . " + cdr->toString();
                break;
            }
            t = t->next();
            if (t) {
                s += " ";
            }
        }
        s += ")";
        return s;
    }

    const ConsCell* next() const;
    ConsCell* next();

    bool operator!() const
    {
        return !car;
    }
};

struct FArgs;

struct Function
{
    std::string name;
    int minArgs = 0;
    int maxArgs = 0xffff;
    std::function<std::unique_ptr<Object>(FArgs&)> func;
};

struct Symbol
{
    Machine* parent = nullptr;
    std::string name;
    std::unique_ptr<Object> variable;
    std::unique_ptr<Function> function;
};

struct StringObject : Object
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

    bool equals(const Object& o) const override
    {
        const IntObject* op = dynamic_cast<const IntObject*>(&o);
        return op && op->value == value;
    }
};

struct FloatObject : Object
{
    double value;

    FloatObject(double value) : value(value) {}

    std::string toString() const override { return std::to_string(value); }

    bool isFloat() const override { return true; }

    bool operator==(std::int64_t value) const override
    {
        return this->value == value;
    };

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<FloatObject>(value);
    }

    bool equals(const Object& o) const override
    {
        const FloatObject* op = dynamic_cast<const FloatObject*>(&o);
        return op && op->value == value;
    }
};

struct ConsCellObject : Object
{
    std::shared_ptr<ConsCell> cc;
    bool quoted = false;

    ConsCellObject() { cc = std::make_shared<ConsCell>(); }

    ConsCellObject(std::unique_ptr<Object> car, std::unique_ptr<Object> cdr)
        : ConsCellObject() {
        this->cc->car = std::move(car);
        if (!(*cdr)) {
            return;
        }
        this->cc->cdr = std::move(cdr);
    }

    std::string toString() const override {
        return (quoted ? "'" : "") + cc->toString();
    }

    bool isList() const override { return true; }
    bool isNil() const override { return !(*this); }
    bool operator!() const override { return !(*cc); }

    std::unique_ptr<Object> clone() const override {
        auto copy = std::make_unique<ConsCellObject>();
        copy->cc = cc;
        copy->quoted = quoted;
        return copy;
    }

    std::unique_ptr<ConsCellObject> deepCopy() const {
        std::unique_ptr<ConsCellObject> copy = std::make_unique<ConsCellObject>();
        ConsCell *origPtr = cc.get();
        ConsCell *copyPtr = copy->cc.get();
        assert(origPtr && copyPtr);
        while (origPtr) {
            if (origPtr->car) {
                copyPtr->car = origPtr->car->clone();
            }
            origPtr = origPtr->next();
            if (origPtr) {
                copyPtr->cdr = std::make_unique<ConsCellObject>();
                copyPtr = copyPtr->next();
            }
        }

        return copy;
    }

    bool equals(const Object &o) const override {
        const ConsCellObject *op = dynamic_cast<const ConsCellObject *>(&o);
        if (op && !(*this) && !(*op)) {
            return true;
        }
        return this->cc == op->cc;
    }

    std::unique_ptr<Object> eval() override;
};

struct NamedObject : Object
{
    Machine* parent;
    std::string name;

    std::string toString() const override
    {
        return name;
    }

    Function* resolveFunction() override;

    NamedObject(Machine *parent) : parent(parent) {}

    std::unique_ptr<Object> clone() const override
    {
        auto sym = std::make_unique<NamedObject>(parent);
        sym->name = name;
        return sym;
    }

    std::unique_ptr<Object> eval() override;
};

struct SymbolObject : Object
{
    std::shared_ptr<Symbol> sym;
    bool quoted = false;

    SymbolObject(std::shared_ptr<Symbol> sym, bool quoted) : sym(sym), quoted(quoted) {}

    std::string toString() const override
    {
        return (quoted ? "'" : "") + sym->name;
    }

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<SymbolObject>(sym, quoted);
    }

    std::unique_ptr<Object> eval() override
    {
        return std::make_unique<SymbolObject>(sym, false);
    }

    bool equals(const Object& o) const override
    {
        const SymbolObject* op = dynamic_cast<const SymbolObject*>(&o);
        return op && op->sym == sym;
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

std::unique_ptr<Object> ConsCellObject::eval() {
    if (quoted) {
        auto cloned = clone();
        dynamic_cast<ConsCellObject *>(cloned.get())->quoted = false;
        return cloned;
    }
    auto &c = *cc;
    if (!c) {
        return std::make_unique<ConsCellObject>();
    }
    const Function *f = c.car->resolveFunction();
    if (f) {
        assert(dynamic_cast<const NamedObject *>(c.car.get()));
        auto &m = *dynamic_cast<const NamedObject *>(c.car.get())->parent;
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

template <typename T>
struct OptCheck : std::false_type
{
    using BaseType = T;
};

template <typename T>
struct OptCheck<std::optional<T>> : std::true_type
{
    using BaseType = T;
};

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
    while (((*str) >= '0' && (*str) <= '9') || (*str) == '.' || (*str) == '-') {
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
inline std::optional<double> getValue(const Object &sym)
{
    auto s = dynamic_cast<const FloatObject*>(&sym);
    if (s) {
        return s->value;
    }
    return std::nullopt;
}

template <> inline std::optional<ConsCellObject> getValue(const Object &sym) {
    auto s = dynamic_cast<const ConsCellObject *>(&sym);
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

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
// we specialize for pointers to member function
{
    using result_type = ReturnType;
    using arg_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);
};

template <class F, std::size_t ... Is, class T>
auto lambda_to_func_impl(F f, std::index_sequence<Is...>, T) {
    return std::function<typename T::result_type(std::tuple_element_t<Is, typename T::arg_tuple>...)>(f);
}

template <class F>
auto lambda_to_func(F f) {
    using traits = function_traits<F>;
    return lambda_to_func_impl(f, std::make_index_sequence<traits::arity>{}, traits{});
}

template<int N, typename... Ts>
using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

template <size_t I, typename... Args>
inline typename std::enable_if<I == sizeof...(Args), size_t>::type countNonOpts()
{
    return 0;
}

template <size_t I, typename... Args>
inline typename std::enable_if<I < sizeof...(Args), size_t>::type countNonOpts()
{
    using ThisType = NthTypeOf<I, Args...>;
    if (OptCheck<ThisType>::value) {
        return 0;
    }
    return 1 + countNonOpts<I+1, Args...>();
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

    template <typename T>
    std::unique_ptr<Object> makeObject(T);

    template<> std::unique_ptr<Object> makeObject(std::int64_t i) { return makeInt(i); }
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
        // std::cout << name << " " << func->minArgs << " " << func->maxArgs << std::endl;
        getSymbol(name)->function = std::move(func);
    }

    std::unique_ptr<Object> makeTrue()
    {
        assert(m_syms.count("t"));
        auto r = std::make_unique<NamedObject>(this);
        r->name = "t";
        return r;
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

    std::string parseNextName(const char*& str)
    {
        std::string name;
        while (*str && isPartOfSymName(*str)) {
            name += *str;
            str++;
        }
        return name;
    }

    std::unique_ptr<Object> parseNamedObject(const char*& str, bool quoted)
    {
        std::string name = parseNextName(str);
        if (quoted) {
            return std::make_unique<SymbolObject>(getSymbol(std::move(name)), true);
        }
        else {
            auto obj = std::make_unique<NamedObject>(this);
            obj->name = std::move(name);
            return obj;
        }
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

    std::unique_ptr<Object> parseNext(const char *&expr)
    {
        bool quoted = false;
        // Skip whitespace until next symbol
        while (*expr) {
            const char c = *expr;
            const char n = *(expr+1);
            if (isWhiteSpace(c)) {
                expr++;
                continue;
            }
            if ((c >= '0' && c <= '9') ||
                (c =='-' && (n >= '0' && n <= '9'))) {
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
        return parse(expr)->eval();
    }

    Object* resolveVariable(NamedObject* sym)
    {
        if (m_syms.count(sym->name)) {
            return m_syms[sym->name]->variable.get();
        }
        return nullptr;
    }

    Function* resolveFunction(NamedObject* sym)
    {
        if (m_syms.count(sym->name)) {
            return m_syms[sym->name]->function.get();
        }
        return nullptr;
    }

    void makeFunc(const char *name, int minArgs, int maxArgs,
                  std::function<std::unique_ptr<Object>(FArgs &)> f)
    {
        auto func = std::make_unique<Function>();
        func->name = name;
        func->minArgs = minArgs;
        func->maxArgs = maxArgs;
        func->func = f;
        getSymbol(name)->function = std::move(func);
    }

    Machine()
    {
        setVariable("nil", makeNil());
        setVariable("t", std::make_unique<SymbolObject>(getSymbol("t"), false));
        
        defun("atom", [](std::any obj) {
            if (obj.type() != typeid(std::shared_ptr<ConsCell>)) return true;
            std::shared_ptr<ConsCell> cc = std::any_cast<std::shared_ptr<ConsCell>>(obj);
            return !(cc && !!(*cc));
        });
        defun("null", [](bool isNil) { return !isNil; });
        defun("car", [](ConsCellObject obj) {
            return obj.cc->car ? obj.cc->car->clone() : makeNil();
        });
        defun("cdr", [](ConsCellObject obj) {
            return obj.cc->cdr ? obj.cc->cdr->clone() : makeNil();
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
        makeFunc("quote", 1, 1, [](FArgs& args) {
            return args.cc->car ? args.cc->car->clone() : makeNil();
        });
        makeFunc("stringp", 1, 1, [this](FArgs& args) {
            return (args.get()->isString()) ? makeTrue() : makeNil();
        });
        makeFunc("numberp", 1, 1, [this](FArgs& args) {
            auto arg = args.get();
            return arg->isInt() || arg->isFloat() ? makeTrue() : makeNil();
        });
        makeFunc("make-symbol", 1, 1, [](FArgs& args) {
            const auto arg = args.get();
            if (!arg->isString()) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>();
            symbol->name = arg->value<std::string>();
            return std::make_unique<SymbolObject>(symbol, false);
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
            return std::make_unique<StringObject>(sym->sym->name);
        });
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
        makeFunc("progn", 0, 0xfffff, [](FArgs &args) {
            std::unique_ptr<Object> ret = makeNil();
            for (auto obj : args) {
                ret = std::move(obj);
            }
            return ret;
        });
        makeFunc("intern", 1, 1, [this](FArgs& args) {
            const auto arg = args.get();
            if (!arg->isString()) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            return std::make_unique<SymbolObject>(getSymbol(arg->value<std::string>()), false);
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
                r = std::make_unique<SymbolObject>(getSymbol(arg->value<std::string>()), false);
            }
            return r;
        });
        makeFunc("setq", 2, 2, [this](FArgs &args) {
            const NamedObject *name =
                dynamic_cast<NamedObject *>(args.cc->car.get());
            if (!name) {
                throw exceptions::WrongTypeArgument(args.cc->car->toString());
            }
            auto value = args.cc->next()->car->eval();
            name->parent->setVariable(name->name, value->clone());
            return value;
        });
        makeFunc("eq", 2, 2, [this](FArgs& args) {
            return args.get()->equals(*args.get()) ? makeTrue() : makeNil();
        });
        makeFunc("describe-variable", 1, 1, [this](FArgs& args) {
            const auto arg = args.get();
            std::string descr = "You did not specify a variable.";
            if (auto sym = dynamic_cast<SymbolObject*>(arg.get())) {
                assert(sym->sym);
                if (!sym->sym->variable) {
                    descr = arg->toString() + " is void as a variable.";
                }
                else {
                    descr = arg->toString() + "'s value is " + sym->sym->variable->toString();
                }
            } else if (auto list = dynamic_cast<ConsCellObject *>(arg.get())) {
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
    }

    template<typename F>
    void defun(const char* name, F&& f)
    {
        makeFuncInternal(name, lambda_to_func(f));
    }

    void setVariable(std::string name, std::unique_ptr<Object> obj)
    {
        assert(obj);
        auto s = getSymbol(name);
        s->variable = std::move(obj);
    }

    void setMessageHandler(std::function<void(std::string)> handler)
    {
        m_msgHandler = handler;
    }
};

std::unique_ptr<Object> NamedObject::eval() 
{
    auto var = parent->resolveVariable(this);
    if (!var) {
        throw exceptions::VoidVariable(toString());
    }
    return var->clone();
}

Function* NamedObject::resolveFunction()
{
    return parent->resolveFunction(this);
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


}
