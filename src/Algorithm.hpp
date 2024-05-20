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

#include <algorithm>

namespace PA
{
	template <typename TInputIt, typename T>
	inline auto Find(TInputIt first, TInputIt last, const T& v) -> TInputIt;

	template <class TContainer>
	inline auto Fill(TContainer& c, const typename TContainer::value_type& value) -> V;

	template <typename T>
	inline auto GenerateSequence(T first, T last) -> Array<T>;

	template <typename TContainer, typename TComp>
	inline auto Sort(TContainer& c, TComp comp) -> V;
}


namespace PA
{
	template<typename TInputIt, typename T>
	inline auto Find(TInputIt first, TInputIt last, const T& v) -> TInputIt
	{
		return std::find(first, last, v);
	}


	template <typename TContainer>
	inline auto Fill(TContainer& c, const typename TContainer::value_type& value) -> V
	{
		std::fill(c.begin(), c.end(), value);
	}


	template<typename T>
	auto GenerateSequence(T first, T last) -> Array<T>
	{
		Array<T> result;

		for (auto i = first; i < last; ++i)
		{
			result.emplace_back(i);
		}

		return result;
	}


	template<typename TContainer, typename TComp>
	auto Sort(TContainer& c, TComp comp) -> V
	{
		std::sort(c.begin(), c.end(), comp);
	}

}