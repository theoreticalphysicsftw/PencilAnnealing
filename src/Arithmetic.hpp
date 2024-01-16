// MIT License
// 
// Copyright (c) 2024 Mihail Mladenov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#pragma once

#include <cmath>
#include <limits>

#include "Types.hpp"

namespace PA
{
    template <typename T>
    using Limits = std::numeric_limits<T>;

    inline auto ArcCos(F32 s) -> F32;
    inline auto ArcSin(F32 s) -> F32;
    inline auto Sin(F32 a) -> F32;
    inline auto Cos(F32 a) -> F32;
    inline auto Frac(F32 n) -> F32;
}

namespace PA::ConstF32
{
    static constexpr F32 CPi = 3.1415927410125732421875f;
    static constexpr F32 C2Pi = 2.f * CPi;
}


namespace PA
{
    inline auto ArcCos(F32 c) -> F32
    {
        return std::acos(c);
    }

    inline auto ArcSin(F32 s) -> F32
    {
        return std::asin(s);
    }

    inline auto Sin(F32 a) -> F32
    {
        return std::asin(a);
    }

    inline auto Cos(F32 a) -> F32
    {
        return std::cos(a);
    }

    inline auto Frac(F32 n) -> F32
    {
        F32 integralPart;
        return std::modf(n, &integralPart);
    }
}
