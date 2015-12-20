
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015 Francois Beaune, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "utilities.h"

// Standard headers.
#include <sstream>

namespace asf = foundation;

std::string wide_to_utf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    const int wstr_size = static_cast<int>(wstr.size());
    const int result_size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_size, nullptr, 0, nullptr, nullptr);

    std::string result(result_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_size, &result[0], result_size, nullptr, nullptr);

    return result;
}

std::string wide_to_utf8(const TCHAR* wstr)
{
    const int result_size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);

    std::string result(result_size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], result_size - 1, nullptr, nullptr);

    return result;
}

std::wstring utf8_to_wide(const std::string& str)
{
    if (str.empty())
        return std::wstring();

    const int str_size = static_cast<int>(str.size());
    const int result_size = MultiByteToWideChar(CP_UTF8, 0, &str[0], str_size, nullptr, 0);

    std::wstring result(result_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], str_size, &result[0], result_size);

    return result;
}

std::wstring utf8_to_wide(const char* str)
{
    const int result_size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);

    std::wstring result(result_size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &result[0], result_size - 1);

    return result;
}

std::string fmt_color_expr(const asf::Color3f& srgb)
{
    std::stringstream sstr;
    sstr << '[';
    sstr << srgb.r; sstr << ", ";
    sstr << srgb.g; sstr << ", ";
    sstr << srgb.b;
    sstr << ']';
    return sstr.str();
}
