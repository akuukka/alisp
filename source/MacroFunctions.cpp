#include "ConsCellObject.hpp"
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

struct Macro {
    ConsCellObject argList;
    ConsCellObject code;
    
    Macro(const ConsCellObject& argList, const ConsCellObject& code) :
        argList(argList),
        code(code)
    {
        
    }
};

}

void initMacroFunctions(Machine& m)
{
    std::shared_ptr<std::map<std::string, Macro>> storage =
        std::make_shared<std::map<std::string, Macro>>();
    
    m.makeFunc("defmacro", 2, std::numeric_limits<int>::max(), [&m, storage](FArgs& args) {
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
        (*storage).insert(std::make_pair(macroName, Macro(argList, code)));
        m.makeFunc(macroName.c_str(), argc, argc, [&m, macroName, storage](FArgs& a) {
            const auto& argList = storage->at(macroName).argList;
            const auto& code = storage->at(macroName).code;
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
