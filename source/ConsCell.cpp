#include "alisp.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

ALISP_INLINE void ConsCell::iterateList(std::function<bool(Object* car,
                                                 bool isCircular,
                                                 Object* dotcdr)> f) const
{
    auto p = this;
    std::set<const ConsCell*> traversed;
    bool circular = false;
    Object* dotcdr = nullptr;
    while (p && p->car) {
        if (traversed.count(p)) {
            circular = true;
        }
        if (p->cdr && !p->cdr->isList()) {
            dotcdr = p->cdr.get();
        }
        if (!f(p->car.get(), circular, dotcdr)) {
            return;
        }
        traversed.insert(p);
        p = p->next();
    }
}

ALISP_INLINE const ConsCell* ConsCell::next() const
{
    auto cc = dynamic_cast<ConsCellObject*>(this->cdr.get());
    if (cc) {
        return cc->cc.get();
    }
    return nullptr;
}

ALISP_INLINE ConsCell* ConsCell::next()
{
    auto cc = dynamic_cast<ConsCellObject*>(this->cdr.get());
    if (cc) {
        return cc->cc.get();
    }
    return nullptr;
}

ALISP_INLINE void ConsCell::traverse(const std::function<bool(const ConsCell*)>& f) const
{
    if (!*this) {
        return;
    }
    const ConsCell* cell = this;
    while (cell) {
        if (!f(cell)) {
            return;
        }
        if (cell->car->isList() && !cell->car->isNil()) {
            cell->car->asList()->cc.get()->traverse(f);
        }
        cell = cell->next();
    }
}

ALISP_INLINE bool ConsCell::isCyclical() const
{
    if (!*this) {
        return false;
    }
    size_t selfTimes = 0;
    bool cycled = false;
    std::set<const ConsCell*> visited;
    traverse([&](const ConsCell* cell) {
        if (cell == this) {
            selfTimes++;
            if (selfTimes >= 2) {
                cycled = true;
                return false;
            }
        }
        if (visited.count(cell)) {
            return false;
        }
        visited.insert(cell);
        return true;
    });
    return cycled;
}

}
