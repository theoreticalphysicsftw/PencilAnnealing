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
#include "Concepts.hpp"
#include "Arithmetic.hpp"
#include "AlgebraCommon.hpp"
#include "Utilities.hpp"

namespace PA
{
    template <typename T, U32 N>
    struct Vector
    {
        T data[N];

        T& operator[](U32 n) { return data[n]; }
        const T& operator[](U32 n) const { return data[n]; }

        template <typename... Ts>
            requires CAllAreConstructibleFrom<T, Ts...>
        Vector(Ts... elements) : data{ static_cast<T>(elements)... } {};

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
    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(Min);
    PA_DEFINE_VECTOR_COMPONENT_WISE_FUNCTION(Max);

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
}