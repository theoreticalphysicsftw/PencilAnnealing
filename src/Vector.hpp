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
#include "Concepts.hpp"
#include "Arithmetic.hpp"
#include "AlgebraCommon.hpp"
#include "Utilities.hpp"

namespace PA
{
    template <typename T, U32 N>
    struct Vector
    {
        StaticArray<T, N> data;

        T& operator[](U32 n) { return data[n]; }
        const T& operator[](U32 n) const { return data[n]; }

        template <typename... Ts>
            requires CAllAreConstructibleFrom<T, Ts...>
        Vector(Ts... elements) : data{ static_cast<T>(elements)... } {};
        Vector(T fill) { for (auto i = 0; i < N; ++i) data[i] = fill; }

        T Dot(const Vector& other) const
        {
            T sum = T(0);
            for (auto i = 0u; i < N; ++i)
            {
                sum += data[i] * other.data[i];
            }
            return sum;
        }

        T Length() const
        {
            return Sqrt(this->Dot(*this));
        }

        PA_DEFINE_COMPONENT_WISE_OPERATOR(Vector, N, +);
        PA_DEFINE_COMPONENT_WISE_OPERATOR(Vector, N, -);
        PA_DEFINE_COMPONENT_WISE_OPERATOR(Vector, N, *);
        PA_DEFINE_COMPONENT_WISE_OPERATOR(Vector, N, /);
        PA_DEFINE_COMPONENT_WISE_OPERATOR(Vector, N, %);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Vector, N, +);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Vector, N, -);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Vector, N, *);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Vector, N, /);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Vector, N, %);
    };
    
    #define PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(FUNCTION_NAME) \
        template <typename T, U32 N> \
        Vector<T, N> FUNCTION_NAME (const Vector<T, N>& v) \
        { \
            Vector<T, N> result; \
            for (auto i = 0; i < N; ++i) \
            { \
                result.data[i] = FUNCTION_NAME(v.data[i]); \
            } \
            return result; \
        }

    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(Sin);
    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(Cos);
    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(ArcSin);
    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(ArcCos);
    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(Abs);
    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(Sqrt);

    #undef PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION

    #define PA_DEFINE_VECTOR_LEFT_SCALAR_OPERATOR(OP) \
        template <typename T, U32 N> \
        Vector<T, N> operator OP (T scalar, const Vector<T, N>& v) \
        { \
            Vector<T, N> result; \
            for (auto i = 0; i < N; ++i) \
            { \
                result.data[i] = scalar OP v.data[i]; \
            } \
            return result; \
        }
    
    PA_DEFINE_VECTOR_LEFT_SCALAR_OPERATOR(+);
    PA_DEFINE_VECTOR_LEFT_SCALAR_OPERATOR(-);
    PA_DEFINE_VECTOR_LEFT_SCALAR_OPERATOR(*);
    PA_DEFINE_VECTOR_LEFT_SCALAR_OPERATOR(/);
    PA_DEFINE_VECTOR_LEFT_SCALAR_OPERATOR(%);

    #undef PA_DEFINE_VECTOR_LEFT_SCALAR_OPERATOR

    template <typename TF, U32 N>
    auto SquaredDistance(const Vector<TF, N>& v0, const Vector<TF, N>& v1) -> TF
    {
        auto dir = v0 - v1;
        return dir.Dot(dir);
    }

    template <typename TF, U32 N>
    auto Distance(const Vector<TF, N>& v0, const Vector<TF, N>& v1) -> TF
    {
        return Sqrt(SquaredDistance(v0, v1));
    }

    template <typename TF, U32 N>
    auto Max(const Vector<TF, N>& v0, const Vector<TF, N>& v1) -> Vector<TF, N>
    {
        Vector<TF, N> result;
        for (auto i = 0u; i < N; ++i)
        {
            result[i] = Max(v0[i], v1[i]);
        }
        return result;
    }

    template <typename TF, U32 N>
    auto Min(const Vector<TF, N>& v0, const Vector<TF, N>& v1) -> Vector<TF, N>
    {
        Vector<TF, N> result;
        for (auto i = 0u; i < N; ++i)
        {
            result[i] = Min(v0[i], v1[i]);
        }
        return result;
    }

    template <typename TF, U32 N>
    auto operator==(const Vector<TF, N>& v0, const Vector<TF, N>& v1) -> B
    {
        return v0.data == v1.data;
    }
}
