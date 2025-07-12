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

#include <cstdio>
#include <filesystem>

namespace PA
{
	inline auto ReadWholeFile(StrView path, Array<Byte>& data) -> B;
	inline auto WriteWholeFile(StrView path, Span<const Byte> data, B append = false) -> B;
    inline auto FileExists(StrView path) -> B;
    inline auto RemoveFile(StrView path) -> B;
    inline auto RemoveDirectoryRecursive(StrView path) -> B;
    inline auto CreateDirectory(StrView path) -> B;

    using Path = std::filesystem::path;
    using ErrorCode = std::error_code;
}


namespace PA
{
	auto ReadWholeFile(StrView path, Array<Byte>& data) -> B
	{
        auto handle = fopen(Str(path).c_str(), "rb");

        if (!handle)
        {
            return false;
        }

        fseek(handle, 0, SEEK_END);
        auto size = ftell(handle);
        fseek(handle, 0, SEEK_SET);

        data.resize(size);

        if (fread(data.data(), size, 1, handle) < 1)
        {
            fclose(handle);
            return false;
        }

        fclose(handle);
        return true;
	}


	auto WriteWholeFile(StrView path, Span<const Byte> data, B append) -> B
	{
        auto handle = fopen(Str(path).c_str(), append? "ab" : "wb");

        if (!handle)
        {
            return false;
        }

        if (fwrite(data.data(), data.size(), 1, handle) < 1)
        {
            fclose(handle);
            return false;
        }

        fclose(handle);
        return true;
	}


    inline auto FileExists(StrView p) -> B
    {
        Path path(p);
        if (std::filesystem::is_regular_file(path))
        {
            return true;
        }
        return false;
    }


    inline auto RemoveFile(StrView path) -> B
    {
       return std::filesystem::remove(path);
    }


    inline auto RemoveDirectoryRecursive(StrView path) -> B
    {
        ErrorCode ec;
        std::filesystem::remove_all(Path(path), ec);
        return ec.value() != -1;
    }


    inline auto CreateDirectory(StrView path) -> B
    {
        return std::filesystem::create_directory(Path(path));
    }
}