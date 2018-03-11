
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

// Interface header.
#include "builtinmapsupport.h"

// appleseed-max headers.
#include "oslutils.h"
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// 3ds Max Headers.
#include <imtl.h>
#include <maxtypes.h>

namespace asf = foundation;
namespace asr = renderer;

void connect_output_map(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const Color             const_value)
{
    const auto t = GetCOREInterface()->GetTime();
    auto color_balance_layer_name = foundation::format("{0}_{1}_color_balance", material_node_name, material_input_name);

    Texmap* input_map = nullptr;
    get_paramblock_value_by_name(texmap->GetParamBlock(0), L"map1", t, input_map, FOREVER);

    connect_color_texture(shader_group, color_balance_layer_name.c_str(), "in_defaultColor", input_map, Color(1.0, 1.0, 1.0));

    renderer::ParamArray output_params = get_output_params(texmap)
        .insert("in_constantColor", fmt_osl_expr(to_color3f(const_value)));

    shader_group.add_shader("shader", "as_max_color_balance", color_balance_layer_name.c_str(), output_params);

    shader_group.add_connection(
        color_balance_layer_name.c_str(), "out_outColor",
        material_node_name, material_input_name);
}

void connect_output_map(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const float             const_value)
{
    const auto t = GetCOREInterface()->GetTime();
    auto color_balance_layer_name = foundation::format("{0}_{1}_color_balance", material_node_name, material_input_name);

    Texmap* input_map = nullptr;
    get_paramblock_value_by_name(texmap->GetParamBlock(0), L"map1", t, input_map, FOREVER);

    connect_float_texture(shader_group, color_balance_layer_name.c_str(), "in_defaultFloat", input_map, 1.0f);

    renderer::ParamArray output_params = get_output_params(texmap)
        .insert("in_constantFloat", fmt_osl_expr(const_value));

    shader_group.add_shader("shader", "as_max_color_balance", color_balance_layer_name.c_str(), output_params);

    shader_group.add_connection(
        color_balance_layer_name.c_str(), "out_outAlpha",
        material_node_name, material_input_name);
}
