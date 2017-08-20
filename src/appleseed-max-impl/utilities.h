
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

#pragma once

// appleseed.renderer headers.
#include "renderer/api/scene.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/color.h"
#include "foundation/math/matrix.h"
#include "foundation/math/vector.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <assert1.h>
#include <color.h>
#include <ioapi.h>
#include <iparamb2.h>
#include <matrix3.h>
#include <maxtypes.h>
#include <point3.h>
#include <point4.h>

// Standard headers.
#include <cstddef>
#include <string>

// Forward declarations.
namespace renderer  { class BaseGroup; }
class Interval;
class Texmap;


//
// Math functions.
//

Matrix3 transpose(const Matrix3& matrix);


//
// Type conversion functions.
//

// Convert a 3ds Max color to an appleseed one.
foundation::Color3f to_color3f(const Color& c);
foundation::Color3f to_color3f(const Point3& p);

// Convert a 3ds Max transformation matrix to an appleseed one.
foundation::Matrix4d to_matrix4d(const Matrix3& input);


//
// String conversion functions.
//
// Reference:
//
//   http://stackoverflow.com/a/3999597/393756
//

// Convert a wide Unicode string to an UTF-8 string.
std::string wide_to_utf8(const std::wstring& wstr);
std::string wide_to_utf8(const TCHAR* wstr);

// Convert an UTF-8 string to a wide Unicode string.
std::wstring utf8_to_wide(const std::string& str);
std::wstring utf8_to_wide(const char* str);


//
// Bitmap functions.
//

// Return true if a given map is a valid bitmap texture.
bool is_bitmap_texture(Texmap* map);


//
// I/O functions.
//

// Return the root path of the plugin, i.e. the path to the directory containing the plugin DLLs
// as an UTF-8 string, for instance: "C:\Program Files\Autodesk\3ds Max 2017\Plugins\appleseed"
std::string get_root_path();

// Write a block of data to a 3ds Max file. Return true on success.
bool write(ISave* isave, const void* data, const size_t size);

// Write a typed object to a 3ds Max file. Return true on success.
template <typename T>
bool write(ISave* isave, const T& object);

// Read a typed object from a 3ds Max file. Return true on success.
template <typename T>
IOResult read(ILoad* iload, T* object);


//
// Parameter blocks functions.
//

template <typename T>
BOOL GetParamBlockValueByName(
    IParamBlock2*           param_block,
    const MCHAR* const      param_name,
    const TimeValue         t,
    T&                      value,
    Interval&               validity,
    const int               tab_index = 0);


//
// Project construction functions.
//

template <typename EntityContainer>
std::string make_unique_name(
    const EntityContainer&  entities,
    const std::string&      name);

void insert_color(
    renderer::BaseGroup&    base_group,
    const Color&            color,
    const char*             name);

std::string insert_texture_and_instance(
    renderer::BaseGroup&    base_group,
    Texmap*                 texmap,
    renderer::ParamArray    texture_params = renderer::ParamArray(),
    renderer::ParamArray    texture_instance_params = renderer::ParamArray());


//
// Implementation.
//

inline Matrix3 transpose(const Matrix3& matrix)
{
    const Point4 c0 = matrix.GetColumn(0);
    const Point4 c1 = matrix.GetColumn(1);
    const Point4 c2 = matrix.GetColumn(2);

    return Matrix3(
        Point3(c0[0], c0[1], c0[2]),    // first row
        Point3(c1[0], c1[1], c1[2]),    // second row
        Point3(c2[0], c2[1], c2[2]),    // third row
        Point3(0.0f, 0.0f, 0.0f));      // fourth row
}

inline foundation::Color3f to_color3f(const Color& c)
{
    return foundation::Color3f(c.r, c.g, c.b);
}

inline foundation::Color3f to_color3f(const Point3& p)
{
    return foundation::Color3f(p.x, p.y, p.z);
}

inline foundation::Matrix4d to_matrix4d(const Matrix3& input)
{
    foundation::Matrix4d output;

    output(0, 0) =  input[0][0];
    output(1, 0) =  input[0][2];
    output(2, 0) = -input[0][1];
    output(3, 0) =  0.0;

    output(0, 1) =  input[1][0];
    output(1, 1) =  input[1][2];
    output(2, 1) = -input[1][1];
    output(3, 1) =  0.0;

    output(0, 2) =  input[2][0];
    output(1, 2) =  input[2][2];
    output(2, 2) = -input[2][1];
    output(3, 2) =  0.0;

    output(0, 3) =  input[3][0];
    output(1, 3) =  input[3][2];
    output(2, 3) = -input[3][1];
    output(3, 3) =  1.0;

    return output;
}

inline bool write(ISave* isave, const void* data, const size_t size)
{
    ULONG written;
    const IOResult result =
        isave->WriteVoid(data, static_cast<ULONG>(size), &written);
    return result == IO_OK && written == size;
}

template <typename T>
inline bool write(ISave* isave, const T& object)
{
    return write(isave, &object, sizeof(T));
}

template <>
inline bool write(ISave* isave, const bool& b)
{
    return write<BYTE>(isave, b ? 1 : 0);
}

template <>
inline bool write(ISave* isave, const MSTR& s)
{
    return isave->WriteWString(s) == IO_OK;
}

template <typename T>
inline IOResult read(ILoad* iload, T* object)
{
    ULONG read;
    const IOResult result =
        iload->ReadVoid(object, sizeof(T), &read);
    return read != sizeof(T) ? IO_ERROR : result;
}

template <>
inline IOResult read(ILoad* iload, bool* b)
{
    BYTE byte;
    const IOResult result = read<BYTE>(iload, &byte);
    *b = byte == 1;
    return result;
}

template <>
inline IOResult read(ILoad* iload, MSTR* s)
{
    MCHAR* buf;
    const IOResult result = iload->ReadWStringChunk(&buf);
    if (result == IO_OK)
        *s = buf;
    return result;
}

template <typename T>
BOOL GetParamBlockValueByName(
    IParamBlock2*           param_block,
    const MCHAR* const      param_name,
    const TimeValue         t,
    T&                      value,
    Interval&               validity,
    const int               tab_index)
{
    ParamBlockDesc2* const pb_desc = param_block->GetDesc();
    DbgAssert(pb_desc != nullptr);

    const int param_index = pb_desc->NameToIndex(param_name);
    DbgAssert(param_index >= 0);

    const ParamID param_id = pb_desc->IndextoID(param_index);
    DbgAssert(param_id >= 0);

    return param_block->GetValue(param_id, t, value, validity, tab_index);
}

template <typename EntityContainer>
std::string make_unique_name(
    const EntityContainer&  entities,
    const std::string&      name)
{
    return
        entities.get_by_name(name.c_str()) == nullptr
            ? name
            : asr::make_unique_name(name + "_", entities);
}
