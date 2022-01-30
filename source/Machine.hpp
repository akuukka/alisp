#pragma once
#include "alisp.hpp"

namespace alisp {

struct StringObject;

class Machine
{
    std::map<std::string, std::shared_ptr<Symbol>> m_syms;
    std::function<void(std::string)> m_msgHandler;

    std::map<std::string, std::vector<std::unique_ptr<Object>>> m_locals;

    void pushLocalVariable(std::string name, std::unique_ptr<Object> obj)
    {
        m_locals[name].push_back(std::move(obj));
    }

    bool popLocalVariable(std::string name)
    {
        assert(m_locals[name].size());
        m_locals[name].pop_back();
        if (m_locals[name].empty()) {
            m_locals.erase(name);
        }
        return true;
    }

    template <typename T>
    std::unique_ptr<Object> makeObject(T);

    template<> std::unique_ptr<Object> makeObject(bool value);
    template<> std::unique_ptr<Object> makeObject(std::unique_ptr<Object> o)
    {
        return std::move(o);
    }
    template<> std::unique_ptr<Object> makeObject(std::shared_ptr<Object> o)
    {
        return std::unique_ptr<Object>(o->clone());
    }

    template <typename... Args>
    inline size_t getMinArgs()
    {
        return countNonOpts<0, Args...>();
    }
    
    template<typename R, typename ...Args>
    typename std::enable_if<sizeof...(Args) == 1, void>::type
    defunInternal(const char* name, std::function<R(Args...)> f)
    {
        using T = typename std::tuple_element<0, std::tuple<Args...>>::type;
        std::function<std::unique_ptr<Object>(FArgs&)> w = [=](FArgs& args) {
            return makeObject<R>(f(getFuncParam<T>(args)));
        };
        makeFunc(name, getMinArgs<Args...>(), sizeof...(Args), w);
    }

    template<typename R, typename ...Args>
    typename std::enable_if<sizeof...(Args) != 1, void>::type
    defunInternal(const char* name, std::function<R(Args...)> f)
    {
        makeFunc(name, getMinArgs<Args...>(), sizeof...(Args), [=](FArgs& args) {
            std::tuple<Args...> t = toTuple<Args...>(args);
            return makeObject<R>(std::apply(f, std::move(t)));
        });
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
