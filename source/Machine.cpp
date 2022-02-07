#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include "FunctionObject.hpp"
#include "Error.hpp"
#include "Object.hpp"
#include "Sequence.hpp"
#include "alisp.hpp"
#include "AtScopeExit.hpp"
#include "Machine.hpp"
#include "StringObject.hpp"
#include "ValueObject.hpp"
#include "ConsCellObject.hpp"
#include "SymbolObject.hpp"
#include "Init.hpp"
#include "UTF8.hpp"
#include "StreamObject.hpp"

namespace alisp {

void initMacroFunctions(Machine& m);
void initMathFunctions(Machine& m);
void initSequenceFunctions(Machine& m);

ALISP_INLINE Function* Machine::makeFunc(std::string name, int minArgs, int maxArgs,
                                         const std::function<std::unique_ptr<Object>(FArgs &)>& f)
{
    if (ConvertParsedNamesToUpperCase) {
        name = utf8::toUpper(name);
    }
    auto func = std::make_unique<Function>();
    func->name = name;
    func->minArgs = minArgs;
    func->maxArgs = maxArgs;
    func->func = std::move(f);
    auto sym = getSymbol(name);
    sym->function = std::move(func);
    return sym->function.get();
}

ALISP_INLINE std::shared_ptr<Symbol> Machine::getSymbolOrNull(std::string name)
{
    if (m_locals.count(name)) {
        return m_locals[name].back();
    }
    if (!m_syms.count(name)) {
        return nullptr;
    }
    return m_syms[name];
}

ALISP_INLINE std::shared_ptr<Symbol> Machine::getSymbol(std::string name)
{
    if (m_locals.count(name)) {
        return m_locals[name].back();
    }
    if (!m_syms.count(name)) {
        auto newSym = std::make_shared<Symbol>();
        newSym->parent = this;
        if (name.size() && name[0] == ':') {
            newSym->constant = true;
            newSym->variable = std::make_unique<SymbolObject>(this, nullptr, name);
        }
        m_syms[name] = newSym;
        newSym->name = std::move(name);
        return newSym;
    }
    return m_syms[name];
}

ALISP_INLINE bool isWhiteSpace(const char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

ALISP_INLINE bool onlyWhitespace(const char* expr)
{
    bool inComment = true;
    while (*expr) {
        if (*expr == ';') {
            inComment = true;
        }
        if (!inComment && !isWhiteSpace(*expr)) {
            return false;
        }
        if (inComment && *expr == '\n') {
            inComment = false;
        }
        expr++;
    }
    return true;
}

ALISP_INLINE void skipWhitespace(const char*& expr)
{
    while (*expr && (isWhiteSpace(*expr) || *expr == ';')) {
        if (*expr == ';') {
            while (*expr && *expr != '\n') {
                expr++;
            }
            if (!*expr) {
                return;
            }
        }
        expr++;
    }
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(String str)
{
    return std::make_unique<StringObject>(str);
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(std::string str)
{
    return std::make_unique<StringObject>(str);
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(const char* value)
{
    return std::make_unique<StringObject>(std::string(value));
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(std::shared_ptr<Symbol> sym)
{
    return std::make_unique<SymbolObject>(this, sym, "");
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(std::int64_t i)
{
    return makeInt(i);
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(std::uint32_t i)
{
    return makeInt(i);
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(int i)
{
    return makeInt(i);
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(size_t i)
{
    return makeInt((std::int64_t)i);
}

inline bool isPartOfSymName(const char c)
{
    if (c=='.') return true;
    if (c=='?') return true;
    if (c=='+') return true;
    if (c==':') return true;
    if (c=='%') return true;
    if (c=='*') return true;
    if (c=='&') return true;
    if (c=='=') return true;
    if (c=='<') return true;
    if (c=='>') return true;
    if (c=='/') return true;
    if (c=='-') return true;
    if (c>='a' && c<='z') return true;
    if (c>='A' && c<='Z') return true;
    if (c>='0' && c<='9') return true;
    return false;
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
        if (c == ';') {
            expr++;
            while (*expr && *expr != '\n') {
                expr++;
            }
            if (*expr && *expr == '\n') {
                expr++;
            }
            continue;
        }
        if (c == '\"') {
            return parseString(++expr);
        }
        else if (c == '\'') {
            expr++;
            return quote(parseNext(expr));
        }
        else if (c == '#' && n == '\'') {
            expr+=2;
            return quote(parseNext(expr), "function");
        }
        else if (isPartOfSymName(c))
        {
            return parseNamedObject(expr);
        }
        else if (c == '(') {
            auto l = makeList(this);
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
                        lastConsCell->cdr = std::make_unique<ConsCellObject>(this);
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
    setVariable(parsedSymbolName("&optional"),
                std::make_unique<SymbolObject>(this, nullptr, parsedSymbolName("&optional")), true);
    setVariable(NilName, makeNil(), true);
    setVariable(TName,
                std::make_unique<SymbolObject>(this, nullptr, TName), true);
    setVariable(parsedSymbolName("*standard-output*"),
                std::make_unique<OStreamObject>(&std::cout));
    setVariable(parsedSymbolName("*standard-input*"),
                std::make_unique<IStreamObject>(&std::cin));
    setVariable(parsedSymbolName("*query-io*"),
                std::make_unique<IOStreamObject>(&std::cin, &std::cout));
    setVariable(parsedSymbolName("most-positive-fixnum"),
                makeInt(std::numeric_limits<std::int64_t>::max()));
    setVariable(parsedSymbolName("most-negative-fixnum"),
                makeInt(std::numeric_limits<std::int64_t>::min()));
    initFunctionFunctions();
    initErrorFunctions();
    initMathFunctions(*this);
    initMacroFunctions(*this);
    initSequenceFunctions(*this);
    initStringFunctions();
    initSymbolFunctions();
    defun("append", [this](const ConsCell& a, const ConsCell& b) {
        ListBuilder builder(*this);
        for (const auto& obj : a) { builder.append(obj); }
        for (const auto& obj : b) { builder.append(obj); }
        return builder.get();
    });
    defun("atom", [](const Object& obj) { return !obj.isList() || obj.isNil(); });
    defun("null", [](bool isNil) { return !isNil; });
    defun("not", [](bool value) { return !value; });
    defun("setcar", [](ConsCell& cc, ObjectPtr newcar) {
        return cc.car = newcar->clone(), std::move(newcar);
    });
    defun("setcdr", [](ConsCell& cc, ObjectPtr newcdr) {
        return cc.cdr = newcdr->clone(), std::move(newcdr);
    });
    defun("car", [this](const ConsCell& cc) { return cc.car ? cc.car->clone() : makeNil(); });
    defun("cdr", [this](const ConsCell& cc) { return cc.cdr ? cc.cdr->clone() : makeNil(); });
    defun("consp", [](const Object& obj) { return obj.isList() && !obj.isNil(); });
    defun("listp", [](const Object& obj) { return obj.isList(); });
    defun("nlistp", [](const Object& obj) { return !obj.isList(); });
    defun("proper-list-p", [this](ObjectPtr obj) {
        if (!obj->isList()) return makeNil();
        std::shared_ptr<ConsCell> cc = obj->asList()->cc;
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
    makeFunc("if", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
        if (!!*args.pop()) {
            return args.pop()->clone();
        }
        args.skip();
        while (auto res = args.pop()) {
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
            std::string name;
            ObjectPtr value;
            if (arg.isList()) {
                auto list = arg.asList();
                auto cc = list->cc.get();
                const auto sym = dynamic_cast<const SymbolObject*>(cc->car.get());
                assert(sym && sym->name.size());
                name = sym->name;
                value = cc->cdr->asList()->cc->car->eval();
            }
            else if (auto sym = dynamic_cast<const SymbolObject*>(&arg)) {
                assert(sym->name.size());
                name = sym->name;
                value = makeNil();
            }
            else {
                throw exceptions::WrongTypeArgument(arg.toString());
            }
            if (star) {
                pushLocalVariable(name, std::move(value));
                varList.push_back(name);
            }
            else {
                pushList.emplace_back(name, std::move(value));
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
    makeFunc("let", 2, std::numeric_limits<int>::max(),
             std::bind(let, std::placeholders::_1, false));
    makeFunc("let*", 2, std::numeric_limits<int>::max(),
             std::bind(let, std::placeholders::_1, true));
    defun("make-list", [this](std::int64_t n, ObjectPtr ptr) {
        std::unique_ptr<Object> r = makeNil();
        for (std::int64_t i=0; i < n; i++) {
            r = std::make_unique<ConsCellObject>(ptr->clone(), r->clone(), this);
        }
        return r;
    });
    makeFunc("quote", 1, 1, [this](FArgs& args) {
        return args.cc->car && !args.cc->car->isNil() ? args.cc->car->clone() : makeNil();
    });
    makeFunc("function", 1, 1, [this](FArgs& args) {
        return args.cc->car && !args.cc->car->isNil() ? args.cc->car->clone() : makeNil();
    });
    defun("numberp", [](ObjectPtr obj) { return obj->isInt() || obj->isFloat(); });
    makeFunc("symbolp", 1, 1, [this](FArgs& args) {
        return dynamic_cast<SymbolObject*>(args.pop()) ? makeTrue() : makeNil();
    });
    makeFunc("symbol-name", 1, 1, [](FArgs& args) {
        const auto obj = args.pop();
        const auto sym = dynamic_cast<SymbolObject*>(obj);
        if (!sym) {
            throw exceptions::WrongTypeArgument(obj->toString());
        }
        return std::make_unique<StringObject>(sym->getSymbolName());
    });
    makeFunc("eval", 1, 1, [](FArgs& args) { return args.pop()->eval(); });
    makeFunc("progn", 0, 0xfffff, [&](FArgs& args) {
        std::unique_ptr<Object> ret = makeNil();
        for (auto obj : args) {
            ret = std::move(obj);
        }
        return ret;
    });
    makeFunc("prog1", 0, 0xfffff, [&](FArgs& args) {
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
        const auto arg = args.pop();
        const auto sym = dynamic_cast<SymbolObject*>(arg);
        if (!sym) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        const bool uninterned = m_syms.erase(sym->getSymbolName());
        return uninterned ? makeTrue() : makeNil();
    });
    makeFunc("intern-soft", 1, 1, [this](FArgs& args) {
        const auto arg = args.pop();
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
        const SymbolObject nil(this, nullptr, parsedSymbolName("nil"));
        const auto& p1 = args.pop();
        const SymbolObject* name = p1->isNil() ? &nil : dynamic_cast<SymbolObject*>(p1);
        if (!name) {
            throw exceptions::WrongTypeArgument(p1->toString());
        }
        if (!name->sym && m_locals.count(name->name) && m_locals[name->name].size()) {
            auto& loc = m_locals[name->name].back()->variable;
            loc = args.pop()->clone();
            return loc->clone();
        }
        auto sym = name->getSymbol();
        assert(sym);
        if (sym->constant) {
            throw exceptions::Error("setting-constant " + name->toString());
        }
        sym->variable = args.pop()->clone();
        return sym->variable->clone();
    });
    makeFunc("defvar", 1, 3, [this](FArgs& args) {
        const SymbolObject nil(this, nullptr, parsedSymbolName("nil"));
        const auto& p1 = args.pop(false);
        const SymbolObject* name = p1->isNil() ? &nil : dynamic_cast<SymbolObject*>(p1);
        if (!name || name->name.empty()) {
            throw exceptions::WrongTypeArgument(p1->toString());
        }
        if (!m_syms.count(name->name)) {
            auto sym = getSymbol(name->name);
            if (args.hasNext()) {
                sym->variable = args.pop(true)->clone();
            }
            if (args.hasNext()) {
                auto docString = args.pop();
                if (docString->isString()) {
                    sym->description = docString->value<std::string>();
                }
            }
        }
        return std::make_unique<SymbolObject>(this, m_syms[name->name], "");
    });
    makeFunc("eq", 2, 2, [this](FArgs& args) {
        return args.pop()->equals(*args.pop()) ? makeTrue() : makeNil();
    });
    makeFunc("describe-variable", 1, 1, [this](FArgs& args) {
        const auto arg = args.pop();
        std::string descr = "You did not specify a variable.";
        if (auto symObj = dynamic_cast<SymbolObject*>(arg)) {
            auto sym = symObj->getSymbolOrNull();
            const Object* var = nullptr;
            if (sym) var = sym->variable.get();
            if (!var) {
                descr = arg->toString() + " is void as a variable.";
            }
            else {
                descr = arg->toString() + "'s value is " + var->toString();
            }
        }
        else if (auto list = dynamic_cast<ConsCellObject*>(arg)) {
            if (!*list) {
                descr = arg->toString() + "'s value is " + arg->toString();
            }
        }
        return std::make_unique<StringObject>(descr);
    });
    defun("nth", [&](std::int64_t index, const ConsCell& list) {
        auto p = &list;
        auto obj = list.car.get();
        for (size_t i = 0; i < index; i++) {
            p = p->next();
            if (!p) {
                return makeNil();
            }
            obj = p->car.get();
        }
        return obj->clone();
    });
    defun("nthcdr", [&](std::int64_t index, const Object& obj) {
        if (!obj.isList()) {
            throw exceptions::WrongTypeArgument(obj.toString());
        }
        const Object* p = obj.asList();
        for (size_t i = 0; i < index; i++) {
            if (!p || !p->asList()) {
                if (p && !p->asList()) {
                    throw exceptions::WrongTypeArgument(p->toString());
                }
                return makeNil();
            }
            p = p->asList()->cdr();
        }
        return p->clone();
    });
    defun("mapatoms", [this](const Symbol& sym) {
        if (!sym.function) {
            throw exceptions::VoidFunction(sym.name);
        }
        auto list = makeList(this);
        for (const auto& p : m_syms) {
            list->cc->car = quote(std::make_unique<SymbolObject>(this, p.second, ""));
            FArgs args(*list->cc, *this);
            (sym.function->func)(args);
        }
        return makeNil();
    });
    defun("and", [](Rest& args) -> ObjectPtr {
        ObjectPtr ret = args.m.makeTrue();
        while (args.cc) {
            ret = args.pop()->clone();
            if (ret->isNil()) {
                break;
            }
        }
        return ret;
    });
    makeFunc("cons", 2, 2, [](FArgs& args) {
        return std::make_unique<ConsCellObject>(args.pop()->clone(), args.pop()->clone(), &args.m);
    });
    makeFunc("list", 0, std::numeric_limits<int>::max(), [](FArgs& args) {
        auto newList = makeList(&args.m);
        ConsCell *lastCc = newList->cc.get();
        bool first = true;
        for (auto obj : args) {
            if (!first) {
                lastCc->cdr = std::make_unique<ConsCellObject>(&args.m);
                lastCc = lastCc->next();
            }
            lastCc->car = obj->clone();
            first = false;
        }
        return newList;
    });
    defun("boundp", [](const Symbol& sym) { return sym.variable ? true : false; });
    defun("makunbound", [](std::shared_ptr<Symbol> sym) {
        if (!sym || sym->constant) {
            throw exceptions::Error("setting-constant" + (sym ? sym->name : std::string("nil")));
        }
        sym->variable = nullptr;
        return sym;
    });
    defun("symbol-value", [this](std::shared_ptr<Symbol> sym) -> ObjectPtr {
        if (!sym) {
            return makeNil();
        }
        if (!sym->variable) {
            throw exceptions::VoidVariable(sym->name);
        }
        return sym->variable->clone();
    });
    makeFunc("while", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
        while (!args.cc->car->eval()->isNil()) { 
            args.evalAll(args.cc->next());
        }
        return makeNil();
    });
    makeFunc("dolist", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
        const auto p1 = args.pop(false)->asList();
        const std::string varName = p1->car()->asSymbol()->name;
        auto evaluated = p1->cdr()->asList()->car()->eval();
        auto codestart = args.cc;
        for (const auto& obj : *evaluated->asList()) {
            pushLocalVariable(varName, obj.clone());
            AtScopeExit onExit([this, varName](){ popLocalVariable(varName); });
            auto code = codestart;
            while (code) {
                code->car->eval();
                code = code->next();
            }
        }
        return makeNil();
    });
    evaluate(getInitCode());
}

ALISP_INLINE std::shared_ptr<Function> Machine::resolveFunction(const std::string& name)
{
    if (m_syms.count(name)) {
        return m_syms[name]->function;
    }
    return nullptr;
}

ALISP_INLINE std::string Machine::parsedSymbolName(std::string name)
{
    if (!ConvertParsedNamesToUpperCase) {
        return name;
    }
    return utf8::toUpper(name);
}

ALISP_INLINE std::unique_ptr<Object> Machine::parse(const char *expr)
{
    auto r = parseNext(expr);
    if (onlyWhitespace(expr)) {
        return r;
    }

    auto prog = makeList(this);
    prog->cc->car =
        std::make_unique<SymbolObject>(this, nullptr, parsedSymbolName("progn"));
    
    auto consCell = makeList(this);
    consCell->cc->car = std::move(r);
    auto lastCell = consCell->cc.get();
    
    while (!onlyWhitespace(expr)) {
        auto n = parseNext(expr);
        lastCell->cdr = std::make_unique<ConsCellObject>(std::move(n), nullptr, this);
        lastCell = lastCell->next();
    }
    prog->cc->cdr = std::move(consCell);
    return prog;
}

ALISP_INLINE std::string Machine::parseNextName(const char*& str)
{
    std::string name;
    while (*str && isPartOfSymName(*str)) {
        name += *str;
        str++;
    }
    return name;
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(bool value)
{
    return value ? makeTrue() : makeNil();
}

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(const Object& obj)
{
    return obj.clone();
}

ALISP_INLINE
std::unique_ptr<Object> Machine::makeNil() { return alisp::makeNil(this); }

ALISP_INLINE std::unique_ptr<Object> Machine::makeObject(std::unique_ptr<Object> o)
{
    return o;
}

inline std::pair<std::uint32_t, size_t> parseNextChar(const char* str)
{
    // https://www.gnu.org/software/emacs/manual/html_node/elisp/Basic-Char-Syntax.html
    std::pair<std::uint32_t, size_t> p;
    if (str[0] == '\\') {
        if (!str[1]) {
            throw exceptions::SyntaxError("EOF while parsing");
        }
        const char c = str[1];
        if (c == 'a') return std::make_pair(static_cast<std::uint32_t>(7), 2);
        if (c == 'b') return std::make_pair(static_cast<std::uint32_t>(8), 2);
        if (c == 't') return std::make_pair(static_cast<std::uint32_t>(9), 2);
        if (c == 'n') return std::make_pair(static_cast<std::uint32_t>(10), 2);
        if (c == 'v') return std::make_pair(static_cast<std::uint32_t>(11), 2);
        if (c == 'f') return std::make_pair(static_cast<std::uint32_t>(12), 2);
        if (c == 'r') return std::make_pair(static_cast<std::uint32_t>(13), 2);
        if (c == 'e') return std::make_pair(static_cast<std::uint32_t>(27), 2);
        if (c == 's') return std::make_pair(static_cast<std::uint32_t>(32), 2);
        if (c == '\\') return std::make_pair(static_cast<std::uint32_t>(92), 2);
        if (c == 'd') return std::make_pair(static_cast<std::uint32_t>(127), 2);
        str++;
    }
    p.second = utf8::next(str, &p.first);
    return p;
}

ALISP_INLINE std::unique_ptr<Object> Machine::parseNamedObject(const char*& str)
{
    if (str[0] == '?') {
        if (!str[1]) {
            throw exceptions::SyntaxError("EOF while parsing");
        }
        const std::pair<std::uint32_t, size_t> nchar = parseNextChar(str+1);
        if (nchar.second) {
            const std::uint32_t codepoint = utf8::decode(nchar.first);
            str += 1 + nchar.second;
            return std::make_unique<IntObject>(codepoint);
        }
        throw exceptions::Error("Invalid read syntax");
    }
    std::string next = parseNextName(str);
    auto num = getNumericConstant(next);
    if (num) {
        return num;
    }
    if (ConvertParsedNamesToUpperCase) {
        next = utf8::toUpper(next);
    }
    if (next == parsedSymbolName("nil")) {
        // It's optimal to return nil already at this point.
        // Note that even the GNU elisp manual says:
        // 'After the Lisp reader has read either `()' or `nil', there is no way to determine
        //  which representation was actually written by the programmer.'
        return makeNil();
    }
    return std::make_unique<SymbolObject>(this, nullptr, std::move(next));
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
std::unique_ptr<ConsCellObject> Machine::makeConsCell(ObjectPtr car, ObjectPtr cdr)
{
    return std::make_unique<ConsCellObject>(std::move(car), std::move(cdr), this);
}

ALISP_INLINE
std::unique_ptr<SymbolObject> Machine::makeSymbol(std::string name, bool parsedName)
{
    return std::make_unique<SymbolObject>(this,
                                          nullptr,
                                          parsedName ? parsedSymbolName(name) : name);
}

ALISP_INLINE
std::unique_ptr<Object> Machine::quote(std::unique_ptr<Object> obj, const char* quoteFunc)
{
    std::unique_ptr<ConsCellObject> list = std::make_unique<ConsCellObject>(this);
    list->cc->car = std::make_unique<SymbolObject>(this, nullptr, parsedSymbolName(quoteFunc));
    std::unique_ptr<ConsCellObject> cdr = std::make_unique<ConsCellObject>(this);
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

ALISP_INLINE std::unique_ptr<Object> Machine::makeTrue() 
{
    return std::make_unique<SymbolObject>(this, nullptr, TName);
}

ALISP_INLINE void Machine::pushLocalVariable(std::string name, ObjectPtr obj)
{
    auto sym = std::make_shared<Symbol>();
    m_locals[name].push_back(sym);
    sym->variable = std::move(obj);
    sym->local = true;
}

ALISP_INLINE bool Machine::popLocalVariable(std::string name)
{
    assert(m_locals[name].size());
    m_locals[name].pop_back();
    if (m_locals[name].empty()) {
        m_locals.erase(name);
    }
    return true;
}

ALISP_INLINE std::unique_ptr<Object> Machine::evaluate(const char *expr)
{
    auto obj = parse(expr);
    return obj ? obj->eval() : nullptr;
}

}
