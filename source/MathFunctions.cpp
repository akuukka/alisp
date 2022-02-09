#include "ConsCellObject.hpp"
#include "Error.hpp"
#include "Machine.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

namespace alisp
{

template<template<class> class Comp>
bool numberCompare(Number first,
                   Rest& rest)
{
    Comp<double> fcomp;
    Comp<std::int64_t> icomp;
    std::int64_t i = first.i;
    double f = first.f;
    for (auto sym : rest) {
        if (sym->isFloat()) {
            const double v = sym->value<double>();
            if (!fcomp(f, v)) {
                return false;
            }
            first.isFloat = true;
            f = v;
        }
        else if (sym->isInt()) {
            const std::int64_t v = sym->value<std::int64_t>();
            if (first.isFloat) {
                double fv = static_cast<double>(v);
                if (!fcomp(f, fv)) {
                    return false;
                }
            }
            else {
                if (!icomp(i, v)) {
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
}

ALISP_INLINE double toDouble(std::variant<double, std::int64_t> var)
{
    try {
        return std::get<std::int64_t>(var);
    }
    catch (std::bad_variant_access&) {
        return std::get<double>(var);
    }
}

ALISP_INLINE std::int64_t truncate(std::variant<double, std::int64_t> obj,
                                   std::optional<std::variant<double, std::int64_t>> divisor)
{
    std::optional<double> div;
    if (divisor) {
        div = toDouble(*divisor);
        if (*div == 0) {
            throw exceptions::ArithError("Division by zero");
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

ALISP_INLINE void Machine::initMathFunctions()
{
    defun("truncate", truncate);
    defun("floor", floor);
    defun("ceiling", ceiling);
    defun("isnan", [](double num) { return std::isnan(num); });
    defun("evenp", [](std::int64_t i) { return i % 2 == 0; });
    defun("%", [](std::int64_t in1, std::int64_t in2) { return in1 % in2; });
    defun("abs", [](Number num) {
        num.i = std::abs(num.i);
        num.f = std::abs(num.f);
        return num;
    });
    makeFunc("=", 1, 0xffff, [&](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool fp = false;
        bool first = true;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                if (!first) {
                    if (v != f) {
                        return makeNil();
                    }
                }
                f = v;
                fp = true;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                if (!first) {
                    if ((!fp && v != i) || (fp && v != f)) {
                        return makeNil();
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
        return makeTrue();
    });
    defun("1+", [](std::variant<double, std::int64_t> obj) -> std::unique_ptr<Object> {
        try {
            const std::int64_t i = std::get<std::int64_t>(obj);
            return makeInt(i+1);
        }
        catch (std::bad_variant_access&) {
            const double f = std::get<double>(obj);
            return makeFloat(f+1.0);
        }
    });
    makeFunc("+", 0, 0xffff, [](FArgs& args) {
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
    makeFunc("*", 0, 0xffff, [](FArgs& args) {
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
    makeFunc("/", 1, 0xffff, [](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool first = true;
        bool fp = false;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
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
                if (v == 0 && !first && !fp) {
                    throw exceptions::ArithError("Division by zero");
                }
                if (first) {
                    i = v;
                    f = v;
                }
                else {
                    if (!fp) i /= v;
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
    defun("<=", numberCompare<std::less_equal>);
    defun("<", numberCompare<std::less>);
    defun(">=", numberCompare<std::greater_equal>);
    defun(">", numberCompare<std::greater>);
    defun("ash", [](std::int64_t integer, std::int64_t count) {
        return count >= 0 ? (integer << count) : (integer >> (-count));
    });
    defun("lsh", [](std::int64_t integer, std::int64_t count) {
        return count >= 0 ? (integer >> count) : (integer << (-count));
    });
}

}


