// Copyright 2024 Mihail Mladenov
//
// This file is part of PencilAnnealing.
//
// PencilAnnealing is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PencilAnnealing is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PencilAnnealing.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include <cmath>
#include <limits>

#include "Types.hpp"
#include "Utilities.hpp"

namespace PA
{
    template <typename T>
    using Limits = std::numeric_limits<T>;

    template <typename TF>
    inline auto ArcCos(TF s) -> TF;
    template <typename TF>
    inline auto ArcSin(TF s) -> TF;
    template <typename TF>
    inline auto Sin(TF a) -> TF;
    template <typename TF>
    inline auto Cos(TF a) -> TF;
    template <typename TF>
    inline auto ArcTan2(TF y, TF x) -> TF;
    template <typename TF>
    inline auto Exp(TF n) -> TF;
    template <typename TF>
    inline auto Logarithm(TF n) -> TF;
    template <typename TF>
    inline auto Sqrt(TF n) -> TF;
    template <typename TF>
    inline auto Cbrt(TF n) -> TF;
    template <typename TF>
    inline auto Frac(TF n) -> TF;
    template <typename TF>
    inline auto Abs(TF n) -> TF;
    template <typename TF>
    inline auto Floor(TF n) -> TF;
    template <typename TF>
    inline auto Ceil(TF n) -> TF;

    template <typename T>
    inline auto SmoothStep(T range0, T range1, T x) -> T;

    template <typename TF>
    inline auto IsNan(TF n) -> B;

    template <typename T>
    inline auto Clamp(T value, T range0, T range1) -> T;


    template <typename TF>
    struct Constants
    {
        static constexpr TF CPi = TF(3.14159265358979323846264338327950288419716939937510582097494459230781640628620899);
        static constexpr TF C2Pi = TF(2) * CPi;
    };
}


namespace PA
{
    template <typename TF>
    inline auto ArcCos(TF c) -> TF
    {
        return std::acos(c);
    }


    template <typename TF>
    inline auto ArcSin(TF s) -> TF
    {
        return std::asin(s);
    }


    template <typename TF>
    inline auto Sin(TF a) -> TF
    {
        return std::sin(a);
    }


    template <typename TF>
    inline auto Cos(TF a) -> TF
    {
        return std::cos(a);
    }


    template<typename TF>
    auto ArcTan2(TF y, TF x) -> TF
    {
        return std::atan2(y, x);
    }


    template<typename TF>
    auto Exp(TF n) -> TF
    {
        return std::exp(n);
    }


    template<typename TF>
    auto Logarithm(TF n) -> TF
    {
        return std::log(n);
    }


    template<typename TF>
    auto Sqrt(TF n) -> TF
    {
        return std::sqrt(n);
    }


    template<typename TF>
    auto Cbrt(TF n) -> TF
    {
        return std::cbrt(n);
    }


    template <typename TF>
    inline auto Frac(TF n) -> TF
    {
        TF integralPart;
        return std::modf(n, &integralPart);
    }


    template<typename TF>
    auto Abs(TF n) -> TF
    {
        return std::abs(n);
    }

    template<typename TF>
    auto Floor(TF n) -> TF
    {
        return std::floor(n);
    }

    template<typename TF>
    auto Ceil(TF n) -> TF
    {
        return std::ceil(n);
    }


    template<typename T>
    auto SmoothStep(T range0, T range1, T x) -> T
    {
        x = Clamp((x - range0) / (range1 - range0), T(0), T(1));
        return x * x * (T(3) - T(2) * x);
    }


    template<typename TF>
    auto IsNan(TF n) -> B
    {
        return std::isnan(n);
    }


    template<typename T>
    auto Clamp(T value, T range0, T range1) -> T
    {
        return Min(Max(value, range0), range1);
    }
}
