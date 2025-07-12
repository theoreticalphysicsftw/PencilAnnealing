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
#include "Logging.hpp"
#include "String.hpp"

namespace PA::CLI
{
	struct Parser
	{
		using ArgType = Variant<B*, I32*, I64*, U8*, U16*, U32*, U64*, F32*, F64*, Str*>;

		template <typename T>
		auto Add(StrView name, T& writeLocation) -> V
		{
			args.emplace(name, ArgType(&writeLocation));
		}

		auto Parse(I32 argc, const C** argv, B log = true) -> V
		{
			for (auto i = 1u; i < argc; ++i)
			{
				auto arg = args.find(argv[i]);
				if (arg != args.end())
				{
					if (i + 1 < argc)
					{
						Visit<V>
						(
							[&](auto* v) -> V
							{
								using T = RemovePointer<RemoveReference<decltype(v)>>;
								*v = String::template To<T>(argv[i + 1]);
							},
							arg->second
						);
						i++;
					}
					else if (log)
					{
						Log("No value provided for argument \"", arg->first, "\"");
					}
				}
				else
				{
					Log("Unknown argument \"", argv[i], "\"");
				}
			}
		}

		Map<Str, ArgType> args;
	};
}