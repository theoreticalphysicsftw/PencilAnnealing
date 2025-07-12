// Copyright 2025 Mihail Mladenov
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


#include "Types.hpp"
#include "Arithmetic.hpp"

#include <cstring>

namespace PA::String
{
	template <typename T>
	auto To(StrView sv) -> T
	{
		return T(sv);
	}

	template <>
	auto To<B>(StrView sv) -> B
	{
		return strtoul(sv.data(), nullptr, 10);
	}

	template <>
	auto To<U8>(StrView sv) -> U8
	{
		return Clamp(U32(strtoul(sv.data(), nullptr, 10)), 0u, 255u);
	}

	template <>
	auto To<U16>(StrView sv) -> U16
	{
		return Clamp(U32(strtoul(sv.data(), nullptr, 10)), 0u, (1u << 16) - 1);
	}

	template <>
	auto To<U32>(StrView sv) -> U32
	{
		return strtoul(sv.data(), nullptr, 10);
	}

	template <>
	auto To<U64>(StrView sv) -> U64
	{
		return strtoull(sv.data(), nullptr, 10);
	}

	template <>
	auto To<I32>(StrView sv) -> I32
	{
		return strtol(sv.data(), nullptr, 10);
	}

	template <>
	auto To<I64>(StrView sv) -> I64
	{
		return strtoll(sv.data(), nullptr, 10);
	}

	template <>
	auto To<F32>(StrView sv) -> F32
	{
		return strtof(sv.data(), nullptr);
	}

	template <>
	auto To<F64>(StrView sv) -> F64
	{
		return strtod(sv.data(), nullptr);
	}
}