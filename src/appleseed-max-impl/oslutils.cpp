
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

// Interface header.
#include "oslutils.h"

// appleseed-max headers.
#include "appleseedoslplugin/oslshadermetadata.h"
#include "appleseedoslplugin/osltexture.h"
#include "builtinmapsupport.h"
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
#include <maxtypes.h>
#include <stdmat.h>
#include <iparamm2.h>

namespace asf = foundation;
namespace asr = renderer;

asr::ParamArray get_uv_params(Texmap* texmap, const TimeValue time)
{
    asr::ParamArray uv_params;

    if (texmap == nullptr)
        return uv_params;

    UVGen* uv_gen = texmap->GetTheUVGen();
    if (!uv_gen || !uv_gen->IsStdUVGen())
        return uv_params;

    StdUVGen* std_uv = static_cast<StdUVGen*>(uv_gen);

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
        const int use_clip = pblock->GetInt(bmtex_apply, time, FOREVER);
        if (use_clip)
        {
            const float clip_u = pblock->GetFloat(bmtex_clipu, time, FOREVER);
            const float clip_v = pblock->GetFloat(bmtex_clipv, time, FOREVER);

            const float clip_w = pblock->GetFloat(bmtex_clipw, time, FOREVER);
            const float clip_h = pblock->GetFloat(bmtex_cliph, time, FOREVER);

            const int crop_place = pblock->GetInt(bmtex_crop_place, time, FOREVER);

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


asr::ParamArray get_output_params(Texmap* texmap, const TimeValue time)
{
    asr::ParamArray output_params;
    output_params.insert("in_multiplier", fmt_osl_expr(1.0f));
    output_params.insert("in_clamp_output", fmt_osl_expr(0));
    output_params.insert("in_invert", fmt_osl_expr(0));
    output_params.insert("in_colorGain", fmt_osl_expr(foundation::Color3f(1.0f)));
    output_params.insert("in_colorOffset", fmt_osl_expr(foundation::Color3f(0.0f)));
    output_params.insert("in_alphaGain", fmt_osl_expr(1.0f));
    output_params.insert("in_alphaOffset", fmt_osl_expr(0.0f));
    output_params.insert("in_alphaIsLuminance", fmt_osl_expr(0));

    if (texmap == nullptr)
        return output_params;

    StdTexoutGen* std_tex_output = nullptr;
    for (int i = 0, e = texmap->NumRefs(); i < e; ++i)
    {
        ReferenceTarget* ref = texmap->GetReference(i);
        if (ref != nullptr && ref->SuperClassID() == TEXOUTPUT_CLASS_ID)
        {
            std_tex_output = dynamic_cast<StdTexoutGen*>(ref);
            if (std_tex_output != nullptr)
                break;
        }
    }
    if (std_tex_output == nullptr)
        return output_params;

    output_params.insert("in_multiplier", fmt_osl_expr(std_tex_output->GetOutAmt(time)));
    output_params.insert("in_clamp_output", fmt_osl_expr(std_tex_output->GetClamp()));
    output_params.insert("in_invert", fmt_osl_expr(std_tex_output->GetInvert()));
    output_params.insert("in_colorGain", fmt_osl_expr(foundation::Color3f(std_tex_output->GetRGBAmt(time))));
    output_params.insert("in_colorOffset", fmt_osl_expr(foundation::Color3f(std_tex_output->GetRGBOff(time))));
    output_params.insert("in_alphaGain", fmt_osl_expr(std_tex_output->GetRGBAmt(time)));
    output_params.insert("in_alphaOffset", fmt_osl_expr(std_tex_output->GetRGBOff(time)));
    output_params.insert("in_alphaIsLuminance", fmt_osl_expr(std_tex_output->GetAlphaFromRGB()));

    return output_params;
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

std::string fmt_osl_normal_expr(const asf::Vector3f& normal)
{
    return asf::format("normal {0} {1} {2}", normal.x, normal.y, normal.z);
}

std::string fmt_osl_point_expr(const asf::Vector3f& point)
{
    return asf::format("point {0} {1} {2}", point.x, point.y, point.z);
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
    const float         const_value,
    const TimeValue     time)
{
    if (is_supported_procedural_texture(texmap, false))
    {
        create_supported_texture(
            shader_group,
            material_node_name,
            material_input_name,
            texmap,
            const_value,
            time);
        return;
    }

    if (is_osl_texture(texmap))
    {
        OSLTexture* osl_tex = static_cast<OSLTexture*>(texmap);
        osl_tex->create_osl_texture(shader_group, material_node_name, material_input_name, time);
        return;
    }

    if (is_bitmap_texture(texmap))
    {
        const auto uv_transform_layer_name = asf::format("{0}_{1}_uv_transform", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap, time));

        const auto layer_name = asf::format("{0}_{1}_texture", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_float_texture", layer_name.c_str(),
            asr::ParamArray()
                .insert("Filename", fmt_osl_expr(texmap)));

        asr::ParamArray color_balance_params = get_output_params(texmap, time)
            .insert("in_constantFloat", fmt_osl_expr(const_value));

        const auto color_balance_layer_name = asf::format("{0}_{1}_color_balance", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_color_balance", color_balance_layer_name.c_str(), color_balance_params);

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_U",
            layer_name.c_str(), "U");

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_V",
            layer_name.c_str(), "V");

        shader_group.add_connection(
            layer_name.c_str(), "FloatOut",
            color_balance_layer_name.c_str(), "in_defaultFloat");

        shader_group.add_connection(
            color_balance_layer_name.c_str(), "out_outAlpha",
            material_node_name, material_input_name);
    }
}

void connect_color_texture(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const Color         const_color,
    const TimeValue     time)
{
    if (is_supported_procedural_texture(texmap, false))
    {
        create_supported_texture(
            shader_group,
            material_node_name,
            material_input_name,
            texmap,
            const_color,
            time);
        return;
    }

    if (is_osl_texture(texmap))
    {
        OSLTexture* osl_tex = static_cast<OSLTexture*>(texmap);
        osl_tex->create_osl_texture(shader_group, material_node_name, material_input_name, time);
        return;
    }
    
    if (is_bitmap_texture(texmap))
    {
        const auto uv_transform_layer_name = asf::format("{0}_{1}_uv_transform", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap, time));
        if (!is_linear_texture(static_cast<BitmapTex*>(texmap)))
        {
            const auto texture_layer_name = asf::format("{0}_{1}_texture", material_node_name, material_input_name);
            shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
                asr::ParamArray()
                    .insert("Filename", fmt_osl_expr(texmap)));

            const auto srgb_to_linear_layer_name = asf::format("{0}_{1}_srgb_to_linear", material_node_name, material_input_name);
            shader_group.add_shader("shader", "as_max_srgb_to_linear_rgb", srgb_to_linear_layer_name.c_str(),
                asr::ParamArray());

            asr::ParamArray color_balance_params = get_output_params(texmap, time)
                .insert("in_constantColor", fmt_osl_expr(to_color3f(const_color)));

            const auto color_balance_layer_name = asf::format("{0}_{1}_color_balance", material_node_name, material_input_name);
            shader_group.add_shader("shader", "as_max_color_balance", color_balance_layer_name.c_str(), color_balance_params);

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
                color_balance_layer_name.c_str(), "in_defaultColor");

            shader_group.add_connection(
                color_balance_layer_name.c_str(), "out_outColor",
                material_node_name, material_input_name);
        }
        else
        {
            const auto texture_layer_name = asf::format("{0}_{1}_texture", material_node_name, material_input_name);
            shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
                asr::ParamArray()
                    .insert("Filename", fmt_osl_expr(texmap)));

            asr::ParamArray color_balance_params = get_output_params(texmap, time)
                .insert("in_constantColor", fmt_osl_expr(to_color3f(const_color)));

            const auto color_balance_layer_name = asf::format("{0}_{1}_color_balance", material_node_name, material_input_name);
            shader_group.add_shader("shader", "as_max_color_balance", color_balance_layer_name.c_str(), color_balance_params);

            shader_group.add_connection(
                uv_transform_layer_name.c_str(), "out_U",
                texture_layer_name.c_str(), "U");

            shader_group.add_connection(
                uv_transform_layer_name.c_str(), "out_V",
                texture_layer_name.c_str(), "V");

            shader_group.add_connection(
                texture_layer_name.c_str(), "ColorOut",
                color_balance_layer_name.c_str(), "in_defaultColor");

            shader_group.add_connection(
                color_balance_layer_name.c_str(), "out_outColor",
                material_node_name, material_input_name);
        }
    }
}

void connect_bump_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_normal_input_name,
    const char*         material_tn_input_name,
    Texmap*             texmap,
    const float         amount,
    const TimeValue     time)
{
    if (is_supported_procedural_texture(texmap, false) || is_osl_texture(texmap))
    {
        auto bump_map_layer_name = asf::format("{0}_bump_map", material_node_name);

        connect_float_texture(
            shader_group,
            bump_map_layer_name.c_str(),
            "Height",
            texmap,
            1.0f,
            time);

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
        shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap, time));

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
    const int           up_vector,
    const float         amount,
    const TimeValue     time)
{
    if (is_supported_procedural_texture(texmap, false) || is_osl_texture(texmap))
    {
        auto normal_map_layer_name = asf::format("{0}_normal_map", material_node_name);

        connect_color_texture(
            shader_group,
            normal_map_layer_name.c_str(),
            "Color",
            texmap,
            Color(1.0f, 1.0f, 1.0f),
            time);

        shader_group.add_shader("shader", "as_max_normal_map", normal_map_layer_name.c_str(),
            asr::ParamArray()
                .insert("UpVector", fmt_osl_expr(up_vector == 0 ? "Green" : "Blue"))
                .insert("Amount", fmt_osl_expr(amount)));

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
        shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(texmap, time));

        auto texture_layer_name = asf::format("{0}_normal_map_texture", material_node_name);
        shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
            asr::ParamArray()
                .insert("Filename", fmt_osl_expr(texmap)));

        auto normal_map_layer_name = asf::format("{0}_normal_map", material_node_name);
        shader_group.add_shader("shader", "as_max_normal_map", normal_map_layer_name.c_str(),
            asr::ParamArray()
                .insert("UpVector", fmt_osl_expr(up_vector == 0 ? "Green" : "Blue"))
                .insert("Amount", fmt_osl_expr(amount)));

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
    Mtl*                    mat,
    const TimeValue         time)
{
    auto appleseed_mtl = static_cast<IAppleseedMtl*>(mat->GetInterface(IAppleseedMtl::interface_id()));
    if (!appleseed_mtl)
        return;

    std::string layer_name =
        make_unique_name(assembly.materials(), asf::format("{0}_{1}_sub_mat", shader_name, mat->GetName()));
    assembly.materials().insert(appleseed_mtl->create_material(
        assembly,
        layer_name.c_str(),
        false,
        time));

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

void create_osl_shader(
    renderer::Assembly*     assembly,
    asr::ShaderGroup&       shader_group,
    const char*             layer_name,
    IParamBlock2*           param_block,
    const OSLShaderInfo*    shader_info,
    const TimeValue         time)
{
    asr::ParamArray params;

    for (const auto& param_info : shader_info->m_params)
    {
        const MaxParam& max_param = param_info.m_max_param;
        Texmap* texmap = nullptr;
        if (max_param.m_connectable)
        {
            int texture_param_id = 
                max_param.m_has_constant ? max_param.m_max_param_id + 1 : max_param.m_max_param_id;
            param_block->GetValue(texture_param_id, time, texmap, FOREVER);

            if (texmap != nullptr)
            {
                switch (max_param.m_param_type)
                {
                  case MaxParam::Float:
                    {
                        const float constant_value = 
                            max_param.m_has_constant ? param_block->GetFloat(max_param.m_max_param_id, time, FOREVER) : 1.0f;
                        connect_float_texture(
                            shader_group,
                            layer_name,
                            max_param.m_osl_param_name.c_str(),
                            texmap,
                            constant_value,
                            time);
                    }
                    break;
                  case MaxParam::Color:
                    {
                        const Color constant_color = 
                            max_param.m_has_constant ? param_block->GetColor(max_param.m_max_param_id, time, FOREVER) : Color(1.0, 1.0, 1.0);
                        connect_color_texture(
                            shader_group,
                            layer_name,
                            max_param.m_osl_param_name.c_str(),
                            texmap,
                            constant_color,
                            time);
                    }
                    break;

                  case MaxParam::VectorParam:
                  case MaxParam::NormalParam:
                  case MaxParam::PointParam:
                    {
                        if (is_osl_texture(texmap))
                        {
                            OSLTexture* osl_tex = static_cast<OSLTexture*>(texmap);
                            osl_tex->create_osl_texture(
                                shader_group,
                                layer_name,
                                max_param.m_osl_param_name.c_str(),
                                time);
                        }
                    }
                    break;
                }
            }
        }

        if (max_param.m_connectable && max_param.m_param_type == MaxParam::Closure)
        {
            Mtl* material = nullptr;
            param_block->GetValue(max_param.m_max_param_id, time, material, FOREVER);
            if (material != nullptr && assembly != nullptr)
            {
                connect_sub_mtl(
                    *assembly,
                    shader_group,
                    layer_name,
                    max_param.m_osl_param_name.c_str(),
                    material,
                    time);
            }
        }

        if (!max_param.m_connectable || texmap == nullptr)
        {
            switch (max_param.m_param_type)
            {
              case MaxParam::Float:
                {
                    const float param_value = param_block->GetFloat(max_param.m_max_param_id, time, FOREVER);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(param_value));
                }
                break;

              case MaxParam::IntNumber:
              case MaxParam::IntCheckbox:
              case MaxParam::IntMapper:
                {
                    const int param_value = param_block->GetInt(max_param.m_max_param_id, time, FOREVER);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(param_value));
                }
                break;

              case MaxParam::Color:
                {
                    const auto param_value = param_block->GetColor(max_param.m_max_param_id, time);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(to_color3f(param_value)));
                }
                break;

              case MaxParam::VectorParam:
                {
                    const Point3 param_value = param_block->GetPoint3(max_param.m_max_param_id, time);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(to_vector3f(param_value)));
                }
                break;

              case MaxParam::NormalParam:
                {
                    const Point3 param_value = param_block->GetPoint3(max_param.m_max_param_id, time);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_normal_expr(to_vector3f(param_value)));
                }
                break;

              case MaxParam::PointParam:
                {
                    const Point3 param_value = param_block->GetPoint3(max_param.m_max_param_id, time);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_point_expr(to_vector3f(param_value)));
                }
                break;

              case MaxParam::StringPopup:
                {
                    std::vector<std::string> fields;
                    asf::tokenize(param_info.m_options, "|", fields);

                    const int param_value = param_block->GetInt(max_param.m_max_param_id, time, FOREVER);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(fields[param_value]));
                }
                break;

              case MaxParam::String:
                {
                    const wchar_t* str_value;
                    param_block->GetValue(max_param.m_max_param_id, time, str_value, FOREVER);
                    if (str_value != nullptr)
                        params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(wide_to_utf8(str_value)));
                }
                break;
            }
        }
    }

    shader_group.add_shader("shader", shader_info->m_shader_name.c_str(), layer_name, params);
}
