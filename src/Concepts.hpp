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
