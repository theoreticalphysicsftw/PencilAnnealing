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


#include "Serialization.hpp"
#include "File.hpp"

using namespace PA;

I32 main()
{
	Array<Vector<F32, 2>> in;
	Array<Byte> inOut;
	for (auto i = 0; i < 100; ++i)
	{
		in.emplace_back(F32(i), F32(i));
	}
	Serialize(inOut, in);
	Array<Vector<F32, 2>> inDeserialized;
	Span<const Byte> inSpan(inOut.data(), inOut.size());
	Deserialize(inSpan, inDeserialized);
	PA_ASSERT(!memcmp(inDeserialized.data(), in.data(), in.size() * sizeof(Vector<F32, 2>)));
}