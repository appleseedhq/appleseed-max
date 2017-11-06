
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017 Francois Beaune, The appleseedhq Organization
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
#include "oslutils.h"

// appleseed-max headers.
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <bitmap.h>
#include <stdmat.h>

namespace asf = foundation;
namespace asr = renderer;

std::string fmt_osl_expr(const std::string& s)
{
    return asf::format("string {0}", s);
}

std::string fmt_osl_expr(const float value)
{
    return asf::format("float {0}", value);
}

std::string fmt_osl_expr(const asf::Color3f& linear_rgb)
{
    return asf::format("color {0} {1} {2}", linear_rgb.r, linear_rgb.g, linear_rgb.b);
}

std::string fmt_osl_expr(Texmap* texmap)
{
    if (is_bitmap_texture(texmap))
    {
        const auto texture_filepath = wide_to_utf8(static_cast<BitmapTex*>(texmap)->GetMap().GetFullFilePath());
        return fmt_osl_expr(texture_filepath);
    }
    else return fmt_osl_expr(std::string());
}

void connect_float_texture(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const float         multiplier)
{
    auto layer_name = asf::format("{0}_{1}", material_node_name, material_input_name);

    shader_group.add_shader("shader", "as_max_float_texture", layer_name.c_str(),
        asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap))
            .insert("Multiplier", fmt_osl_expr(multiplier)));

    shader_group.add_connection(
        layer_name.c_str(), "FloatOut",
        material_node_name, material_input_name);
}

void connect_color_texture(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const Color         multiplier)
{
    if (is_bitmap_texture(texmap) && !is_linear_texture(static_cast<BitmapTex*>(texmap)))
    {
        auto texture_layer_name = asf::format("{0}_{1}_texture", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
            asr::ParamArray()
                .insert("Filename", fmt_osl_expr(texmap))
                .insert("Multiplier", fmt_osl_expr(to_color3f(multiplier))));

        auto srgb_to_linear_layer_name = asf::format("{0}_{1}_srgb_to_linear", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_srgb_to_linear_rgb", srgb_to_linear_layer_name.c_str(),
            asr::ParamArray());

        shader_group.add_connection(
            texture_layer_name.c_str(), "ColorOut",
            srgb_to_linear_layer_name.c_str(), "ColorIn");

        shader_group.add_connection(
            srgb_to_linear_layer_name.c_str(), "ColorOut",
            material_node_name, material_input_name);
    }
    else
    {
        auto texture_layer_name = asf::format("{0}_{1}_texture", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
            asr::ParamArray()
                .insert("Filename", fmt_osl_expr(texmap))
                .insert("Multiplier", fmt_osl_expr(to_color3f(multiplier))));

        shader_group.add_connection(
            texture_layer_name.c_str(), "ColorOut",
            material_node_name, material_input_name);
    }
}

void connect_bump_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_normal_input_name,
    const char*         material_tn_input_name,
    Texmap*             texmap,
    const float         amount)
{
    auto texture_layer_name = asf::format("{0}_bump_map_texture", material_node_name);
    shader_group.add_shader("shader", "as_max_float_texture", texture_layer_name.c_str(),
        asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap)));

    auto bump_map_layer_name = asf::format("{0}_bump_map", material_node_name);
    shader_group.add_shader("shader", "as_max_bump_map", bump_map_layer_name.c_str(),
        asr::ParamArray()
            .insert("Amount", fmt_osl_expr(amount)));

    shader_group.add_connection(
        texture_layer_name.c_str(), "FloatOut",
        bump_map_layer_name.c_str(), "Height");
    shader_group.add_connection(
        bump_map_layer_name.c_str(), "NormalOut",
        material_node_name, material_normal_input_name);
}

void connect_normal_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_normal_input_name,
    const char*         material_tn_input_name,
    Texmap*             texmap,
    const int           up_vector)
{
    auto texture_layer_name = asf::format("{0}_normal_map_texture", material_node_name);
    shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
        asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap)));

    auto normal_map_layer_name = asf::format("{0}_normal_map", material_node_name);
    shader_group.add_shader("shader", "as_max_normal_map", normal_map_layer_name.c_str(),
        asr::ParamArray()
            .insert("UpVector", fmt_osl_expr(up_vector == 0 ? "Green" : "Blue")));

    shader_group.add_connection(
        texture_layer_name.c_str(), "ColorOut",
        normal_map_layer_name.c_str(), "Color");
    shader_group.add_connection(
        normal_map_layer_name.c_str(), "NormalOut",
        material_node_name, material_normal_input_name);
    shader_group.add_connection(
        normal_map_layer_name.c_str(), "TangentOut",
        material_node_name, material_tn_input_name);
}
