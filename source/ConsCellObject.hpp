#pragma once
#include "ConsCell.hpp"
#include "Object.hpp"
#include "SharedValueObject.hpp"
#include "Sequence.hpp"
#include "Error.hpp"

namespace alisp {

class Machine;
struct Symbol;

struct ConsCellObject :
        SharedValueObjectBase,
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

    ConsCellObject(Machine* parent) : parent(parent) { }
    ConsCellObject(std::shared_ptr<ConsCell> value, Machine* machine) :
        cc(value),
        parent(machine) {}
    ConsCellObject(std::unique_ptr<Object> car, std::unique_ptr<Object> cdr, Machine* p) :
        ConsCellObject(p)
    {
        cc = std::make_shared<ConsCell>();
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
    bool operator!() const override { return !cc || !(*cc); }
    ConsCellObject* asList() override { return this; }
    const ConsCellObject* asList() const override { return this; }
    Object* car() const { return cc ? cc->car.get() : nullptr; };
    Object* cdr() const { return cc ? cc->cdr.get() : nullptr; };
    Object* cadr() const;
    Object* setCar(ObjectPtr obj) { cc->car = std::move(obj); return cc->car.get(); }
    Object* setCdr(ObjectPtr obj) { cc->cdr = std::move(obj); return cc->cdr.get(); }
    std::shared_ptr<Function> resolveFunction() const override;

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<ConsCellObject>(*this);
    }

    ObjectPtr copy() const override;
    ObjectPtr reverse() const override;

    bool equals(const Object &o) const override;
    size_t length() const override;
    std::unique_ptr<Object> eval() override;
    std::unique_ptr<Object> elt(std::int64_t index) const override;

    ConsCell::Iterator begin() const
    {
        return cc ? cc->begin() : ConsCell::Iterator{nullptr};
    }
    
    ConsCell::Iterator end() const
    {
        return cc ? cc->end() : ConsCell::Iterator{nullptr};
    }

    std::unique_ptr<ConsCellObject> deepCopy() const;
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
    bool canConvertTo(ConvertibleTo<ConsCell&>::Tag) const override;
    bool canConvertTo(ConvertibleTo<const ConsCell&>::Tag) const override;
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
    void dot(std::unique_ptr<Object> obj);
    ConsCell* tail() { return m_last; }
    std::unique_ptr<ConsCellObject> get();
};

}
