
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

// appleseed-max headers.
#include "main.h"

// appleseed.renderer headers.
#include "renderer/api/color.h"
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

// Windows headers.
#include <Shlwapi.h>

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

bool is_supported_texture(Texmap* map)
{
    if (map == nullptr)
        return false;

    switch (map->ClassID().PartA())
    {
      case CHECKER_CLASS_ID:
      case MARBLE_CLASS_ID:
      case MASK_CLASS_ID:
      case MIX_CLASS_ID:
      case NOISE_CLASS_ID:
      case GRADIENT_CLASS_ID:
      case TINT_CLASS_ID:
      case BMTEX_CLASS_ID:
      //case ACUBIC_CLASS_ID:       // Reflect/refract.
      //case MIRROR_CLASS_ID:       // Flat mirror.
      case COMPOSITE_CLASS_ID:
      case RGBMULT_CLASS_ID:
      //case FALLOFF_CLASS_ID:      // Falloff texture.
      case OUTPUT_CLASS_ID:
      //case PLATET_CLASS_ID:       // Plate glass texture
      case COLORCORRECTION_CLASS_ID:
      case 0x0000214:               // WOOD_CLASS_ID
      case 0x0000218:               // DENT_CLASS_ID
      case 0x46396cf1:              // PLANET_CLASS_ID
      case 0x7712634e:              // WATER_CLASS_ID
      case 0xa845e7c:               // SMOKE_CLASS_ID
      case 0x62c32b8a:              // SPECKLE_CLASS_ID
      case 0x90b04f9:               // SPLAT_CLASS_ID
      case 0x9312fbe:               // STUCCO_CLASS_ID
          return true;
          break;
      default:
          return false;
          break;
    }
    
    return true;
}

std::string get_root_path()
{
    wchar_t path[MAX_PATH];
    const DWORD path_length = sizeof(path) / sizeof(wchar_t);

    const auto result = GetModuleFileName(g_module, path, path_length);
    DbgAssert(result != 0);

    PathRemoveFileSpec(path);

    return wide_to_utf8(path);
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

WStr replace_extension(const WStr& file_path, const WStr& new_ext)
{
    const int i = file_path.last(L'.');
    WStr new_file_path = i == -1 ? file_path : file_path.Substr(0, i);
    new_file_path.Append(new_ext);
    return new_file_path;
}
