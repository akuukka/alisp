#pragma once
#include <map>
#include <type_traits>
#include "Error.hpp"
#include "Object.hpp"
#include "Template.hpp"
#include "alisp.hpp"
#include "Symbol.hpp"
#include "FArgs.hpp"
#include "String.hpp"

namespace alisp {

struct Closure;
struct ConsCellObject;
struct StringObject;
struct Number;

template<typename T, typename O>
void requireType(const O& obj)
{
    if (!dynamic_cast<const T*>(&obj)) {
        throw exceptions::WrongTypeArgument(obj.toString());
    }        
}

class Machine
{
    std::map<std::string, std::shared_ptr<Symbol>> m_syms;
    std::map<std::string, std::vector<std::shared_ptr<Symbol>>> m_locals;

    void pushLocalVariable(std::string name, std::unique_ptr<Object> obj);
    bool popLocalVariable(std::string name);
    
    std::unique_ptr<Object> makeObject(Number num);
    std::unique_ptr<Object> makeObject(std::string str);
    std::unique_ptr<Object> makeObject(String str);
    std::unique_ptr<Object> makeObject(const char* value);
    std::unique_ptr<Object> makeObject(std::shared_ptr<Symbol> sym);
    std::unique_ptr<Object> makeObject(std::unique_ptr<Object>);
    std::unique_ptr<Object> makeObject(std::int64_t i);
    std::unique_ptr<Object> makeObject(std::uint32_t i);
    std::unique_ptr<Object> makeObject(int i);
    std::unique_ptr<Object> makeObject(size_t i);
    std::unique_ptr<Object> makeObject(bool);
    std::unique_ptr<Object> makeObject(const Object&);

    template <typename... Args>
    inline size_t getMinArgs()
    {
        return countNonOpts<0, Args...>();
    }

    template <typename... Args>
    inline size_t getMaxArgs()
    {
        return countMaxArgs<0, Args...>();
    }

    template<typename R, typename... Args, std::size_t... Is>
    std::function<ObjectPtr(FArgs&)> genCaller(std::function<R(Args...)> f,
                                               std::index_sequence<Is...>)
    {
        if constexpr (std::is_same_v<R, void>) {
            return [=](FArgs& args) {
                f((getFuncParam<typename std::tuple_element_t<Is, std::tuple<Args...>>>(args))...);
                return makeNil();
            };
        }
        else {
            return [=](FArgs& args) {
                return makeObject(f((getFuncParam<std::tuple_element_t<Is, std::tuple<Args...>>>(args))...));
            };
        }
    }
    
    template<typename R, typename ...Args>
    void defunInternal(const char* name, std::function<R(Args...)> f)
    {
        makeFunc(name, getMinArgs<Args...>(), getMaxArgs<Args...>(),
                 genCaller(f, std::index_sequence_for<Args...>{}));
    }

    std::string parseNextName(const char*& str);
    std::unique_ptr<Object> parseNamedObject(const char*& str);    
    std::unique_ptr<StringObject> parseString(const char *&str);
    std::unique_ptr<Object> parseNext(const char *&expr);
    std::unique_ptr<Object> getNumericConstant(const std::string& str) const;

    void initErrorFunctions();
    void initListFunctions();
    void initFunctionFunctions();
    void initStringFunctions();
    void initSymbolFunctions();
    void initSequenceFunctions();
public:
    std::unique_ptr<Object> makeNil();
    std::unique_ptr<ConsCellObject> makeConsCell(ObjectPtr car, ObjectPtr cdr = nullptr);
    std::unique_ptr<SymbolObject> makeSymbol(std::string name, bool parsedName);
    std::unique_ptr<Object> quote(std::unique_ptr<Object> obj, const char* quoteFunc = "quote");
    std::string parsedSymbolName(std::string name);
    
    std::unique_ptr<Object> parse(const char *expr);
    std::unique_ptr<Object> evaluate(const char *expr);
    ObjectPtr execute(const ConsCellObject& lambda, FArgs& a);

    Function* makeFunc(std::string name, int minArgs, int maxArgs,
                       const std::function<std::unique_ptr<Object>(FArgs &)>& f);

    Machine(bool initStandardLibrary = true);

    template<typename F>
    void defun(const char* name, F&& f)
    {
        defunInternal(name, lambda_to_func(f));
    }

    void setVariable(std::string name, std::unique_ptr<Object> obj, bool constant = false);
    std::shared_ptr<Symbol> getSymbolOrNull(std::string name, bool alwaysGlobal = false);
    std::shared_ptr<Symbol> getSymbol(std::string name, bool alwaysGlobal = false);
    std::unique_ptr<Object> makeTrue();

    struct SymbolRef
    {
        std::shared_ptr<Symbol> symbol;

        template <typename T>
        void operator=(T t)
        {
            if constexpr (IsCallable<T>::value) {
                symbol->parent->defun(symbol->name.c_str(), t);
            }
            else {
                symbol->variable = symbol->parent->makeObject(t);
            }
        }
    };
    SymbolRef operator[](const char* name);
};

}
