#pragma once
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
#include "Object.hpp"
#include "SharedDataObject.hpp"
#include "AtScopeExit.hpp"
#include "Init.hpp"
#include "Template.hpp"
#include "Exception.hpp"

namespace alisp {

class Machine;

struct Sequence
{
    virtual size_t length() const = 0;
};

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

struct ConsCellObject : SharedDataObject, Sequence
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

    ConsCellObject(const ConsCellObject& o) : cc(o.cc) {}

    ~ConsCellObject()
    {
        if (cc) tryDestroySharedData();
    }        

    void reset() override { cc.reset(); }
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

    void traverse(const std::function<bool(const Object&)>& f) const override;

    const void* sharedDataPointer() const override { return cc.get(); }
    size_t sharedDataRefCount() const override { return cc.use_count(); }
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

struct SymbolObject : SharedDataObject
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

    ~SymbolObject()
    {
        if (sym) tryDestroySharedData();
    }        

    const std::string getSymbolName()
    {
        return sym ? sym->name : name;
    };

    Symbol* getSymbol() const;
    Symbol* getSymbolOrNull() const;

    Function* resolveFunction() override;

    void reset() override { sym.reset(); }

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

    const void* sharedDataPointer() const override { return sym.get(); }
    size_t sharedDataRefCount() const override { return sym.use_count(); }
    void traverse(const std::function<bool(const Object&)>& f) const override
    {
        if (f(*this) && sym && sym->variable) {
            sym->variable->traverse(f);
        }
    }
};

// Remember nil = ()
inline std::unique_ptr<Object> makeNil() { return std::make_unique<ConsCellObject>(); }

inline std::unique_ptr<ConsCellObject> makeList() {
    return std::make_unique<ConsCellObject>();
}

inline std::unique_ptr<IntObject> makeInt(std::int64_t value) {
    return std::make_unique<IntObject>(value);
}

inline std::unique_ptr<FloatObject> makeFloat(double value) {
    return std::make_unique<FloatObject>(value);
}

inline int countArgs(const ConsCell* cc)
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
    std::vector<std::unique_ptr<Object>> argStorage;
    
    FArgs(ConsCell& cc, Machine& m) : cc(&cc), m(m) {}

    Object* get();
    
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
        Object* arg;
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

inline bool isPartOfSymName(const char c)
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

inline bool isWhiteSpace(const char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline bool onlyWhitespace(const char* expr)
{
    while (*expr) {
        if (!isWhiteSpace(*expr)) {
            return false;
        }
        expr++;
    }
    return true;
}

inline void skipWhitespace(const char*& expr)
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
    if (sym.isNil()) {
        return nullptr;
    }
    auto s = dynamic_cast<const SymbolObject*>(&sym);
    if (s) {
        return s->getSymbol();
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

inline Object* FArgs::get()
{
    if (!cc) {
        return nullptr;
    }
    argStorage.push_back(cc->car->eval());
    cc = cc->next();
    return argStorage.back().get();
}

inline std::unique_ptr<Object> FArgs::Iterator::operator*() { return cc->car->eval(); }

}
