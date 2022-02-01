#include "ConsCellObject.hpp"
#include "Exception.hpp"
#include "Template.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"
#include "AtScopeExit.hpp"

namespace alisp
{

struct FuncParams {
    int min = 0;
    int max = 0;
    std::vector<std::string> names;
};

inline FuncParams getFuncParams(ConsCellObject& closure)
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

struct Closure
{
    std::unique_ptr<ConsCellObject> data;
};

ObjectPtr Machine::execute(Closure& closure, FArgs& a)
{
    const auto fp = getFuncParams(*closure.data);
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
    assert(closure.data->asList());
    std::unique_ptr<Object> ret = makeNil();
    for (auto& obj : *closure.data->asList()->cdr()->asList()) {
        ret = obj.eval();
    }
    return ret;
}

void initFunctionFunctions(Machine& m)
{
    m.makeFunc("defun", 2, std::numeric_limits<int>::max(), [&m](FArgs& args) {
        const SymbolObject* nameSym = dynamic_cast<SymbolObject*>(args.cc->car.get());
        if (!nameSym || nameSym->name.empty()) {
            throw exceptions::WrongTypeArgument(args.cc->car->toString());
        }
        std::string funcName = nameSym->name;
        args.skip();
        std::shared_ptr<Closure> closure = std::make_shared<Closure>();
        closure->data = m.makeConsCell(args.cc->car->clone(), args.cc->cdr->clone());
        const FuncParams fp = getFuncParams(*closure->data);
        m.makeFunc(funcName.c_str(), fp.min, fp.max, [&m, closure](FArgs& a) {
            return m.execute(*closure, a);
        });
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
