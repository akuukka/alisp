#include "ConsCellObject.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

namespace {

void renameSymbols(Machine&m, ConsCellObject& obj, std::map<std::string, Object*>& conv)
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

std::pair<bool, std::string> isMacroCall(const ConsCellObject* form)
{
    if (form && form->car() && form->car()->isSymbol()) {
        const SymbolObject* sym = dynamic_cast<const SymbolObject*>(form->car());
        assert(sym);
        const Function* func = sym->parent->resolveFunction(sym->name);
        if (func && func->isMacro) {
            return std::make_pair(true, sym->name);
        }
    }
    return std::make_pair(false, std::string());
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

ObjectPtr expand(Machine& m, const Macro& macro, std::function<Object*()> paramSource)
{
    std::map<std::string, Object*> conv;
    for (const auto& obj : macro.argList) {
        const SymbolObject* from =
            dynamic_cast<const SymbolObject*>(&obj);
        conv[from->name] = paramSource();
    }
    auto copied = macro.code.deepCopy();
    renameSymbols(m, *copied, conv);
    ObjectPtr ret;
    for (const auto& obj : *copied) {
        ret = obj.clone()->eval();
    }
    return ret;
}

ObjectPtr macroExpand(std::shared_ptr<std::map<std::string, Macro>> storage,
                      bool once,
                      ObjectPtr obj)
{
    if (!obj->isList() || obj->isNil()) {
        return obj->clone();
    }
    for (;;) {
        const auto form = obj->asList();
        const auto macroCall = isMacroCall(form);
        if (!macroCall.first) {
            break;
        }
        auto cc = form->cc.get();
        assert(storage->count(macroCall.second));
        const auto& macro = storage->at(macroCall.second);
        obj = expand(*form->parent,
                     macro,
                     [&cc](){ cc = cc->next(); return cc->car.get(); });
        if (once) {
            break;
        }
    }
    return obj->clone();
}

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
        ListBuilder builder(m);
        while (args.cc) {
            builder.append(args.cc->car->clone());
            args.skip();
        }
        auto code = builder.get();
        (*storage).insert(std::make_pair(macroName, Macro(argList, *code)));
        m.makeFunc(macroName.c_str(), argc, argc, [&m, macroName, storage](FArgs& a) {
            assert(storage->count(macroName));
            const auto& macro = storage->at(macroName);
            return expand(m, macro, [&a](){ return a.pop(false); })->eval();
        });
        m.getSymbol(macroName)->function->isMacro = true;
        return std::make_unique<SymbolObject>(&m, nullptr, std::move(macroName));
    });
    m.defun("macroexpand", [storage](ObjectPtr obj) { 
        return macroExpand(storage, false, std::move(obj));
    });
    m.defun("macroexpand-1", [storage](ObjectPtr obj) {
        return macroExpand(storage, true, std::move(obj));
    });
}

}
