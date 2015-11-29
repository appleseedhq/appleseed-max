
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

#pragma once

// appleseed.foundation headers.
#include "foundation/image/color.h"
#include "foundation/math/matrix.h"
#include "foundation/math/vector.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <color.h>
#include <ioapi.h>
#include <matrix3.h>
#include <maxtypes.h>
#include <point3.h>

// Standard headers.
#include <cstddef>
#include <string>


//
// Type conversion functions.
//

// Convert a 3ds Max color to an appleseed one.
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
std::string utf8_encode(const std::wstring& wstr);
std::string utf8_encode(const TCHAR* wstr);

// Convert an UTF-8 string to a wide Unicode String.
std::wstring utf8_decode(const std::string& str);


//
// I/O functions.
//

// Write a block of data to a 3ds Max file. Return true on success.
bool write(ISave* isave, const void* data, const size_t size);

// Write a typed object to a 3ds Max file. Return true on success.
template <typename T>
bool write(ISave* isave, const T& object);

// Read a typed object from a 3ds Max file. Return true on success.
template <typename T>
IOResult read(ILoad* iload, T* object);


//
// Implementation.
//

inline foundation::Color3f to_color3f(const Point3& p)
{
    return foundation::Color3f(p.x, p.y, p.z);
}

inline foundation::Color3f to_color3f(const Color& c)
{
    return foundation::Color3f(c.r, c.g, c.b);
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
