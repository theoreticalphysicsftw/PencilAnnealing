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

#include "Vector.hpp"

namespace PA
{
    template <typename T, U32 TRows, U32 TCols>
    struct Matrix
    {
        union
        {
            T data[TRows * TCols];
            Vector<T, TRows> rows[TCols];
        };

        Vector<T, TCols>& operator[](U32 n) { return rows[n]; }
        const Vector<T, TCols>&& operator[](U32 n) const { return rows[n]; }

        template <typename... Ts>
            requires CAllAreConstructibleFrom<T, Ts...>
        Matrix(Ts... elements) : data{ static_cast<T>(elements)... } {};

        Vector<T, TCols> operator*(const Vector<T, TCols>& v)
        {
            Vector<T, TCols> result;
            for (auto i = 0; i < TCols; ++i)
            {
                result[i] = rows[i].Dot(v);
            }
            return result;
        }

        PA_DEFINE_COMPONENT_WISE_OPERATOR(Matrix, TRows * TCols, +);
        PA_DEFINE_COMPONENT_WISE_OPERATOR(Matrix, TRows * TCols, -);
        PA_DEFINE_COMPONENT_WISE_OPERATOR(Matrix, TRows * TCols, *);
        PA_DEFINE_COMPONENT_WISE_OPERATOR(Matrix, TRows * TCols, / );
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Matrix, TRows * TCols, +);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Matrix, TRows * TCols, -);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Matrix, TRows * TCols, *);
        PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(Matrix, TRows * TCols, / );
    };

    #define PA_DEFINE_MATRIX_COMPONENT_WISE_FUNCTION(FUNCTION_NAME) \
        template <typename T, U32 TRows, U32 TCols> \
        Matrix<T, TRows, TCols> FUNCTION_NAME (const Matrix<T, TRows, TCols>& v) \
        { \
            Matrix<T, TRows, TCols> result; \
            for (auto i = 0; i < TRows * TCols; ++i) \
            { \
                result.data[i] = FUNCTION_NAME(v.data[i]); \
            } \
            return result; \
        }

    PA_DEFINE_MATRIX_COMPONENT_WISE_FUNCTION(Sin);
    PA_DEFINE_MATRIX_COMPONENT_WISE_FUNCTION(Cos);
    PA_DEFINE_MATRIX_COMPONENT_WISE_FUNCTION(ArcSin);
    PA_DEFINE_MATRIX_COMPONENT_WISE_FUNCTION(ArcCos);

    #undef PA_DEFINE_MATRIX_COMPONENT_WISE_FUNCTION

    template <typename TF>
    auto CreateRotation(TF angle) -> Matrix<TF, 2, 2>
    {
        TF cA = Cos(angle);
        TF sA = Sin(angle);
        return Matrix<TF, 2, 2>(cA, -sA, sA, cA);
    }
}