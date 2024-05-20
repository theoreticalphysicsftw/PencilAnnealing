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

#define PA_DEFINE_COMPONENT_WISE_OPERATOR(CLASS, COMPONENTS, OP) \
    CLASS operator OP (const CLASS& other) const \
    { \
        CLASS result; \
        for (auto i = 0u; i < COMPONENTS; ++i) \
        { \
            result.data[i] = this->data[i] OP other.data[i]; \
        } \
        return result; \
    }

#define PA_DEFINE_COMPONENT_WISE_OPERATOR_SCALAR(CLASS, COMPONENTS, OP) const \
    CLASS operator OP (T scalar) \
    { \
        CLASS result; \
        for (auto i = 0u; i < COMPONENTS; ++i) \
        { \
            result.data[i] = this->data[i] OP scalar; \
        } \
        return result; \
    }

