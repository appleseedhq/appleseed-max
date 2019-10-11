
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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

#pragma once

// appleseed-max-common headers.
#include "appleseed-max-common/utilities.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/scene.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/image.h"
#include "foundation/platform/windows.h"
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/string.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <GetCOREInterface.h>
#include <IPathConfigMgr.h>
#include <MaxDirectories.h>
#include <maxapi.h>
#include <maxtypes.h>
#include <strclass.h>
#include <Path.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Standard headers.
#include <cstddef>
#include <string>

// Forward declarations.
namespace renderer  { class BaseGroup; }
class Bitmap;
class BitmapTex;
class Color;
class IParamMap2;
class Texmap;


//
// Parameter blocks functions.
//

void update_map_buttons(IParamMap2* param_map);

//
// Bitmap functions.
//

bool is_bitmap_texture(Texmap* map);

bool is_supported_procedural_texture(Texmap* map, const bool use_max_procedural_maps);

bool is_linear_texture(BitmapTex* bitmap_tex);

// Render a Max bitmap to a tiled 32-bit floating point RGBA appleseed image.
foundation::auto_release_ptr<foundation::Image> render_bitmap_to_image(
    Bitmap*                     bitmap,
    const size_t                image_width,
    const size_t                image_height,
    const size_t                tile_width,
    const size_t                tile_height);


//
// Project construction functions.
//

template <typename EntityContainer>
std::string make_unique_name(
    const EntityContainer&      entities,
    const std::string&          name);

void insert_color(
    renderer::BaseGroup&        base_group,
    const Color&                color,
    const char*                 name);

std::string insert_texture_and_instance(
    renderer::BaseGroup&        base_group,
    Texmap*                     texmap,
    const bool                  use_max_procedural_maps,
    const TimeValue             time,
    const renderer::ParamArray& texture_params = renderer::ParamArray(),
    const renderer::ParamArray& texture_instance_params = renderer::ParamArray());

std::string insert_bitmap_texture_and_instance(
    renderer::BaseGroup&        base_group,
    BitmapTex*                  bitmap_tex,
    renderer::ParamArray        texture_params = renderer::ParamArray(),
    renderer::ParamArray        texture_instance_params = renderer::ParamArray());

std::string insert_procedural_texture_and_instance(
    renderer::BaseGroup&        base_group,
    Texmap*                     texmap,
    const TimeValue             time,
    renderer::ParamArray        texture_params = renderer::ParamArray(),
    renderer::ParamArray        texture_instance_params = renderer::ParamArray());

//
// Version information functions.
//

extern void print_libraries_information();

extern void print_libraries_features();

extern const char* to_enabled_disabled(const bool value);

extern void print_third_party_libraries_information();

//
// Implementation.
//

template <typename EntityContainer>
std::string make_unique_name(
    const EntityContainer&      entities,
    const std::string&          name)
{
    return
        entities.get_by_name(name.c_str()) == nullptr
            ? name
            : asr::make_unique_name(name + "_", entities);
}

template <typename T>
const T load_ini_setting(const wchar_t* category, const wchar_t* key_name, const T& default_value)
{
    WStr filename;
    filename += GetCOREInterface()->GetDir(APP_PLUGCFG_DIR);
    filename += L"\\appleseed\\appleseed.ini";

    const DWORD BufferSize = 1024;
    wchar_t buf[BufferSize];
    GetPrivateProfileString(
        category,
        key_name,
        utf8_to_wide(foundation::to_string(default_value)).c_str(),
        buf,
        BufferSize,
        filename);

    const std::wstring result(buf);
    return foundation::from_string<T>(wide_to_utf8(result));
}

template <typename T>
const T load_system_setting(const wchar_t* key_name, const T& default_value)
{
    return load_ini_setting(L"System", key_name, default_value);
}

template <typename T>
void save_ini_setting(const wchar_t* category, const wchar_t* key_name, const T& value)
{
    MaxSDK::Util::Path filepath(GetCOREInterface()->GetDir(APP_PLUGCFG_DIR));
    filepath.Append(L"\\appleseed\\");
    if (!filepath.Exists())
        IPathConfigMgr::GetPathConfigMgr()->CreateDirectoryHierarchy(filepath);
    filepath.Append(L"appleseed.ini");

    WritePrivateProfileString(
        category,
        key_name,
        utf8_to_wide(foundation::to_string(value)).c_str(),
        filepath.GetCStr());
}

template <typename T>
void save_system_setting(const wchar_t* key_name, const T& value)
{
    save_ini_setting(L"System", key_name, value);
}
