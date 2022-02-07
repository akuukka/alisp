#pragma once
#include "ConsCell.hpp"
#include "Object.hpp"
#include "SharedDataObject.hpp"
#include "Sequence.hpp"
#include "Error.hpp"

namespace alisp {

class Machine;
struct Symbol;

struct ConsCellObject :
        SharedDataObject,
        Sequence,
        ConvertibleTo<const Symbol&>,
        ConvertibleTo<Symbol&>,
        ConvertibleTo<const ConsCell&>,
        ConvertibleTo<std::shared_ptr<ConsCell>>,
        ConvertibleTo<ConsCell&>,
        ConvertibleTo<ConsCell*>,
        ConvertibleTo<const ConsCell*>
{
    std::shared_ptr<ConsCell> cc;
    Machine* parent = nullptr;

    ConsCellObject(Machine* parent) : parent(parent) { cc = std::make_shared<ConsCell>(); }
    ConsCellObject(std::shared_ptr<ConsCell> value, Machine* machine) :
        cc(value),
        parent(machine) {}
    ConsCellObject(std::unique_ptr<Object> car, std::unique_ptr<Object> cdr, Machine* p) :
        ConsCellObject(p)
    {
        this->cc->car = std::move(car);
        if (!cdr || !(*cdr)) {
            return;
        }
        this->cc->cdr = std::move(cdr);
    }

    ConsCellObject(const ConsCellObject& o) : cc(o.cc), parent(o.parent) {}

    ~ConsCellObject()
    {
        if (cc) tryDestroySharedData();
    }        

    void reset() override { cc.reset(); }
    std::string toString(bool aesthetic = false) const override;
    bool isList() const override { return true; }
    bool isNil() const override { return !(*this); }
    bool operator!() const override { return !(*cc); }
    ConsCellObject* asList() override { return this; }
    const ConsCellObject* asList() const override { return this; }
    Object* car() const { return cc->car.get();  };
    Object* cdr() const { return cc->cdr.get();  };
    Object* setCar(ObjectPtr obj) { cc->car = std::move(obj); return cc->car.get(); }
    Object* setCdr(ObjectPtr obj) { cc->cdr = std::move(obj); return cc->cdr.get(); }
    std::shared_ptr<Function> resolveFunction() const override;

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<ConsCellObject>(*this);
    }

    ObjectPtr copy() const override;
    ObjectPtr reverse() const override;

    bool equals(const Object &o) const override
    {
        const ConsCellObject *op = dynamic_cast<const ConsCellObject *>(&o);
        if (op && !(*this) && !(*op)) {
            return true;
        }
        return this->cc == op->cc;
    }

    size_t length() const override;
    std::unique_ptr<Object> eval() override;
    std::unique_ptr<Object> elt(std::int64_t index) const override;

    ConsCell::Iterator begin() const
    {
        return cc->begin();
    }
    
    ConsCell::Iterator end() const
    {
        return cc->end();
    }

    std::unique_ptr<ConsCellObject> deepCopy() const
    {
        std::unique_ptr<ConsCellObject> copy = std::make_unique<ConsCellObject>(parent);
        ConsCell *origPtr = cc.get();
        ConsCell *copyPtr = copy->cc.get();
        assert(origPtr && copyPtr);
        while (origPtr) {
            if (origPtr->car) {
                if (origPtr->car->isList()) {
                    const auto list = dynamic_cast<ConsCellObject*>(origPtr->car.get());
                    copyPtr->car = list->deepCopy();
                }
                else {
                    copyPtr->car = origPtr->car->clone();
                }
            }
            auto cdr = origPtr->cdr.get();
            origPtr = origPtr->next();
            if (origPtr) {
                copyPtr->cdr = std::make_unique<ConsCellObject>(parent);
                copyPtr = copyPtr->next();
            }
            else if (cdr) {
                assert(!cdr->isList());
                copyPtr->cdr = cdr->clone(); 
            }
        }
        return copy;
    }

    void traverse(const std::function<bool(const Object&)>& f) const override;

    const void* sharedDataPointer() const override { return cc.get(); }
    size_t sharedDataRefCount() const override { return cc.use_count(); }

    Symbol& convertTo(ConvertibleTo<Symbol&>::Tag) const override;
    const Symbol& convertTo(ConvertibleTo<const Symbol&>::Tag) const override;
    const ConsCell& convertTo(ConvertibleTo<const ConsCell&>::Tag) const override;
    ConsCell& convertTo(ConvertibleTo<ConsCell&>::Tag) const override;
    bool canConvertTo(ConvertibleTo<Symbol&>::Tag) const override;
    bool canConvertTo(ConvertibleTo<const Symbol&>::Tag) const override;
    std::shared_ptr<ConsCell>
    convertTo(ConvertibleTo<std::shared_ptr<ConsCell>>::Tag) const override { return cc; }
    ConsCell* convertTo(ConvertibleTo<ConsCell*>::Tag) const override { return cc.get(); }
    const ConsCell* convertTo(ConvertibleTo<const ConsCell*>::Tag) const override {
        return cc.get();
    }
};

inline std::unique_ptr<Object> makeNil(Machine* parent)
{
    return std::make_unique<ConsCellObject>(parent);
}

inline std::unique_ptr<ConsCellObject> makeList(Machine* parent)
{
    return std::make_unique<ConsCellObject>(parent);
}

class ListBuilder
{
    std::unique_ptr<ConsCellObject> m_list;
    ConsCell* m_last = nullptr;
    Machine& m_parent;
public:
    ListBuilder(Machine& parent) : m_parent(parent) {}
    void append(std::unique_ptr<Object> obj);
    void append(const Object& obj);
    std::unique_ptr<ConsCellObject> get();
};

}
