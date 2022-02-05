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

}

void Machine::initErrorFunctions()
{
    defun("signal", [&](std::shared_ptr<Symbol> sym, ObjectPtr data) {
        throw exceptions::Error(std::make_unique<SymbolObject>(this,
                                                               sym,
                                                               ""),
                                data->clone());
    });
    defun("error-message-string", [](const ConsCell& err) {
        return err.car->toString() + ":" + err.cdr->toString();
    });
    makeFunc("condition-case", 2, std::numeric_limits<int>::max(), [&](FArgs& args) {
        auto arg = args.cc->car.get();
        args.skip();
        if (!arg->isSymbol() && !arg->isNil()) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        const std::string symName = arg->isNil() ? NilName : arg->asSymbol()->getSymbolName();
        auto protectedForm = args.pop(false);
        try {
            return protectedForm->eval();
        }
        catch (exceptions::Error& error) {
            error.onHandle(args.m);
            while (args.hasNext()) {
                auto next = args.pop(false);
                assert(next->isList());
                auto nextCar = next->asList()->car();
                assert(nextCar);
                if (!nextCar->isSymbol()) {
                    throw exceptions::Error("Invalid condition handler: " + next->asList()->toString());
                }
                auto errSym = nextCar->asSymbol();
                if (errSym->equals(*error.sym)) {
                    pushLocalVariable(symName,
                                      args.m.makeConsCell(error.sym->clone(), error.data->clone()));
                    auto& m = args.m;
                    AtScopeExit onExit([&m, symName](){
                        m.popLocalVariable(symName);
                    });
                    auto cc = next->asList()->cc.get();
                    cc = cc->next();
                    ObjectPtr ret = makeNil();
                    while (cc) {
                        ret = cc->car->eval();
                        cc = cc->next();
                    }
                    return ret;
                }
            }
            throw;
        }
    });
}

}
