#pragma once
#include "Object.hpp"
#include "ConsCell.hpp"
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
    bool isMacro = false;
    std::function<std::unique_ptr<Object>(FArgs&)> func;
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

template<typename T>
inline typename std::enable_if<std::is_reference_v<T> , T>::type getFuncParam(FArgs& args)
{
    Object* arg = args.get();
    try {
        return arg->value<T>();
    }
    catch (std::runtime_error& ex) {
        std::cout << "err: " << ex.what() << std::endl;
        throw exceptions::WrongTypeArgument(arg->toString());
    }
}

template<typename T>
inline typename std::enable_if<!OptCheck<T>::value && !std::is_reference_v<T> , T>::type getFuncParam(FArgs& args)
{
    Object* arg = args.get();
    std::optional<typename OptCheck<T>::BaseType> opt = arg->valueOrNull<T>();
    if (!opt) {
        throw exceptions::WrongTypeArgument(arg->toString());
    }
    return std::move(*opt);
}

template<typename T>
inline typename std::enable_if<OptCheck<T>::value, T>::type getFuncParam(FArgs& args)
{
    std::optional<typename OptCheck<T>::BaseType> opt;
    Object* arg;
    bool conversionFailed = false;
    if (args.hasNext()) {
        arg = args.get();
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
    
inline Object* FArgs::get()
{
    if (!cc) {
        return nullptr;
    }
    auto self = cc->car->trySelfEvaluate();
    if (self) {
        cc = cc->next();
        return self;
    }
    argStorage.push_back(cc->car->eval());
    cc = cc->next();
    return argStorage.back().get();
}

inline std::unique_ptr<Object> FArgs::Iterator::operator*() { return cc->car->eval(); }

}
