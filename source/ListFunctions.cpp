#include "alisp.hpp"
#include "Machine.hpp"
#include "ValueObject.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

void Machine::initListFunctions()
{
    defun("setcar", [](ConsCell& cc, ObjectPtr newcar) {
        return cc.car = newcar->clone(), std::move(newcar);
    });
    defun("setcdr", [](ConsCell& cc, ObjectPtr newcdr) {
        return cc.cdr = newcdr->clone(), std::move(newcdr);
    });
    defun("car", [this](const ConsCell& cc) { return cc.car ? cc.car->clone() : makeNil(); });
    defun("cdr", [this](const ConsCell& cc) { return cc.cdr ? cc.cdr->clone() : makeNil(); });
    defun("consp", [](const Object& obj) { return obj.isList() && !obj.isNil(); });
    defun("listp", [](const Object& obj) { return obj.isList(); });
    defun("nlistp", [](const Object& obj) { return !obj.isList(); });
    defun("proper-list-p", [this](const Object& obj) -> ObjectPtr {
        if (!obj.isList()) return makeNil();
        ConsCell* cc = obj.value<ConsCell*>();
        if (cc->isCyclical()) {
            return makeNil();
        }
        auto p = cc;
        std::int64_t count = p->car ? 1 : 0;
        while (p->cdr && p->cdr->isList()) {
            count++;
            p = p->cdr->value<ConsCell*>();
            if (p->cdr && !p->cdr->isList()) {
                return makeNil();
            }
        }
        return makeInt(count);
    });
    defun("make-list", [this](std::int64_t n, const Object& ptr) {
        std::unique_ptr<Object> r = makeNil();
        for (std::int64_t i=0; i < n; i++) {
            r = std::make_unique<ConsCellObject>(ptr.clone(), r->clone(), this);
        }
        return r;
    });
}

}