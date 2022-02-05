#include "alisp.hpp"
#include "Error.hpp"
#include "ConsCell.hpp"
#include "Symbol.hpp"
#include "SymbolObject.hpp"
#include "ConsCellObject.hpp"
#include "StringObject.hpp"
#include "Machine.hpp"

namespace alisp {
namespace exceptions {

ALISP_STATIC std::string getErrorMessageFrom(const std::unique_ptr<Object>& data)
{
    if (data->isList() && data->asList()->car() && data->asList()->car()->isString()) {
        return data->asList()->car()->toString(true);
    }
    return "";
}

Error::Error(Machine& machine, std::string msg) :
    Exception(msg)
{
    ListBuilder builder(machine);
    builder.append(std::make_unique<StringObject>(msg));

    data = builder.get();
    sym = std::make_unique<SymbolObject>(&machine,
                                         machine.getSymbol(machine.parsedSymbolName("error")),
                                         "");
}

Error::Error(std::unique_ptr<SymbolObject> sym,
             std::unique_ptr<Object> data) :
    Exception(getErrorMessageFrom(data)),
    sym(std::move(sym)), data(std::move(data))
{
    
}


Error::~Error() {}

}

void Machine::initErrorFunctions()
{
    defun("signal", [&](std::shared_ptr<Symbol> sym, ObjectPtr data) {
        throw exceptions::Error(std::make_unique<SymbolObject>(this,
                                                               sym,
                                                               ""),
                                data->clone());
    });
}

}
