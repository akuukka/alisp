#include "ConsCellObject.hpp"
#include "Exception.hpp"
#include "Machine.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"
#include <cmath>
#include <functional>
#include <limits>

namespace alisp
{

ALISP_INLINE double toDouble(std::variant<double, std::int64_t> var)
{
    try {
        return std::get<std::int64_t>(var);
    }
    catch (std::bad_variant_access&) {
        return std::get<double>(var);
    }
}

ALISP_INLINE
std::int64_t truncate(std::variant<double, std::int64_t> obj,
                      std::optional<std::variant<double, std::int64_t>> divisor)
{
    std::optional<double> div;
    if (divisor) {
        div = toDouble(*divisor);
        if (*div == 0) {
            throw alisp::exceptions::ArithError("Division by zero");
        }
    }
    try {
        const std::int64_t i = std::get<std::int64_t>(obj);
        return div ? static_cast<std::int64_t>(i / *div) : i;
    }
    catch (std::bad_variant_access&) {
        if (!div) { div = 1; }
        return static_cast<std::int64_t>(std::get<double>(obj)/ *div);
    }
}

ALISP_INLINE std::int64_t floor(std::variant<double, std::int64_t> obj)
{
    return static_cast<std::int64_t>(std::floor(toDouble(obj)));
}

ALISP_INLINE std::int64_t ceiling(std::variant<double, std::int64_t> obj)
{
    return static_cast<std::int64_t>(std::ceil(toDouble(obj)));
}

void initMathFunctions(Machine& m)
{
    m.defun("truncate", truncate);
    m.defun("floor", floor);
    m.defun("ceiling", ceiling);
    m.defun("%", [](std::int64_t in1, std::int64_t in2) { return in1 % in2; });
    m.makeFunc("=", 1, 0xffff, [&m](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool fp = false;
        bool first = true;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                if (!first) {
                    if (v != f) {
                        return makeNil(&m);
                    }
                }
                f = v;
                fp = true;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                if (!first) {
                    if ((!fp && v != i) || (fp && v != f)) {
                        return makeNil(&m);
                    }
                }
                i = v;
                f = v;
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
            first = false;
        }
        return m.makeTrue();
    });
    m.defun("1+", [](std::variant<double, std::int64_t> obj) -> std::unique_ptr<Object> {
        try {
            const std::int64_t i = std::get<std::int64_t>(obj);
            return makeInt(i+1);
        }
        catch (std::bad_variant_access&) {
            const double f = std::get<double>(obj);
            return makeFloat(f+1.0);
        }
    });
    m.makeFunc("+", 0, 0xffff, [](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool fp = false;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                fp = true;
                i += v;
                f += v;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                i += v;
                f += v;
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
        }
        return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
            : static_cast<std::unique_ptr<Object>>(makeInt(i));
    });
    m.makeFunc("*", 0, 0xffff, [](FArgs& args) {
        std::int64_t i = 1;
        double f = 1;
        bool fp = false;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                fp = true;
                i *= v;
                f *= v;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                i *= v;
                f *= v;
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
        }
        return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
            : static_cast<std::unique_ptr<Object>>(makeInt(i));
    });
    m.makeFunc("/", 1, 0xffff, [](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool first = true;
        bool fp = false;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                if (v == 0) {
                    throw exceptions::ArithError("Division by zero");
                }
                fp = true;
                if (first) {
                    i = v;
                    f = v;
                }
                else {
                    i /= v;
                    f /= v;
                }
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                if (v == 0) {
                    throw exceptions::ArithError("Division by zero");
                }
                if (first) {
                    i = v;
                    f = v;
                }
                else {
                    i /= v;
                    f /= v;
                }
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
            first = false;
        }
        return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
            : static_cast<std::unique_ptr<Object>>(makeInt(i));
    });
    m.defun("<=", [](Number first, Rest& rest) {
        std::int64_t i = first.i;
        double f = first.f;
        for (auto sym : rest) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                if (f <= v) {

                }
                else {
                    return false;
                }
                first.isFloat = true;
                i = v;
                f = v;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                if (first.isFloat) {
                    double fv = static_cast<double>(v);
                    if (f <= fv) {

                    }
                    else {
                        return false;
                    }
                }
                else {
                    if (i <= v) {

                    }
                    else {
                        return false;
                    }
                }
                i = v;
                f = v;
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
        }
        return true;
    });
}

}
