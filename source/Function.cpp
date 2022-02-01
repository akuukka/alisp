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

struct Closure
{
    std::vector<std::string> argList;
    std::unique_ptr<Object> code;
};

ObjectPtr Machine::execute(Closure& closure, FArgs& a)
{
    size_t i = 0;
    for (size_t i = 0; i < closure.argList.size(); i++) {
        pushLocalVariable(closure.argList[i], a.pop()->clone());
    }
    AtScopeExit onExit([this, &closure]() {
        for (auto it = closure.argList.rbegin(); it != closure.argList.rend(); ++it) {
            popLocalVariable(*it);
        }
    });
    assert(closure.code->asList());
    std::unique_ptr<Object> ret = makeNil();
    for (auto& obj : *closure.code->asList()) {
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
        std::shared_ptr<Closure> closure = std::make_shared<Closure>();
        closure->argList = std::move(argList);
        closure->code = args.cc->cdr->clone();
        m.makeFunc(funcName.c_str(), argc, argc, [&m, closure](FArgs& a) {
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
