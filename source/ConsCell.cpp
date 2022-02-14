#include "alisp.hpp"
#include <vector>
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

ALISP_STATIC bool traverseCycle(const ConsCell* cc,
                                std::vector<const ConsCell*>& visited,
                                size_t d)
{
    size_t added = 0;
    while (cc) {
        assert(cc && cc->car);
        //if (cc->car->isInt()) {
        //    std::cout << std::string(d, ' ') << cc->car->toString() << " " << cc << " "
        //              << visited.size() << std::endl;
        //}
        if (std::find(visited.begin(), visited.end(), cc) != visited.end()) {
            return true;
        }
        visited.push_back(cc);
        added++;
        if (cc->car && cc->car->isList() && !cc->car->isNil()) {
            if (traverseCycle(cc->car->asList()->cc.get(), visited, d+1)) {
                return true;
            }
        }
        cc = cc->next();
    }
    for (size_t i=0;i<added;i++) {
        visited.pop_back();
    }
    return false;
}

ALISP_INLINE bool ConsCell::isCyclical() const
{
    if (!*this) {
        return false;
    }
    std::vector<const ConsCell*> visited;
    return traverseCycle(this, visited, 0);
}

}
