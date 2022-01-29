#include "Converter.hpp"
#include "ConsCellObject.hpp"
#include "alisp.hpp"

namespace alisp
{

template<>
ALISP_INLINE std::optional<std::shared_ptr<Object>> getValue(const Object& sym)
{
    return std::shared_ptr<Object>(sym.clone().release());
}

template<>
ALISP_INLINE std::optional<bool> getValue(const Object &sym)
{
    return !!sym;
}

template<>
ALISP_INLINE std::optional<std::any> getValue(const Object& sym)
{
    auto s = dynamic_cast<const ConsCellObject*>(&sym);
    if (s) {
        std::shared_ptr<ConsCell> cc = s->cc;
        return cc;
    }
    return std::any();
}

}
