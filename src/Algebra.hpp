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
#include "Matrix.hpp"
#include "Arithmetic.hpp"
#include "Error.hpp"

namespace PA
{
    using Vec2 = Vector<F32, 2>;
    using Vec3 = Vector<F32, 3>;
    using Vec4 = Vector<F32, 4>;

    using Mat2x2 = Matrix<F32, 2, 2>;
    using Mat3x3 = Matrix<F32, 3, 3>;
    using Mat4x4 = Matrix<F32, 4, 4>;

    template<typename TF>
    auto SolveCubic(TF a, TF b, TF c, TF d) -> StaticArray<TF, 3>;
}

namespace PA
{
    template<typename TF>
    auto SolveCubic(TF a, TF b, TF c, TF d) -> StaticArray<TF, 3>
    {
        static constexpr TF tolerance = TF(1) * Limits<TF>::epsilon();
        PA_ASSERT(a > tolerance);
        auto nan = Limits<TF>::quiet_NaN();
        StaticArray<TF, 3> roots =
        {
            nan,
            nan,
            nan
        };
        b = b / a;
        c = c / a;
        d = d / a;
        a = 1.f;
        // Transform into a depressed cubic.
        auto bSq = b * b;
        auto p = c - bSq / TF(3);
        auto q = (TF(2) * b * bSq - TF(9) * b * c) / TF(27) + d;

        auto delta = p * p * p / TF(27) + q * q / TF(4);

        if (delta > tolerance)
        {
            auto minusHalfQ = q / (-2.f);
            auto sqrtDelta = Sqrt(delta);
            roots[0] = Cbrt(minusHalfQ + sqrtDelta) + Cbrt(minusHalfQ - sqrtDelta);
        }
        else if (delta > -tolerance)
        {
            if (p > -tolerance && p < tolerance)
            {
                roots[0] = TF(0);
                roots[1] = TF(0);
                roots[2] = TF(0);
            }
            else
            {
                roots[0] = TF(3) * q / p;
                roots[1] = TF(-0.5) * roots[0];
                roots[2] = roots[1];
            }
        }
        else
        {
            // Use Viete trigonometric formula.
            auto tmp0 = TF(2) * Sqrt(-p / TF(3));
            auto tmp1 = TF(3) * q / (TF(2) * p) * Sqrt(TF(-3) / p);
            PA_ASSERT(tmp1 > TF(-1) && tmp1 < TF(1));
            auto tmp2 = TF(1) / TF(3) * ArcCos(tmp1);

            roots[0] = tmp0 * Cos(tmp2 - Constants<TF>::C2Pi / TF(3) * TF(0));
            roots[1] = tmp0 * Cos(tmp2 - Constants<TF>::C2Pi / TF(3) * TF(1));
            roots[2] = tmp0 * Cos(tmp2 - Constants<TF>::C2Pi / TF(3) * TF(2));
        }

        // Now "undepress" the roots.
        roots[0] = roots[0] - b / TF(3) ;
        roots[1] = roots[1] - b / TF(3) ;
        roots[2] = roots[2] - b / TF(3) ;

        return roots;
    }
}