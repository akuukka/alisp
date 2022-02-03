#pragma once
#include <vector>
#include "Object.hpp"

namespace alisp {

template<typename BaseObject>
struct MultiValueObject : BaseObject
{
    template <typename ... Args>
    MultiValueObject(Args&& ... args) : BaseObject(std::forward<Args>(args)...)
    {

    }

    MultiValueObject(const MultiValueObject& o) : BaseObject(o)
    {
        for (auto& obj : o.additionalValues) {
            additionalValues.push_back(obj->clone());
        }
    }
    
    std::vector<ObjectPtr> additionalValues;

    std::string toString(bool aesthetic) const override
    {
        std::string r = BaseObject::toString(aesthetic);
        for (size_t i=0;i<additionalValues.size();i++) {
            r += "\n";
            r += additionalValues[i]->toString();
        }
        return r;
    }

    ObjectPtr clone() const override
    {
        auto r = std::make_unique<MultiValueObject<BaseObject>>(*this);
        return r;
    }
};

}
