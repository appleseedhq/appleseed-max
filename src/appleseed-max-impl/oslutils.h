
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017-2018 Francois Beaune, The appleseedhq Organization
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
#include "foundation/image/color.h"
#include "foundation/math/vector.h"

// 3ds Max Headers.
#include <maxtypes.h>

// Standard headers.
#include <string>

// Forward declarations.
namespace renderer { class Assembly; }
namespace renderer { class ParamArray; }
namespace renderer { class ShaderGroup; }
class Color;
class IParamBlock2;
class Mtl;
class OSLShaderInfo;
class Texmap;

renderer::ParamArray get_uv_params(Texmap* texmap);

renderer::ParamArray get_output_params(Texmap* texmap);

std::string fmt_osl_expr(const std::string& s);

std::string fmt_osl_expr(const int value);

std::string fmt_osl_expr(const float value);

std::string fmt_osl_expr(const foundation::Color3f& linear_rgb);

std::string fmt_osl_expr(const foundation::Vector3f& vector);

std::string fmt_osl_normal_expr(const foundation::Vector3f& normal);

std::string fmt_osl_point_expr(const foundation::Vector3f& point);

std::string fmt_osl_expr(Texmap* texmap);

void connect_float_texture(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const float             const_value);

void connect_color_texture(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_input_name,
    Texmap*                 texmap,
    const Color             const_color);

void connect_bump_map(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_normal_input_name,
    const char*             material_tn_input_name,
    Texmap*                 texmap,
    const float             amount);

void connect_normal_map(
    renderer::ShaderGroup&  shader_group,
    const char*             material_node_name,
    const char*             material_normal_input_name,
    const char*             material_tn_input_name,
    Texmap*                 texmap,
    const int               up_vector,
    const float             amount);

void connect_sub_mtl(
    renderer::Assembly&     assembly,
    renderer::ShaderGroup&  shader_group,
    const char*             shader_name,
    const char*             shader_input,
    Mtl*                    mat);

void create_osl_shader(
    renderer::Assembly*     assembly,
    renderer::ShaderGroup&  shader_group,
    const char*             layer_name,
    IParamBlock2*           param_block,
    const OSLShaderInfo*    shader_info);
