#include "Machine.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

void initSymbolFunctions(Machine& m)
{
    m.defun("make-symbol", [&m](const std::string& name) -> ObjectPtr {
        std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>();
        symbol->name = name;
        return std::make_unique<SymbolObject>(&m, symbol, "");
    });
}

}
