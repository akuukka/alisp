#include "Machine.hpp"
#include <cstdint>
#include <stdexcept>
#include "Sequence.hpp"

namespace alisp
{

void Machine::initSequenceFunctions()
{
    defun("length", [](const Sequence& ptr) {
        return ptr.length();
    });
    defun("elt", [](const Sequence& ptr, std::int64_t index) {
        try {
            return ptr.elt(index);
        }
        catch (std::runtime_error& ex) {
            throw exceptions::Error("Index out of range.");
        }
    });
    defun("sequencep", [](ObjectPtr obj) {
        return dynamic_cast<Sequence*>(obj.get()) != nullptr;
    });
    defun("reverse", [](const Sequence& ptr) {
        return ptr.reverse();
    });
}

}
