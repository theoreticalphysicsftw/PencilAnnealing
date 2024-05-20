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