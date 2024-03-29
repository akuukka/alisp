#include "ConsCell.hpp"
#include "Error.hpp"
#include "AtScopeExit.hpp"
#include "Object.hpp"
#include "SymbolObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "ValueObject.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

void Machine::initListFunctions()
{
    defun("nconc", [this](FArgs& args) -> ObjectPtr {
        if (!args.hasNext()) return makeNil();
        auto list = args.pop();
        while (list->isNil() && args.hasNext()) {
            list = args.pop();
        }
        auto origList = list;
        while (args.hasNext()) {
            assert(list->isList());
            assert(!list->isNil());
            auto next = args.pop();
            while (next->isNil()) {
                if (!args.hasNext()) {
                    break;
                }
                next = args.pop();
            }
            while (list->asList()->next()) {
                list = list->asList()->next();
            }
            list->asList()->setCdr(next->clone());
            list = next;
        }
        return origList->clone();
    });
    defun("list-length", [this](const Object& obj) -> ObjectPtr {
        requireType<ConsCellObject>(obj);
        try {
            return makeInt(static_cast<std::int64_t>(obj.asList()->length()));
        }
        catch (std::runtime_error&) {
            return makeNil();
        }
    });
    defun("copy-list", [this](const Object& obj) -> ObjectPtr {
        requireType<ConsCellObject>(obj);
        return obj.asList()->deepCopy();
    });
    defun("append", [this](const ConsCell* a, const ConsCell* b) {
        ListBuilder builder(*this);
        if (a) {
            for (const auto& obj : *a) { builder.append(obj); }
        }
        if (b) {
            b->iterateList([&](Object* obj, bool circular, Object* dotcdr) {
                if (circular) {
                    throw exceptions::CircularList("Can't append");
                }
                builder.append(obj->clone());
                if (dotcdr) {
                    builder.tail()->cdr = dotcdr->clone();
                }
                return true;
            });
        }
        return builder.get();
    });
    defun("cons", [this](const Object& car, const Object& cdr) { 
        return makeConsCell(car.clone(), cdr.clone());
    });
    defun("last", [this](const Object& obj) {
        requireType<ConsCellObject>(obj);
        auto ptr = dynamic_cast<const ConsCellObject*>(&obj);
        while (ptr->cdr() && ptr->cdr()->isList()) {
            ptr = ptr->cdr()->asList();
        }
        return ptr->clone();
    });
    makeFunc("list", 0, std::numeric_limits<int>::max(), [](FArgs& args) {
        ListBuilder builder(args.m);
        while (args.hasNext()) { builder.append(args.pop()->clone()); }
        return builder.get();
    });
    makeFunc("list*", 0, std::numeric_limits<int>::max(), [](FArgs& args) -> ObjectPtr {
        ListBuilder builder(args.m);
        bool first = true;
        while (args.hasNext()) {
            auto next = args.pop()->clone();
            if (args.hasNext()) {
                builder.append(std::move(next));
            }
            else {
                if (first) {
                    return next->clone();
                }
                builder.dot(std::move(next));
            }
            first = false;
        }
        return builder.get();
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
    defun("rplaca", [&](std::shared_ptr<ConsCell> cc, const Object& obj) {
        cc->car = obj.clone();
        return std::make_unique<ConsCellObject>(cc, this);
    });
    defun("rplacd", [&](std::shared_ptr<ConsCell> cc, const Object& obj) {
        cc->cdr = obj.clone();
        return std::make_unique<ConsCellObject>(cc, this);
    });
    defun("setcar", [](ConsCell& cc, ObjectPtr newcar) {
        return cc.car = newcar->clone(), std::move(newcar);
    });
    defun("setcdr", [](ConsCell& cc, ObjectPtr newcdr) {
        return cc.cdr = newcdr->clone(), std::move(newcdr);
    });
    defun("car", [&](const ConsCell* cc) { return cc && cc->car ? cc->car->clone() : makeNil(); });
    defun("cdr", [&](const ConsCell* cc) { return cc && cc->cdr ? cc->cdr->clone() : makeNil(); });
    defun("consp", [](const Object& obj) { return obj.isList() && !obj.isNil(); });
    defun("listp", [](const Object& obj) { return obj.isList(); });
    defun("nlistp", [](const Object& obj) { return !obj.isList(); });
    defun("proper-list-p", [this](const Object& obj) -> ObjectPtr {
        if (!obj.isList()) return makeNil();
        if (obj.isNil()) return makeInt(0);
        ConsCell* cc = obj.value<ConsCell*>();
        assert(cc);
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
    defun("memq", [this](const Object& object, const Object& listObj) {
        requireType<ConsCellObject>(listObj);
        if (listObj.isNil()) {
            return makeNil();
        }
        auto p = &listObj;
        while (p) {
            if (!p->isList()) {
                throw exceptions::WrongTypeArgument(ListpName + (", " + listObj.toString()));
            }
            if (p->asList()->car()->eq(object)) {
                return p->asList()->clone();
            }
            p = p->asList()->cdr();
        }
        return makeNil();
    });
    defun("delq", [this](const Object& object, const Object& listObj) {
        requireType<ConsCellObject>(listObj);
        if (listObj.isNil()) {
            return makeNil();
        }
        ConsCellObject ret(listObj.asList()->cc, this);
        while (ret.car() && ret.car()->eq(object)) {
            assert(!ret.cdr() || ret.cdr()->asList());
            ret.cc = ret.cdr() ? ret.cdr()->asList()->cc : nullptr;
        }
        if (!ret.cc) {
            return makeNil();
        }
        auto cc = ret.cc.get();
        assert(cc && !ret.cc->car->eq(object));
        while (cc && cc->cdr) {
            requireType<ConsCellObject>(*cc->cdr);
            assert(cc->cdr->asList()->car());
            if (cc->cdr->asList()->car()->eq(object)) {
                cc->cdr = std::move(cc->next()->cdr);
            }
            else {
                cc = cc->next();
            }
        }
        return ret.clone();
    });
}

}
