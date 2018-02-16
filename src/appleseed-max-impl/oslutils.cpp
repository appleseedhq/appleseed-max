
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
#include "appleseedosltexture\osltexture.h"
#include "iappleseedmtl.h"
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <bitmap.h>
#include <imtl.h>
#include <maxapi.h>
#include <stdmat.h>

namespace asf = foundation;
namespace asr = renderer;

asr::ParamArray get_uv_params(Texmap* texmap)
{
    asr::ParamArray uv_params;

    if (texmap == nullptr)
        return uv_params;

    UVGen* uv_gen = texmap->GetTheUVGen();
    if (!uv_gen || !uv_gen->IsStdUVGen())
        return uv_params;

    StdUVGen* std_uv = static_cast<StdUVGen*>(uv_gen);
    auto time = GetCOREInterface()->GetTime();

    DbgAssert(texmap->MapSlotType(texmap->GetMapChannel()) == MAPSLOT_TEXTURE);
    DbgAssert(static_cast<StdUVGen*>(uv_gen)->GetUVWSource() == UVWSRC_EXPLICIT);

    float u_tiling = std_uv->GetUScl(time);
    float v_tiling = std_uv->GetVScl(time);
    float u_offset = std_uv->GetUOffs(time);
    float v_offset = std_uv->GetVOffs(time);
    float w_rotation = std_uv->GetWAng(time);
    int tiling = std_uv->GetTextureTiling();

    if (tiling & U_WRAP)
        uv_params.insert("in_wrapU", fmt_osl_expr(1));
    else if (tiling & U_MIRROR)
        uv_params.insert("in_mirrorU", fmt_osl_expr(1));

    if (tiling & V_WRAP)
        uv_params.insert("in_wrapV", fmt_osl_expr(1));
    else if (tiling & V_MIRROR)
        uv_params.insert("in_mirrorV", fmt_osl_expr(1));

    uv_params.insert("in_offsetU", fmt_osl_expr(u_offset));
    uv_params.insert("in_offsetV", fmt_osl_expr(v_offset));

    uv_params.insert("in_tilingU", fmt_osl_expr(u_tiling));
    uv_params.insert("in_tilingV", fmt_osl_expr(v_tiling));

    uv_params.insert("in_rotateW", fmt_osl_expr(asf::rad_to_deg(w_rotation)));

    // Access BMTex parameters through parameter block.
    enum
    {
        bmtex_params,
        bmtex_time
    };

    enum
    {
        bmtex_clipu,
        bmtex_clipv,
        bmtex_clipw,
        bmtex_cliph,
        bmtex_jitter,
        bmtex_usejitter,
        bmtex_apply,
        bmtex_crop_place
    };

    auto pblock = texmap->GetParamBlock(bmtex_params);
    if (pblock)
    {
        const int use_clip = pblock->GetInt(bmtex_apply, time);
        if (use_clip)
        {
            const float clip_u = pblock->GetFloat(bmtex_clipu, time);
            const float clip_v = pblock->GetFloat(bmtex_clipv, time);

            const float clip_w = pblock->GetFloat(bmtex_clipw, time);
            const float clip_h = pblock->GetFloat(bmtex_cliph, time);

            const int crop_place = pblock->GetInt(bmtex_crop_place, time);

            uv_params.insert("in_cropU", fmt_osl_expr(clip_u));
            uv_params.insert("in_cropV", fmt_osl_expr(clip_v));

            uv_params.insert("in_cropW", fmt_osl_expr(clip_w));
            uv_params.insert("in_cropH", fmt_osl_expr(clip_h));

            if (crop_place)
                uv_params.insert("in_crop_mode", fmt_osl_expr("place"));
            else
                uv_params.insert("in_crop_mode", fmt_osl_expr("crop"));
        }
        else
            uv_params.insert("in_crop_mode", fmt_osl_expr("off"));
    }

    return uv_params;
}

std::string fmt_osl_expr(const std::string& s)
{
    return asf::format("string {0}", s);
}

std::string fmt_osl_expr(const int value)
{
    return asf::format("int {0}", value);
}

std::string fmt_osl_expr(const float value)
{
    return asf::format("float {0}", value);
}

std::string fmt_osl_expr(const asf::Color3f& linear_rgb)
{
    return asf::format("color {0} {1} {2}", linear_rgb.r, linear_rgb.g, linear_rgb.b);
}

std::string fmt_osl_expr(const asf::Vector3f& vector)
{
    return asf::format("vector {0} {1} {2}", vector.x, vector.y, vector.z);
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
    if (is_osl_texture(texmap))
    {
        OSLTexture* osl_tex = static_cast<OSLTexture*>(texmap);
        osl_tex->create_osl_texture(shader_group, material_node_name, material_input_name);
        return;
    }

    auto uv_transform_layer_name = asf::format("{0}_{1}_uv_transform", material_node_name, material_input_name);
    shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap));

    auto layer_name = asf::format("{0}_{1}", material_node_name, material_input_name);
    shader_group.add_shader("shader", "as_max_float_texture", layer_name.c_str(),
        asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap))
            .insert("Multiplier", fmt_osl_expr(multiplier)));

    shader_group.add_connection(
        uv_transform_layer_name.c_str(), "out_U",
        layer_name.c_str(), "U");

    shader_group.add_connection(
        uv_transform_layer_name.c_str(), "out_V",
        layer_name.c_str(), "V");

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
    if (is_osl_texture(texmap))
    {
        OSLTexture* osl_tex = static_cast<OSLTexture*>(texmap);
        osl_tex->create_osl_texture(shader_group, material_node_name, material_input_name);
        return;
    }
    
    auto uv_transform_layer_name = asf::format("{0}_{1}_uv_transform", material_node_name, material_input_name);
    shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap));

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
            uv_transform_layer_name.c_str(), "out_U",
            texture_layer_name.c_str(), "U");

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_V",
            texture_layer_name.c_str(), "V");

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
            uv_transform_layer_name.c_str(), "out_U",
            texture_layer_name.c_str(), "U");

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_V",
            texture_layer_name.c_str(), "V");

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
    if (is_osl_texture(texmap))
    {
        auto bump_map_layer_name = asf::format("{0}_bump_map", material_node_name);

        connect_float_texture(
            shader_group,
            bump_map_layer_name.c_str(),
            "Height",
            texmap,
            1.0f);

        shader_group.add_shader("shader", "as_max_bump_map", bump_map_layer_name.c_str(),
            asr::ParamArray()
            .insert("Amount", fmt_osl_expr(amount)));

        shader_group.add_connection(
            bump_map_layer_name.c_str(), "NormalOut",
            material_node_name, material_normal_input_name);

        return;
    }

    if (is_bitmap_texture(texmap))
    {
        auto uv_transform_layer_name = asf::format("{0}_bump_uv_transform", material_node_name);
        shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap));

        auto texture_layer_name = asf::format("{0}_bump_map_texture", material_node_name);
        shader_group.add_shader("shader", "as_max_float_texture", texture_layer_name.c_str(),
            asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap)));

        auto bump_map_layer_name = asf::format("{0}_bump_map", material_node_name);
        shader_group.add_shader("shader", "as_max_bump_map", bump_map_layer_name.c_str(),
            asr::ParamArray()
            .insert("Amount", fmt_osl_expr(amount)));

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_U",
            texture_layer_name.c_str(), "U");

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_V",
            texture_layer_name.c_str(), "V");

        shader_group.add_connection(
            texture_layer_name.c_str(), "FloatOut",
            bump_map_layer_name.c_str(), "Height");
        shader_group.add_connection(
            bump_map_layer_name.c_str(), "NormalOut",
            material_node_name, material_normal_input_name);
    }
}

void connect_normal_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_normal_input_name,
    const char*         material_tn_input_name,
    Texmap*             texmap,
    const int           up_vector)
{
    if (is_osl_texture(texmap))
    {
        auto normal_map_layer_name = asf::format("{0}_normal_map", material_node_name);

        connect_color_texture(
            shader_group,
            normal_map_layer_name.c_str(),
            "Color",
            texmap,
            Color(1.0f, 1.0f, 1.0f));

        shader_group.add_shader("shader", "as_max_normal_map", normal_map_layer_name.c_str(),
            asr::ParamArray()
            .insert("UpVector", fmt_osl_expr(up_vector == 0 ? "Green" : "Blue")));

        shader_group.add_connection(
            normal_map_layer_name.c_str(), "NormalOut",
            material_node_name, material_normal_input_name);
        shader_group.add_connection(
            normal_map_layer_name.c_str(), "TangentOut",
            material_node_name, material_tn_input_name);

        return;
    }

    if (is_bitmap_texture(texmap))
    {
        auto uv_transform_layer_name = asf::format("{0}_bump_uv_transform", material_node_name);
        shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap));

        auto texture_layer_name = asf::format("{0}_normal_map_texture", material_node_name);
        shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
            asr::ParamArray()
                .insert("Filename", fmt_osl_expr(texmap)));

        auto normal_map_layer_name = asf::format("{0}_normal_map", material_node_name);
        shader_group.add_shader("shader", "as_max_normal_map", normal_map_layer_name.c_str(),
            asr::ParamArray()
                .insert("UpVector", fmt_osl_expr(up_vector == 0 ? "Green" : "Blue")));

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_U",
            texture_layer_name.c_str(), "U");

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_V",
            texture_layer_name.c_str(), "V");

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
}

void connect_sub_mtl(
    asr::Assembly&          assembly,
    asr::ShaderGroup&       shader_group,
    const char*             shader_name,
    const char*             shader_input,
    Mtl*                    mat)
{
    auto appleseed_mtl = static_cast<IAppleseedMtl*>(mat->GetInterface(IAppleseedMtl::interface_id()));
    if (!appleseed_mtl)
        return;

    std::string layer_name =
        make_unique_name(assembly.materials(), asf::format("{0}_{1}_sub_mat", shader_name, mat->GetName()));
    assembly.materials().insert(appleseed_mtl->create_material(assembly, layer_name.c_str(), false));

    asr::Material* layer_material = assembly.materials().get_by_name(layer_name.c_str());
    if (!layer_material->get_parameters().exist_path("osl_surface"))
        return;

    auto shader_group_name = layer_material->get_parameters().get("osl_surface");
    asr::ShaderGroup* mtl_group = assembly.shader_groups().get_by_name(shader_group_name);

    // Don't copy last shader and last connection
    for (auto shader = mtl_group->shaders().begin(); shader != --(mtl_group->shaders().end()); shader++)
    {
        shader_group.add_shader(shader->get_type(), shader->get_shader(), shader->get_layer(), shader->get_parameters());
    }

    for (auto conn = mtl_group->shader_connections().begin(); conn != --(mtl_group->shader_connections().end()); conn++)
    {
        shader_group.add_connection(conn->get_src_layer(), conn->get_src_param(), conn->get_dst_layer(), conn->get_dst_param());
    }

    auto last_conn = mtl_group->shader_connections().get_by_index(mtl_group->shader_connections().size() - 1);
    shader_group.add_connection(layer_name.c_str(), last_conn->get_src_param(), shader_name, shader_input);
}
