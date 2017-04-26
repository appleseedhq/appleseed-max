
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
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

// appleseed.renderer headers.
#include "renderer/api/color.h"
#include "renderer/api/scene.h"
#include "renderer/api/texture.h"

// appleseed.foundation headers.
#include "foundation/utility/searchpaths.h"
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <AssetManagement/AssetUser.h>
#include <assert1.h>
#include <bitmap.h>
#include <imtl.h>
#include <plugapi.h>
#include <stdmat.h>

namespace asf = foundation;
namespace asr = renderer;

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

bool is_bitmap_texture(Texmap* map)
{
    if (map == nullptr)
        return false;

    if (map->ClassID() != Class_ID(BMTEX_CLASS_ID, 0))
        return false;

    Bitmap* bitmap = static_cast<BitmapTex*>(map)->GetBitmap(0);

    if (bitmap == nullptr)
        return false;

    return true;
}

std::string fmt_expr(const asf::Color3f& srgb)
{
    return asf::format("[{0}, {1}, {2}]", srgb.r, srgb.g, srgb.b);
}

std::string fmt_expr(BitmapTex* bitmap_tex)
{
    DbgAssert(bitmap_tex != nullptr);
    const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());

    Bitmap* bitmap = bitmap_tex->GetBitmap(0);
    DbgAssert(bitmap != nullptr);

    const int width = bitmap->Width();
    const int height = bitmap->Height();

    return asf::format("texture(\"{0}\", $u % {1}, $v % {2})", filepath, width, height);
}

std::string fmt_expr(const float scalar, Texmap* map)
{
    auto value = asf::to_string(scalar);

    if (is_bitmap_texture(map))
        value += asf::format(" * {0}", fmt_expr(static_cast<BitmapTex*>(map)));

    return value;
}

void insert_color(asr::BaseGroup& base_group, const Color& color, const char* name)
{
    base_group.colors().insert(
        asr::ColorEntityFactory::create(
            name,
            asr::ParamArray()
                .insert("color_space", "linear_rgb")
                .insert("color", to_color3f(color))));
}

std::string insert_texture_and_instance(
    asr::BaseGroup& base_group,
    Texmap*         texmap,
    asr::ParamArray texture_params,
    asr::ParamArray texture_instance_params)
{
    BitmapTex* bitmap_tex = static_cast<BitmapTex*>(texmap);

    const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());
    texture_params.insert("filename", filepath);

    if (!texture_params.strings().exist("color_space"))
    {
        if (asf::ends_with(filepath, ".exr"))
            texture_params.insert("color_space", "linear_rgb");
        else texture_params.insert("color_space", "srgb");
    }

    const std::string texture_name = wide_to_utf8(bitmap_tex->GetName());
    if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
    {
        base_group.textures().insert(
            asr::DiskTexture2dFactory::static_create(
            texture_name.c_str(),
            texture_params,
            asf::SearchPaths()));
    }

    const std::string texture_instance_name = texture_name + "_inst";
    if (base_group.texture_instances().get_by_name(texture_instance_name.c_str()) == nullptr)
    {
        base_group.texture_instances().insert(
            asr::TextureInstanceFactory::create(
            texture_instance_name.c_str(),
            texture_instance_params,
            texture_name.c_str()));
    }
    return texture_instance_name;
}
