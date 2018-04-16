
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017-2018 Francois Beaune, The appleseedhq Organization
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
#include "seexprutils.h"

// appleseed-max headers.
#include "utilities.h"

// appleseed.foundation headers.
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <assert1.h>
#include <bitmap.h>
#include <stdmat.h>

namespace asf = foundation;
namespace asr = renderer;

std::string fmt_se_expr(const asf::Color3f& srgb)
{
    return asf::format("[{0}, {1}, {2}]", srgb.r, srgb.g, srgb.b);
}

std::string fmt_se_expr(BitmapTex* bitmap_tex)
{
    DbgAssert(bitmap_tex != nullptr);
    const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());

    Bitmap* bitmap = bitmap_tex->GetBitmap(0);
    DbgAssert(bitmap != nullptr);

    const int width = bitmap->Width();
    const int height = bitmap->Height();

    return asf::format("texture(\"{0}\", $u % {1}, $v % {2})", filepath, width, height);
}

std::string fmt_se_expr(const float scalar, Texmap* map)
{
    auto value = asf::to_string(scalar);

    if (is_bitmap_texture(map))
        value += asf::format(" * {0}", fmt_se_expr(static_cast<BitmapTex*>(map)));

    return value;
}
