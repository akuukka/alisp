#include "AtScopeExit.hpp"
#include "alisp.hpp"
#include "Error.hpp"
#include "ConsCell.hpp"
#include "Symbol.hpp"
#include "SymbolObject.hpp"
#include "ConsCellObject.hpp"
#include "StringObject.hpp"
#include "Machine.hpp"
#include <limits>

namespace alisp {
namespace exceptions {

ALISP_STATIC std::string getErrorMessageFrom(const std::unique_ptr<Object>& data)
{
    if (data->isList() && data->asList()->car() && data->asList()->car()->isString()) {
        return data->asList()->car()->toString(true);
    }
    return "";
}

Error::Error(std::string msg, std::string symbolName) :
    Exception(msg), symbolName(symbolName), message(msg)
{
    
}

Error::Error(std::unique_ptr<SymbolObject> sym,
             std::unique_ptr<Object> data) :
    Exception(getErrorMessageFrom(data)),
    sym(std::move(sym)), data(std::move(data))
{
    
}

Error::~Error() {}


void Error::onHandle(Machine& machine)
{
    if (sym && data) {
        symbolName = sym->getSymbolName();
        if (data->isList() && data->asList()->car()) {
            message = data->asList()->car()->toString(true);
        }
        return;
    }
    ListBuilder builder(machine);
    builder.append(std::make_unique<StringObject>(message));

    data = builder.get();
    sym = std::make_unique<SymbolObject>(&machine, machine.getSymbol(symbolName), "");
}

std::string Error::getMessageString()
{
    if (!data || !sym) {
        return symbolName + ": " + message;
    }
    auto errorMessage = get(sym->getSymbol()->plist,
                            *sym->parent->makeSymbol("error-message", true));
    std::string msg = (errorMessage ? errorMessage->toString(true) : sym->toString() ) + ": ";
    if (data->isList()) {
        bool first = true;
        for (auto& obj : *data->asList()) {
            if (!first) msg += "\n\n";
            first = false;
            msg += obj.toString(true);
        }
    }
    else {
        msg += data->toString(true);
    }
    return msg;
}

}

ALISP_STATIC bool handlerMatches(const SymbolObject& error, const SymbolObject& handler)
{
    if (error.eq(handler)) {
        return true;
    }
    const auto prop = error.parent->makeSymbol("error-conditions", true);
    const auto matchesHandlers = get(error.getSymbol()->plist, *prop);
    if (matchesHandlers->isList()) {
        for (const auto& obj : *matchesHandlers->asList()) {
            if (handler.eq(obj)) {
                return true;
            }
        }
    }
    return false;
}

void Machine::initErrorFunctions()
{
    defun("signal", [&](std::shared_ptr<Symbol> sym, const Object& data) {
        throw exceptions::Error(std::make_unique<SymbolObject>(this,
                                                               sym,
                                                               ""),
                                data.clone());
    });
    defun("error-message-string", [](const ConsCell* err) {
        if (!err) {
            return std::string("peculiar error");
        }
        return err->car->toString() + ":" + err->cdr->toString();
    });
    makeFunc("condition-case", 2, std::numeric_limits<int>::max(), [&](FArgs& args) {
        auto arg = args.pop(false);
        if (!arg->isSymbol() && !arg->isNil()) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        const std::string symName = arg->isNil() ? NilName : arg->asSymbol()->getSymbolName();
        try {
            auto protectedForm = args.pop(false);
            return protectedForm->eval();
        }
        catch (exceptions::Error& error) {
            error.onHandle(args.m);
            while (args.hasNext()) {
                auto next = args.pop(false);
                if (!next->isList()) {
                    throw exceptions::Error("Invalid condition handler: " + next->toString());
                }
                auto nextCar = next->asList()->car();
                if (!nextCar) {
                    continue;
                }
                bool match = false;
                if (nextCar->isSymbol()) {
                    auto errSym = nextCar->asSymbol();
                    match = handlerMatches(*error.sym, *errSym);
                }
                else if (nextCar->isList()) {
                    for (auto& obj : *nextCar->asList()) {
                        if (obj.isSymbol()) {
                            match = handlerMatches(*error.sym, *obj.asSymbol());
                            if (match) break;
                        }
                        else {
                            throw exceptions::Error("Invalid condition handler: " +
                                                    next->toString());
                        }
                    }
                }
                else {
                    throw exceptions::Error("Invalid condition handler: " + next->toString());
                }
                if (match) {
                    pushLocalVariable(symName,
                                      args.m.makeConsCell(error.sym->clone(), error.data->clone()));
                    auto& m = args.m;
                    AtScopeExit onExit([&m, symName](){
                        m.popLocalVariable(symName);
                    });
                    auto cc = next->asList()->cc.get();
                    cc = cc->next();
                    ObjectPtr ret;
                    while (cc) {
                        ret = cc->car->eval();
                        cc = cc->next();
                    }
                    if (!ret) ret = makeNil();
                    return ret;
                }
            }
            throw;
        }
    });
}

}
