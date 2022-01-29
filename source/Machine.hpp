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
    
    std::unique_ptr<StringObject> parseString(const char *&str);

    std::unique_ptr<Object> quote(std::unique_ptr<Object> obj)
    {
        std::unique_ptr<ConsCellObject> list = std::make_unique<ConsCellObject>();
        list->cc->car = std::make_unique<SymbolObject>(this, nullptr, "quote");
        std::unique_ptr<ConsCellObject> cdr = std::make_unique<ConsCellObject>();
        cdr->cc->car = std::move(obj);
        list->cc->cdr = std::move(cdr);
        return list;
    }
    
    std::unique_ptr<Object> parseNext(const char *&expr);

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

    Machine(bool initStandardLibrary = true);

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

}
