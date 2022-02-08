#include "Error.hpp"
#include "AtScopeExit.hpp"
#include "SymbolObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "ValueObject.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

void Machine::initListFunctions()
{
    defun("append", [this](const ConsCell& a, const ConsCell& b) {
        ListBuilder builder(*this);
        for (const auto& obj : a) { builder.append(obj); }
        b.iterateList([&](Object* obj, bool circular, Object* dotcdr) {
            if (circular) {
                throw exceptions::CircularList("Can't append");
            }
            builder.append(obj->clone());
            if (dotcdr) {
                builder.tail()->cdr = dotcdr->clone();
            }
            return true;
        });
        return builder.get();
    });
    defun("cons", [this](const Object& car, const Object& cdr) { 
        return makeConsCell(car.clone(), cdr.clone());
    });
    makeFunc("list", 0, std::numeric_limits<int>::max(), [](FArgs& args) {
        auto newList = makeList(&args.m);
        ConsCell *lastCc = newList->cc.get();
        bool first = true;
        for (auto obj : args) {
            if (!first) {
                lastCc->cdr = std::make_unique<ConsCellObject>(&args.m);
                lastCc = lastCc->next();
            }
            lastCc->car = obj->clone();
            first = false;
        }
        return newList;
    });
    makeFunc("dolist", 2, std::numeric_limits<int>::max(), [this](FArgs& args) {
        const auto p1 = args.pop(false)->asList();
        const std::string varName = p1->car()->asSymbol()->name;
        auto evaluated = p1->cdr()->asList()->car()->eval();
        auto codestart = args.cc;
        for (const auto& obj : *evaluated->asList()) {
            pushLocalVariable(varName, obj.clone());
            AtScopeExit onExit([this, varName](){ popLocalVariable(varName); });
            auto code = codestart;
            while (code) {
                code->car->eval();
                code = code->next();
            }
        }
        return makeNil();
    });
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
