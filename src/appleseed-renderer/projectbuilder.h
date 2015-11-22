
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

#ifndef PROJECTBUILDER_H
#define PROJECTBUILDER_H

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include <maxtypes.h>

// Forward declarations.
namespace renderer  { class Project; }
class Bitmap;
class DefaultLight;
class MaxSceneEntities;
class ViewParams;

// Build an appleseed project from the current 3ds Max scene.
foundation::auto_release_ptr<renderer::Project> build_project(
    const MaxSceneEntities& entities,
    DefaultLight*           default_lights,
    const int               default_light_count,
    const ViewParams&       view_params,
    Bitmap*                 bitmap,
    const TimeValue         time);

#endif	// !PROJECTBUILDER_H
