#pragma once
#include <map>
#include <type_traits>
#include "Object.hpp"
#include "alisp.hpp"
#include "Symbol.hpp"
#include "ConsCellObject.hpp"
#include "FArgs.hpp"

namespace alisp {

struct Closure;
struct StringObject;

class Machine
{
    friend void initFunctionFunctions(Machine& m);
    
    std::map<std::string, std::shared_ptr<Symbol>> m_syms;
    std::function<void(std::string)> m_msgHandler;

    std::map<std::string, std::vector<std::shared_ptr<Symbol>>> m_locals;

    void pushLocalVariable(std::string name, std::unique_ptr<Object> obj);
    bool popLocalVariable(std::string name);
    
    std::unique_ptr<Object> makeObject(std::string str);
    std::unique_ptr<Object> makeObject(const char* value);
    std::unique_ptr<Object> makeObject(std::shared_ptr<Symbol> sym);
    std::unique_ptr<Object> makeObject(std::unique_ptr<Object>);
    std::unique_ptr<Object> makeObject(std::int64_t i);
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
    typename std::enable_if<!std::is_same_v<R, void>, std::function<ObjectPtr(FArgs&)> >::type
    genCaller(std::function<R(Args...)> f,
                   std::index_sequence<Is...>)
    {
        return [=](FArgs& args) {
            return makeObject(f((getFuncParam<typename std::tuple_element<Is, std::tuple<Args...>>::type>(args))...));
        };
    }

    template<typename R, typename... Args, std::size_t... Is>
    typename std::enable_if<std::is_same_v<R, void>, std::function<ObjectPtr(FArgs&)> >::type
    genCaller(std::function<R(Args...)> f,
              std::index_sequence<Is...>)
    {
        return [=](FArgs& args) {
            f((getFuncParam<typename std::tuple_element<Is, std::tuple<Args...>>::type>(args))...);
            return makeNil();
        };
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
    std::unique_ptr<Object> quote(std::unique_ptr<Object> obj);    
    std::unique_ptr<Object> parseNext(const char *&expr);
    std::unique_ptr<Object> getNumericConstant(const std::string& str) const;
    void renameSymbols(ConsCellObject& obj, std::map<std::string, std::unique_ptr<Object>>& conv);
    ObjectPtr execute(Closure& closure, FArgs& a);
public:
    std::unique_ptr<Object> makeNil();
    std::unique_ptr<ConsCellObject> makeConsCell(ObjectPtr car, ObjectPtr cdr);

    
    std::unique_ptr<Object> parse(const char *expr);
    std::unique_ptr<Object> evaluate(const char *expr) { return parse(expr)->eval();  }
    Object* resolveVariable(const std::string& name);
    Function* resolveFunction(const std::string& name);

    void makeFunc(const char *name, int minArgs, int maxArgs,
                  const std::function<std::unique_ptr<Object>(FArgs &)>& f);

    Machine(bool initStandardLibrary = true);

    template<typename F>
    void defun(const char* name, F&& f)
    {
        defunInternal(name, lambda_to_func(f));
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
    
    std::shared_ptr<Symbol> getSymbolOrNull(std::string name);
    std::shared_ptr<Symbol> getSymbol(std::string name);
    std::unique_ptr<Object> makeTrue();
};

}
