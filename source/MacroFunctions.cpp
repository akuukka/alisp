#include "Machine.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

namespace {

void renameSymbols(Machine&m, ConsCellObject& obj,
                   std::map<std::string, std::unique_ptr<Object>>& conv)
{
    auto p = obj.cc.get();
    while (p) {
        auto& obj = *p->car.get();
        SymbolObject* sym = dynamic_cast<SymbolObject*>(&obj);
        if (sym && conv.count(sym->name)) {
            p->car = m.quote(conv[sym->name]->clone());
        }
        ConsCellObject* cc = dynamic_cast<ConsCellObject*>(&obj);
        if (cc) {
            renameSymbols(m, *cc, conv);
        }
        p = p->next();
    }
}

}

void initMacroFunctions(Machine& m)
{
    m.makeFunc("defmacro", 2, std::numeric_limits<int>::max(), [&m](FArgs& args) {
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
        m.makeFunc(macroName.c_str(), argc, argc,
                 [&m, macroName, argList, code](FArgs& a) {
                     size_t i = 0;
                     std::map<std::string, std::unique_ptr<Object>> conv;
                     for (const auto& obj : argList) {
                         const SymbolObject* from =
                             dynamic_cast<const SymbolObject*>(&obj);
                         conv[from->name] = a.cc->car.get()->clone();
                         a.skip();
                     }
                     auto copied = code.deepCopy();
                     renameSymbols(m, *copied, conv);
                     return copied->eval()->eval();
                 });
        m.getSymbol(macroName)->function->isMacro = true;
        return std::make_unique<SymbolObject>(&m, nullptr, std::move(macroName));
    });
}

}
