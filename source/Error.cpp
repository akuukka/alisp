#include "Error.hpp"
#include "SymbolObject.hpp"
#include "ConsCellObject.hpp"
#include "StringObject.hpp"
#include "Machine.hpp"

namespace alisp {
namespace exceptions {

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

Error::~Error() {}

}

void Machine::initErrorFunctions()
{
    
}

}
