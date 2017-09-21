
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

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include <baseinterface.h>

// Forward declarations.
namespace renderer  { class Assembly; }
namespace renderer  { class Material; }
class Interface_ID;

class IAppleseedMtl
  : public BaseInterface
{
  public:
    static Interface_ID interface_id();

    virtual Interface_ID GetID() override;

    // The sides (front and/or back) on which this material should be applied.
    // The return value is a combination of renderer::ObjectInstance::Side values.
    virtual int get_sides() const = 0;

    // Can this material emit light, even if this particular instance does not
    // (for instance because its emission is set to 0)?
    virtual bool can_emit_light() const = 0;

    virtual foundation::auto_release_ptr<renderer::Material> create_material(
        renderer::Assembly& assembly,
        const char*         name,
        bool                use_max_source) = 0;
};
