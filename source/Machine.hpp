#pragma once
#include <map>
#include "alisp.hpp"
#include "Symbol.hpp"
#include "ConsCellObject.hpp"
#include "FArgs.hpp"

namespace alisp {

struct StringObject;

class Machine
{
    std::map<std::string, std::shared_ptr<Symbol>> m_syms;
    std::function<void(std::string)> m_msgHandler;

    std::map<std::string, std::vector<std::shared_ptr<Symbol>>> m_locals;

    void pushLocalVariable(std::string name, std::unique_ptr<Object> obj);
    bool popLocalVariable(std::string name);

    template <typename T>
    std::unique_ptr<Object> makeObject(T);

    template <typename... Args>
    inline size_t getMinArgs()
    {
        return countNonOpts<0, Args...>();
    }

    template<typename R, typename... Args, std::size_t... Is>
    auto genCaller(std::function<R(Args...)> f,
                   std::index_sequence<Is...>)
    {
        return [=](FArgs& args) {
            return makeObject<R>(f((getFuncParam<typename std::tuple_element<Is, std::tuple<Args...>>::type>(args))...));
        };
    }
    
    template<typename R, typename ...Args>
    void defunInternal(const char* name, std::function<R(Args...)> f)
    {
        makeFunc(name, getMinArgs<Args...>(), sizeof...(Args),
                 genCaller(f, std::index_sequence_for<Args...>{}));
    }

    std::string parseNextName(const char*& str);
    std::unique_ptr<Object> parseNamedObject(const char*& str);    
    std::unique_ptr<StringObject> parseString(const char *&str);
    std::unique_ptr<Object> quote(std::unique_ptr<Object> obj);    
    std::unique_ptr<Object> parseNext(const char *&expr);
    std::unique_ptr<Object> getNumericConstant(const std::string& str) const;
    void renameSymbols(ConsCellObject& obj, std::map<std::string, std::unique_ptr<Object>>& conv);
public:
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
