
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

#ifndef UTILITIES_H
#define UTILITIES_H

// appleseed.foundation headers.
#include "foundation/math/matrix.h"
#include "foundation/math/vector.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <matrix3.h>

// Standard headers.
#include <string>

inline foundation::Vector3d max_to_as(const Point3& p)
{
    return foundation::Vector3d(p.x, p.z, -p.y);
}

inline foundation::Matrix4d max_to_as(const Matrix3& input)
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


//
// String conversion functions.
//
// Reference:
//
//   http://stackoverflow.com/a/3999597/393756
//

// Convert a wide Unicode string to an UTF8 string.
std::string utf8_encode(const std::wstring& wstr);
std::string utf8_encode(const TCHAR* wstr);

// Convert an UTF8 string to a wide Unicode String.
std::wstring utf8_decode(const std::string& str);

#endif	// !UTILITIES_H
