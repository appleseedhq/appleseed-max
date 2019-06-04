
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
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
#include "appleseedoslplugin/osltexture.h"
#include "oslutils.h"
#include "utilities.h"
#include "_endmaxheaders.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// 3ds Max headers.
#include "_beginmaxheaders.h"
#include <imtl.h>
#include "_endmaxheaders.h"

namespace asf = foundation;
namespace asr = renderer;

void connect_output_selector(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const TimeValue     time)
{
    Texmap* input_map = nullptr;
    texmap->GetParamBlock(0)->GetValueByName(L"source_map", time, input_map, FOREVER);
    if (dynamic_cast<OSLTexture*>(input_map) != nullptr)
    {
        OSLTexture* osl_texture = static_cast<OSLTexture*>(input_map);
        auto output_names = osl_texture->get_output_names();

        int output_index = 0;
        texmap->GetParamBlock(0)->GetValueByName(L"output_index", time, output_index, FOREVER);
        DbgAssert(output_index >= 1);
        output_index -= 1;
        if (output_index < output_names.size())
        {
            osl_texture->create_osl_texture(
                shader_group,
                material_node_name,
                material_input_name,
                output_index);
        }
    }
}

void connect_output_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const Color         const_value,
    const TimeValue     time)
{
    auto color_balance_layer_name = foundation::format("{0}_{1}_color_balance", material_node_name, material_input_name);

    Texmap* input_map = nullptr;
    texmap->GetParamBlock(0)->GetValueByName(L"map1", time, input_map, FOREVER);

    connect_color_texture(
        shader_group,
        color_balance_layer_name.c_str(),
        "in_defaultColor",
        input_map,
        Color(1.0, 1.0, 1.0),
        time);

    renderer::ParamArray output_params = get_output_params(texmap, time)
        .insert("in_constantColor", fmt_osl_expr(to_color3f(const_value)));

    shader_group.add_shader("shader", "as_max_color_balance", color_balance_layer_name.c_str(), output_params);

    shader_group.add_connection(
        color_balance_layer_name.c_str(), "out_outColor",
        material_node_name, material_input_name);
}

void connect_output_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const float         const_value,
    const TimeValue     time)
{
    auto color_balance_layer_name = foundation::format("{0}_{1}_color_balance", material_node_name, material_input_name);

    Texmap* input_map = nullptr;
    texmap->GetParamBlock(0)->GetValueByName(L"map1", time, input_map, FOREVER);

    connect_float_texture(
        shader_group,
        color_balance_layer_name.c_str(),
        "in_defaultFloat",
        input_map,
        1.0f,
        time);

    renderer::ParamArray output_params = get_output_params(texmap, time)
        .insert("in_constantFloat", fmt_osl_expr(const_value));

    shader_group.add_shader("shader", "as_max_color_balance", color_balance_layer_name.c_str(), output_params);

    shader_group.add_connection(
        color_balance_layer_name.c_str(), "out_outAlpha",
        material_node_name, material_input_name);
}
