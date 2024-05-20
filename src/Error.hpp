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

#include "Logging.hpp"
#include "Utilities.hpp"

#ifdef PA_DEBUG
#define PA_ASSERT(ARG) \
	{ \
		Str PA_ASSERT_line = ToString(__LINE__); \
		Str PA_ASSERT_file = __FILE__; \
		Str PA_ASSERT_arg = #ARG; \
		if (!(ARG)) \
		{ \
			LogError(PA_ASSERT_file, ":", PA_ASSERT_line, "->", PA_ASSERT_arg); \
			Terminate(); \
		} \
    }
#else
#define PA_ASSERT(ARG)  (V)(ARG)
#endif

namespace PA
{

}