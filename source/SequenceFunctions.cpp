#include "Error.hpp"
#include "Machine.hpp"
#include <cstdint>
#include <stdexcept>
#include "Sequence.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

void Machine::initSequenceFunctions()
{
    defun("length", [](const Sequence& ptr) {
        return ptr.length();
    });
    defun("elt", [](const Sequence& ptr, std::int64_t index) {
        try {
            return ptr.elt(index);
        }
        catch (std::runtime_error& ex) {
            throw exceptions::Error("Index out of range.");
        }
    });
    defun("sequencep", [](ObjectPtr obj) {
        return dynamic_cast<Sequence*>(obj.get()) != nullptr;
    });
    defun("reverse", [](const Sequence& seq) { return seq.reverse(); });
    defun("copy-sequence", [](const Sequence& seq) { return seq.copy(); });
    defun("mapcar", [](const Function& func, const Sequence& seq) { return seq.mapCar(func); });
    defun("mapc", [](const Function& func, const Object& obj) {
        auto seq = dynamic_cast<const Sequence*>(&obj);
        if (!seq) {
            throw exceptions::WrongTypeArgument(obj.toString());
        }
        seq->mapCar(func);
        return obj.clone();
    });
    defun("nreverse", [this](const Object& obj) -> ObjectPtr {
        if (obj.isList()) {
            auto list = obj.asList();
            auto head = list->cc;
            auto tail = list->cc;
            std::set<const ConsCell*> heads;
            while (tail && tail->cdr) {
                assert(tail->cdr->isList());
                auto newhead = tail->cdr->asList()->cc;
                if (heads.count(newhead.get())) {
                    throw exceptions::CircularList(obj.toString());
                }
                heads.insert(newhead.get());
                assert(tail->cdr->asList()->cc.get());
                std::shared_ptr<ConsCell> oldc;
                if (newhead->cdr) {
                    if (!newhead->cdr->asList()) {
                        auto to = ConsCellObject(newhead, this);
                        throw exceptions::WrongTypeArgument(to.toString());
                    }
                    assert(newhead->cdr->asList());
                    assert(newhead->cdr->asList()->cc);
                    oldc = newhead->cdr->asList()->cc;
                }
                newhead->cdr = std::make_unique<ConsCellObject>(head, this);
                tail->cdr = oldc ? std::make_unique<ConsCellObject>(oldc, this) : nullptr;
                head = newhead;
            }
            return std::make_unique<ConsCellObject>(head, this);
        }
        else {
            throw exceptions::WrongTypeArgument(obj.toString());
        }
    });
    defun("sort", [this](const Object& obj, const Function& pred) -> ObjectPtr {
        if (obj.isNil()) {
            return obj.clone();
        }
        else if (obj.isList()) {
            std::vector<std::shared_ptr<ConsCell>> ccs;
            std::set<const ConsCell*> inserted;
            auto ptr = obj.asList();
            while (ptr) {
                if (inserted.count(ptr->consCell().get())) {
                    throw exceptions::CircularList(obj.toString());
                }
                inserted.insert(ptr->consCell().get());
                ccs.push_back(ptr->consCell());
                if (ptr->cdr() && !ptr->cdr()->isList()) {
                    throw exceptions::WrongTypeArgument("listp " + ptr->cdr()->toString());
                }
                ptr = ptr->next();
            }
            ConsCell cca;
            cca.cdr = std::make_unique<ConsCellObject>(this);
            cca.cdr->asList()->cc = std::make_shared<ConsCell>();
            ConsCell& ccb = *cca.cdr->asList()->cc;            
            std::sort(ccs.begin(), ccs.end(), [&](const auto& a, const auto& b) {
                cca.car = a->car->clone();
                ccb.car = b->car->clone();
                FArgs args(cca, *this);
                return !pred.func(args)->isNil();
            });
            for (auto it = ccs.begin(); it != ccs.end(); ++it) {
                if (it + 1 == ccs.end()) {
                    it->get()->cdr = nullptr;
                }
                else {
                    it->get()->cdr = std::make_unique<ConsCellObject>(*(it +1), this);
                }
            }
            return std::make_unique<ConsCellObject>(ccs.front(), this);
        }
        throw exceptions::WrongTypeArgument(obj.toString());
    });
}

}
