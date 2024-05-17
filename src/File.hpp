// MIT License
// 
// Copyright (c) 2024 Mihail Mladenov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#pragma once

#include "Types.hpp"

#include <cstdio>
#include <filesystem>

namespace PA
{
	inline auto ReadWholeFile(StrView path, Array<Byte>& data) -> B;
	inline auto WriteWholeFile(StrView path, Span<const Byte> data) -> B;
    inline auto FileExists(StrView path) -> B;
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


	auto WriteWholeFile(StrView path, Span<const Byte> data) -> B
	{
        auto handle = fopen(Str(path).c_str(), "wb");

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