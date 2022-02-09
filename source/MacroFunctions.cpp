#include "alisp.hpp"
#include "ConsCellObject.hpp"
#include "Function.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

ALISP_STATIC void renameSymbols(Machine&m, ConsCellObject& obj, std::map<std::string, Object*>& conv)
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

ALISP_STATIC std::pair<bool, ConsCellObject*> isMacroCall(const ConsCellObject* form)
{
    if (form && form->car() && form->car()->isSymbol()) {
        const SymbolObject* sym = dynamic_cast<const SymbolObject*>(form->car());
        assert(sym);
        if (sym->getSymbol()->function->isList()) {
            auto list = sym->getSymbol()->function->asList();
            if (list->car() && list->car()->isSymbol() &&
                list->car() == form->parent->getSymbol(MacroName))
            {
                return std::make_pair(true, list);
            }
        }
    }
    return std::make_pair(false, nullptr);
}

ObjectPtr expand(Machine& m,
                 ConsCellObject* code,
                 std::function<Object*()> paramSource)
{
    const auto params = getFuncParams(*code->cdr()->asList());
    code = code->cdr()->asList()->cdr()->asList();
    ListBuilder builder(m);
    ObjectPtr restList;
    std::map<std::string, Object*> conv;
    const int nc = static_cast<int>(params.names.size());
    for (int i = 0; i < nc; i++) {
        if (params.rest && i == nc - 1) {
            auto next = paramSource();
            while (next) {
                builder.append(next->clone());
                next = paramSource();
            }
            restList = builder.get();
            conv[params.names[i]] = restList.get();
        }
        else {
            conv[params.names[i]] = paramSource();
        }
    }
    auto copied = code->deepCopy();
    renameSymbols(m, *copied, conv);
    ObjectPtr ret;
    for (const auto& obj : *copied) {
        ret = obj.clone()->eval();
    }
    return ret;
}

ObjectPtr macroExpand(bool once,
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
        auto code = macroCall.second->cdr()->asList();
        auto cc = form->cc.get();
        obj = expand(*form->parent,
                     code,
                     [&cc](){ cc = cc->next(); return cc ? cc->car.get() : nullptr; });
        if (once) {
            break;
        }
    }
    return obj->clone();
}

void initMacroFunctions(Machine& m)
{
    m.makeFunc("defmacro", 2, std::numeric_limits<int>::max(), [&m](FArgs& args) {
        const SymbolObject* nameSym = dynamic_cast<SymbolObject*>(args.current());
        if (!nameSym || nameSym->name.empty()) {
            throw exceptions::WrongTypeArgument(args.cc->car->toString());
        }
        std::string macroName = nameSym->name;
        ListBuilder builder(m);
        builder.append(m.makeSymbol("macro", true));
        builder.append(m.makeSymbol("lambda", true));
        args.skip();
        auto cc = args.cc;
        while (cc && cc->car) {
            builder.append(cc->car->clone());
            cc = cc->next();
        }
        m.getSymbol(macroName)->function = builder.get();
        return std::make_unique<SymbolObject>(&m, nullptr, std::move(macroName));
    });
    m.defun("macroexpand", [](ObjectPtr obj) { 
        return macroExpand(false, std::move(obj));
    });
    m.defun("macroexpand-1", [](ObjectPtr obj) {
        return macroExpand(true, std::move(obj));
    });
}

}
