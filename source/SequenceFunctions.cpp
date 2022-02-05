#include "Machine.hpp"
#include <cstdint>
#include <stdexcept>

namespace alisp
{

void initSequenceFunctions(Machine& m)
{
    m.defun("length", [](const Sequence& ptr) {
        return ptr.length();
    });
    m.defun("elt", [&m](const Sequence& ptr, std::int64_t index) {
        try {
            return ptr.elt(index);
        }
        catch (std::runtime_error& ex) {
            throw exceptions::Error(m, "Index out of range.");
        }
    });
    m.defun("sequencep", [](ObjectPtr obj) {
        return dynamic_cast<Sequence*>(obj.get()) != nullptr;
    });
}

}
