#include "Machine.hpp"
#include <cstdint>

namespace alisp
{

void initSequenceFunctions(Machine& m)
{
    m.defun("length", [](const Sequence* ptr) {
        return ptr->length();
    });
    m.defun("elt", [](const Sequence* ptr, std::int64_t index) {
        return ptr->elt(index);
    });
    m.defun("sequencep", [](ObjectPtr obj) {
        return dynamic_cast<Sequence*>(obj.get()) != nullptr;
    });
}

}
