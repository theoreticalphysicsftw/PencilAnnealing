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

namespace PA
{
	static constexpr U32 CEmbeddedTestImageSize = 47690;
	static constexpr U32 CEmbeddedMediumTestImageSize = 21478;
	static constexpr U32 CEmbeddedSmallTestImageSize = 8854;


	extern Byte GEmbeddedTestImageData[CEmbeddedTestImageSize];
	extern Byte GEmbeddedMediumTestImageData[CEmbeddedMediumTestImageSize];
	extern Byte GEmbeddedSmallTestImageData[CEmbeddedSmallTestImageSize];
}
