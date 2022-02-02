#include "ConsCellObject.hpp"
#include "Exception.hpp"
#include "Machine.hpp"
#include "Object.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

void initSymbolFunctions(Machine& m)
{
    m.defun("getf", [&m](const ConsCell& list,
                         const Symbol& sym,
                         std::optional<ObjectPtr> defaultValue) -> ObjectPtr
    {
        bool skip = false;
        bool match = false;
        for (const auto& obj : list) {
            if (match) {
                return obj.clone();
            }
            if (!skip) {
                if (obj.isSymbol() && obj.asSymbol()->getSymbol().get() == &sym) {
                    match = true;
                }
            }
            skip = !skip;
        }
        if (match) {
            throw exceptions::Error("Malformed property list.");
        }
        if (defaultValue) {
            return defaultValue->get()->clone();
        }
        return m.makeNil();
    });
}

}
