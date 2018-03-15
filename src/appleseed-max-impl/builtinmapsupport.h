
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2018 Sergo Pogosyan, The appleseedhq Organization
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

// appleseed-max headers.
#include "osloutputselectormap/osloutputselector.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/image/color.h"

// 3ds Max Headers.
#include <color.h>
#include <maxtypes.h>

// Standard headers.
#include <string>

// Forward declarations.
namespace renderer { class ShaderGroup; }
class Texmap;

void connect_output_map(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const Color             const_value);

void connect_output_map(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const float             const_value);

void connect_output_selector(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const Color             const_value);

void connect_output_selector(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const float             const_value);

template <typename T>
void create_supported_texture(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const T                 const_value)
{
    auto part_a = texmap->ClassID().PartA();
    auto part_b = texmap->ClassID().PartB();

    if (texmap->ClassID() == OSLOutputSelector::get_class_id())
    {
        connect_output_selector(
            shader_group,
            material_node_name,
            material_input_name,
            texmap,
            const_value);
        return;
    }

    switch (part_a)
    {
      case OUTPUT_CLASS_ID:
        connect_output_map(
            shader_group,
            material_node_name,
            material_input_name,
            texmap,
            const_value);

      default:
        break;
    }
}
