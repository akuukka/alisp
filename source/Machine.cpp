#include "alisp.hpp"
#include "Machine.hpp"
#include "StringObject.hpp"
#include "ValueObject.hpp"

namespace alisp {

template<>
ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(std::string str)
{
    return std::make_unique<StringObject>(str);
}

template<>
ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(std::int64_t i)
{
    return makeInt(i);
}

template<>
ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(int i)
{
    return makeInt(i);
}

template<>
ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(size_t i)
{
    return makeInt((std::int64_t)i);
}

ALISP_INLINE std::unique_ptr<Object> Machine::parseNext(const char *&expr)
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


ALISP_INLINE Machine::Machine(bool initStandardLibrary)
{
    if (!initStandardLibrary) {
        return;
    }
    setVariable("nil", makeNil(), true);
    setVariable("t", std::make_unique<SymbolObject>(this, nullptr, "t"), true);
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
                 [this, macroName, argList, code](FArgs& a) {
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
        getSymbol(macroName)->function->isMacro = true;
        return std::make_unique<SymbolObject>(this, nullptr, std::move(macroName));
    });
    makeFunc("if", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
        if (!!*args.get()) {
            return args.get()->clone();
        }
        args.skip();
        while (auto res = args.get()) {
            if (!args.hasNext()) {
                return res->clone();
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
                         pushLocalVariable(argList[i], a.get()->clone());
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
        return dynamic_cast<SymbolObject*>(args.get()) ? makeTrue() : makeNil();
    });
    makeFunc("symbol-name", 1, 1, [](FArgs& args) {
        const auto obj = args.get();
        const auto sym = dynamic_cast<SymbolObject*>(obj);
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
        auto strSym = dynamic_cast<StringObject*>(arg);
        std::string str = *strSym->value;
        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] == '%') {
                if (str[i+1] == '%') {
                    str.erase(i, 1);
                }
                else if (str[i+1] == 's') {
                    const auto nextSym = args.get();
                    std::string stringVal;
                    if (nextSym->isString()) {
                        stringVal = nextSym->value<std::string>();
                    }
                    else {
                        throw exceptions::Error("Format specifier doesn’t match argument type");
                    }
                    str = str.substr(0, i) + stringVal + str.substr(i+2);
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
        const auto sym = dynamic_cast<SymbolObject*>(arg);
        if (!sym) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        const bool uninterned = m_syms.erase(sym->getSymbolName());
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
        const SymbolObject* name = p1->isNil() ? &nil : dynamic_cast<SymbolObject*>(p1);
        if (!name) {
            throw exceptions::WrongTypeArgument(p1->toString());
        }
        auto sym = name->getSymbol();
        assert(sym);
        if (sym->constant) {
            throw exceptions::Error("setting-constant " + name->toString());
        }
        sym->variable = args.get()->clone();
        return sym->variable->clone();
    });
    makeFunc("eq", 2, 2, [this](FArgs& args) {
        return args.get()->equals(*args.get()) ? makeTrue() : makeNil();
    });
    makeFunc("describe-variable", 1, 1, [this](FArgs& args) {
        const auto arg = args.get();
        std::string descr = "You did not specify a variable.";
        if (auto sym = dynamic_cast<SymbolObject*>(arg)) {
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
        else if (auto list = dynamic_cast<ConsCellObject *>(arg)) {
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
    defun("mapatoms", [this](const Symbol* sym) {
        if (!sym || !sym->function) {
            throw exceptions::VoidFunction(sym ? sym->name : "nil");
        }
        auto list = makeList();
        for (const auto& p : m_syms) {
            list->cc->car = quote(std::make_unique<SymbolObject>(this, p.second, ""));
            FArgs args(*list->cc, *this);
            (sym->function->func)(args);
        }
        return makeNil();
    });
    makeFunc("cons", 2, 2, [](FArgs &args) {
        return std::make_unique<ConsCellObject>(args.get()->clone(), args.get()->clone());
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
        ConsCellObject *list = dynamic_cast<ConsCellObject *>(arg);
        if (!list) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        auto ret = list->cc->car->clone();
        return ret;
    });
    evaluate(getInitCode());
}

ALISP_INLINE std::unique_ptr<StringObject> Machine::parseString(const char *&str)
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

ALISP_INLINE
std::unique_ptr<Object> Machine::quote(std::unique_ptr<Object> obj)
{
    std::unique_ptr<ConsCellObject> list = std::make_unique<ConsCellObject>();
    list->cc->car = std::make_unique<SymbolObject>(this, nullptr, "quote");
    std::unique_ptr<ConsCellObject> cdr = std::make_unique<ConsCellObject>();
    cdr->cc->car = std::move(obj);
    list->cc->cdr = std::move(cdr);
    return list;
}

ALISP_INLINE
std::unique_ptr<Object> Machine::getNumericConstant(const std::string& str) const
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

}