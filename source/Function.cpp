#include "ConsCellObject.hpp"
#include "Exception.hpp"
#include "FunctionObject.hpp"
#include "Template.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"
#include "AtScopeExit.hpp"
#include <memory>

namespace alisp
{

struct FuncParams {
    int min = 0;
    int max = 0;
    std::vector<std::string> names;
};

inline FuncParams getFuncParams(const ConsCellObject& closure)
{
    FuncParams fp;
    std::vector<std::string>& argList = fp.names;
    for (auto& arg : *closure.cc->car->asList()) {
        const auto sym = dynamic_cast<const SymbolObject*>(&arg);
        if (!sym) {
            throw exceptions::Error("Malformed arglist: " + closure.cc->car->toString());
        }
        argList.push_back(sym->name);
        fp.min++;
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
        pushLocalVariable(argList[i], a.pop()->clone());
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

void initFunctionFunctions(Machine& m)
{
    m.makeFunc("lambda", 1, std::numeric_limits<int>::max(), [&m](FArgs& args) {
        auto func = std::make_shared<Function>();
        func->name = "anon";
        std::shared_ptr<ConsCellObject> closure =
            m.makeConsCell(args.cc->car->clone(), args.cc->cdr->clone());
        const FuncParams fp = getFuncParams(*closure);
        func->minArgs = fp.min;
        func->maxArgs = fp.max;
        func->func = [&m, closure](FArgs& a) { return m.execute(*closure, a); };
        func->closure = closure;
        return std::make_unique<FunctionObject>(func);
    });
    m.makeFunc("defun", 2, std::numeric_limits<int>::max(), [&m](FArgs& args) {
        const SymbolObject* nameSym = dynamic_cast<SymbolObject*>(args.cc->car.get());
        if (!nameSym || nameSym->name.empty()) {
            throw exceptions::WrongTypeArgument(args.cc->car->toString());
        }
        std::string funcName = nameSym->name;
        args.skip();
        std::shared_ptr<ConsCellObject> closure =
            m.makeConsCell(args.cc->car->clone(), args.cc->cdr->clone());
        const FuncParams fp = getFuncParams(*closure);
        m.makeFunc(funcName.c_str(), fp.min, fp.max, [&m, closure](FArgs& a) {
            return m.execute(*closure, a);
        })->closure = closure;
        return std::make_unique<SymbolObject>(&m, nullptr, std::move(funcName));
    });
    m.defun("functionp", [](const Symbol& sym) {
        return sym.function && !sym.function->isMacro;
    });
    m.defun("func-arity", [&m](const Symbol& sym) {
        if (!sym.function) {
            throw exceptions::VoidFunction("void-function " + sym.name);
        }
        return m.makeConsCell(makeInt(sym.function->minArgs), makeInt(sym.function->maxArgs));
    });
}

}
