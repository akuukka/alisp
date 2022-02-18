#include "ConsCellObject.hpp"
#include "Error.hpp"
#include "Template.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"
#include "AtScopeExit.hpp"
#include <memory>
#include "Function.hpp"

namespace alisp
{

ALISP_INLINE FuncParams getFuncParams(const ConsCellObject& closure)
{
    FuncParams fp;
    std::vector<std::string>& argList = fp.names;
    bool opt = false;
    fp.rest = false;
    for (auto& arg : *closure.cc->car->asList()) {
        const auto sym = dynamic_cast<const SymbolObject*>(&arg);
        if (!sym) {
            throw exceptions::Error("Malformed arglist: " + closure.cc->car->toString());
        }
        if (sym->name == OptionalName) {
            if (opt) {
                throw exceptions::Error("Malformed arglist: " + closure.cc->car->toString());
            }
            opt = true;
            continue;
        }
        else if (sym->name == RestName) {
            fp.rest = true;
            fp.max = std::numeric_limits<int>::max();
            continue;
        }
        argList.push_back(sym->name);
        if (fp.rest) {
            break;
        }
        if (!opt) {
            fp.min++;
        }
        fp.max++;
    }
    return fp;
}

ObjectPtr Machine::execute(const ConsCellObject& closure, FArgs& a)
{
    ListBuilder builder(*this);
    const auto fp = getFuncParams(closure);
    const auto& argList = fp.names;
    size_t i = 0;
    for (size_t i = 0; i < argList.size(); i++) {
        if (!a.hasNext()) {
            pushLocalVariable(argList[i], a.m.makeNil());
        }
        else {
            if (fp.rest && i + 1 == argList.size()) {
                while (a.hasNext()) {
                    builder.append(a.pop()->clone());
                }
                pushLocalVariable(argList[i], builder.get());
            }
            else {
                pushLocalVariable(argList[i], a.pop()->clone());
            }
        }
    }
    AtScopeExit onExit([this, argList, &closure]() {
        for (auto it = argList.rbegin(); it != argList.rend(); ++it) {
            popLocalVariable(*it);
        }
    });
    std::unique_ptr<Object> ret = makeNil();
    assert(closure.cdr() && closure.cdr()->isList());
    for (auto& obj : *closure.cdr()->asList()) {
        ret = obj.eval();
    }
    return ret;
}

void Machine::initFunctionFunctions()
{
    defun("apply", [](const Object& obj, FArgs& args) {
        auto func = obj.resolveFunction();
        if (!func) {
            throw exceptions::Error("Invalid function " + obj.toString());
        }
        auto arg = args.pop();
        if (!arg || (!args.hasNext() && !arg->isList())) {
            throw exceptions::Error("wrong type argument");
        }
        if (!args.hasNext() && arg->isList()) {
            if (arg->isNil()) {
                return func->func(args);
            }
            args.cc = arg->asList()->cc.get();
            args.disableEval = true;
            return func->func(args);
        }
        ListBuilder builder(args.m);
        builder.append(arg->clone());
        while (args.hasNext()) {
            arg = args.pop();
            if (!args.hasNext()) {
                if (!arg->isList()) {
                    throw exceptions::Error("wrong type argument");
                }
                for (auto& obj : *arg->asList()) {
                    builder.append(obj.clone());
                }
            }
            else {
                builder.append(arg->clone());
            }
        }
        auto li = builder.get();
        args.cc = li->cc.get();
        args.disableEval = true;
        return func->func(args);
    });
    defun("funcall", [](const Object& obj, FArgs& args) {
        auto func = obj.resolveFunction();
        if (!func) {
            throw exceptions::Error("Invalid function " + obj.toString());
        }
        return func->func(args);
    });
    makeFunc("defun", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
        const SymbolObject* nameSym = dynamic_cast<SymbolObject*>(args.cc->car.get());
        if (!nameSym || nameSym->name.empty()) {
            throw exceptions::WrongTypeArgument(args.cc->car->toString());
        }
        std::string funcName = nameSym->name;
        ListBuilder builder(*this);
        builder.append(makeSymbol("lambda", true));
        args.skip();
        auto cc = args.cc;
        while (cc && cc->car) {
            builder.append(cc->car->clone());
            cc = cc->next();
        }
        getSymbol(funcName)->function = builder.get();
        return makeSymbol(funcName, false);
    });
    defun("functionp", [](const Object& obj) {
        try {
            auto func = obj.resolveFunction();
            return func && !func->isMacro;
        }
        catch (exceptions::InvalidFunction&) {
            return false;
        }
        catch (exceptions::VoidFunction&) {
            return false;
        }
    });
    defun("func-arity", [&](const Function& func) {
        return makeConsCell(makeInt(func.minArgs), makeInt(func.maxArgs));
    });
    defun("symbol-function", [&](const Symbol& sym) {
        if (!sym.function) {
            return sym.parent->makeNil();
        }
        return sym.function->clone();
    });
    defun("fset", [](Symbol& sym, const Object& definition) {
        sym.function = definition.isNil() ? nullptr : definition.clone();
        return definition.clone();
    });
    defun("fboundp", [this](const Object& obj) {
        requireType<SymbolObject>(obj);
        if (obj.asSymbol()->sym) {
            return obj.asSymbol()->sym->function != nullptr;
        }
        if (getSymbol(obj.asSymbol()->name)->function) {
            return true;
        }
        if (getSymbol(obj.asSymbol()->name, true)->function) {
            return true;
        }
        return false;
    });
}

}
