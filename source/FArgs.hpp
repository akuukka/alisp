#pragma once
#include "Error.hpp"
#include "Object.hpp"
#include "ConsCell.hpp"
#include "Template.hpp"
#include <type_traits>
#include <vector>

namespace alisp {

class Machine;

struct FArgs;
struct ConsCellObject;

struct Function
{
    std::string name;
    int minArgs = 0;
    int maxArgs = 0xffff;
    std::function<std::unique_ptr<Object>(FArgs&)> func;
    std::shared_ptr<ConsCellObject> closure;
    bool isMacro = false;
};

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
    std::string funcName = ""; // For debugging until proper stack traces will be available
    std::vector<std::unique_ptr<Object>> argStorage;
    std::vector<std::shared_ptr<Function>> funcStorage;
    
    FArgs(ConsCell& cc, Machine& m) : cc(&cc), m(m) {}

    Object* current() { return cc->car.get(); }
    Object* pop(bool eval = true);
    
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

    ObjectPtr evalAll(ConsCell* begin = nullptr)
    {
        auto code = begin ? begin : cc;
        while (code->next()) {
            code->car->eval();
            code = code->next();
        }
        return code->car->eval();
    }
};

template<typename T>
inline T getFuncParam(FArgs& args)
{
    if constexpr (std::is_same_v<T, const Function&>) {
        Object* arg = args.pop();
        auto func = arg->resolveFunction();
        assert(func && "Should have thrown...");
        args.funcStorage.push_back(func);
        return *func;
    }
    else if constexpr (std::is_same_v<T, FArgs&>) {
        return args;
    }
    else if constexpr (IsInstantiationOf<std::vector, T>::value) {
        T v;
        using ValueType = typename T::value_type;
        while (args.hasNext()) {
            v.push_back(getFuncParam<ValueType>(args));
        }
        return v;
    }
    else if constexpr (OptCheck<T>::value) {
        std::optional<typename OptCheck<T>::BaseType> opt;
        Object* arg;
        bool conversionFailed = false;
        if (args.hasNext()) {
            arg = args.pop();
            opt = arg->valueOrNull<typename OptCheck<T>::BaseType>();
            if (!opt) {
                if (arg->isNil()) {
                    // nil => std::nullopt makes sense
                }
                else {
                    conversionFailed = true;
                }
            }
        }
        if (conversionFailed) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        return opt;
    }
    else {
        Object* arg = args.pop();
        try {
            return arg->value<T>();
        }
        catch (std::runtime_error& ex) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
    }
}
    
inline Object* FArgs::pop(bool eval)
{
    if (!cc) {
        return nullptr;
    }
    auto self = cc->car->trySelfEvaluate();
    if (self) {
        cc = cc->next();
        return self;
    }
    if (!eval) {
        auto ptr = cc->car.get();
        cc = cc->next();
        return ptr;
    }
    argStorage.push_back(cc->car->eval());
    cc = cc->next();
    return argStorage.back().get();
}

inline std::unique_ptr<Object> FArgs::Iterator::operator*() { return cc->car->eval(); }

using Rest = FArgs;

}
