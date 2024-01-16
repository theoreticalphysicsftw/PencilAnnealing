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

#include "Types.hpp"

#include <concepts>
#include <type_traits>

namespace PA
{
    template <typename T, typename... Ts>
    concept CAllAreConstructibleFrom = (std::constructible_from<Ts, T> && ...); 

    template <typename T>
    concept CIsIntegral = std::integral<T>;

    template <typename T>
    concept CIsArithmetic = std::floating_point<T> || std::integral<T>;

    template <typename T, typename U>
    struct SizeAtLeast : std::integral_constant<bool, sizeof(T) >= sizeof(U)> {};

    template <typename T, typename U>
    concept CSizeAtLeast = SizeAtLeast<T, U>::value;

    template <typename T, typename U>
    concept CSizeAtMost = CSizeAtLeast<U, T>;
}
