#include "ConsCellObject.hpp"
#include "Exception.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"
#include "AtScopeExit.hpp"

namespace alisp
{

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
        std::shared_ptr<Object> code;
        code.reset(args.cc->cdr.release());
        // std::cout << funcName << " defined as: " << *code << " (argc=" << argc << ")\n";
        m.makeFunc(funcName.c_str(), argc, argc,
                 [&m, funcName, argList, code](FArgs& a) {
                     /*if (argList.size()) {
                       std::cout << funcName << " being called with args="
                       << a.cc->toString() << std::endl;
                       }*/
                     size_t i = 0;
                     for (size_t i = 0; i < argList.size(); i++) {
                         m.pushLocalVariable(argList[i], a.pop()->clone());
                     }
                     AtScopeExit onExit([&m, argList]() {
                         for (auto it = argList.rbegin(); it != argList.rend(); ++it) {
                             m.popLocalVariable(*it);
                         }
                     });
                     assert(code->asList());
                     std::unique_ptr<Object> ret = m.makeNil();
                     for (auto& obj : *code->asList()) {
                         //std::cout << "exec: " << obj.toString() << std::endl;
                         ret = obj.eval();
                         //std::cout << " => " << ret->toString() << std::endl;
                     }
                     return ret;
                 });
        return std::make_unique<SymbolObject>(&m, nullptr, std::move(funcName));
    });
    m.defun("functionp", [](const Symbol* sym) {
        return sym->function && !sym->function->isMacro;
    });
    m.defun("func-arity", [&m](const Symbol* sym) {
        if (!sym || !sym->function) {
            throw exceptions::VoidFunction("void-function " +
                                           (sym ? sym->name : std::string("nil")));
        }
        ConsCellObject cc(&m);
        auto cell = std::shared_ptr<ConsCell>();
        cell->car = makeInt(sym->function->minArgs);
        cell->cdr = makeInt(sym->function->maxArgs);
        cc.cc = std::move(cell);
        return cc.clone();
    });
}

}
