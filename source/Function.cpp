#include "ConsCellObject.hpp"
#include "Error.hpp"
#include "FunctionObject.hpp"
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
            throw exceptions::Error(*closure.parent,
                                    "Malformed arglist: " + closure.cc->car->toString());
        }
        if (sym->name == OptionalName) {
            if (opt) {
                throw exceptions::Error(*closure.parent,
                                        "Malformed arglist: " + closure.cc->car->toString());
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
    const auto fp = getFuncParams(closure);
    const auto& argList = fp.names;
    size_t i = 0;
    for (size_t i = 0; i < argList.size(); i++) {
        if (!a.hasNext()) {
            pushLocalVariable(argList[i], a.m.makeNil());
        }
        else {
            pushLocalVariable(argList[i], a.pop()->clone());
        }
    }
    AtScopeExit onExit([this, argList, &closure]() {
        for (auto it = argList.rbegin(); it != argList.rend(); ++it) {
            popLocalVariable(*it);
        }
    });
    std::unique_ptr<Object> ret = makeNil();
    for (auto& obj : *closure.cdr()->asList()) {
        ret = obj.eval();
    }
    return ret;
}

void Machine::initFunctionFunctions()
{
    defun("funcall", [](const Object& obj, FArgs& args) {
        auto func = obj.resolveFunction();
        if (!func) {
            throw exceptions::Error(args.m, "Invalid function " + obj.toString());
        }
        return func->func(args);
    });
    makeFunc("defun", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
        const SymbolObject* nameSym = dynamic_cast<SymbolObject*>(args.cc->car.get());
        if (!nameSym || nameSym->name.empty()) {
            throw exceptions::WrongTypeArgument(args.cc->car->toString());
        }
        std::string funcName = nameSym->name;
        args.skip();
        std::shared_ptr<ConsCellObject> closure =
            makeConsCell(args.cc->car->clone(), args.cc->cdr->clone());
        const FuncParams fp = getFuncParams(*closure);
        makeFunc(funcName.c_str(), fp.min, fp.max, [this, closure](FArgs& a) {
            return execute(*closure, a);
        })->closure = closure;
        return makeSymbol(funcName, false);
    });
    defun("functionp", [](const Object& obj) {
        auto func = obj.resolveFunction();
        return func && !func->isMacro;
    });
    defun("func-arity", [&](const Object& obj) {
        auto func = obj.resolveFunction();
        if (!func) {
            if (!obj.isSymbol()) {
                throw exceptions::Error(*this, "invalid function " + obj.toString());
            }
            throw exceptions::Error(*this, "void-function " + obj.toString());
            
        }
        return makeConsCell(makeInt(func->minArgs), makeInt(func->maxArgs));
    });
    defun("symbol-function", [&](const Symbol& sym) -> ObjectPtr {
        if (!sym.function) {
            return sym.parent->makeNil();
        }
        if (!sym.function->closure) {
            return std::make_unique<FunctionObject>(sym.function);
        }
        else {
            return makeConsCell(makeSymbol("lambda", true), sym.function->closure->clone());
        }
        return makeNil();
    });
}

}
