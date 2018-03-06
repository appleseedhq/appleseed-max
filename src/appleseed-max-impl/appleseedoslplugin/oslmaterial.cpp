
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
#include "oslmaterial.h"

// appleseed-max headers.
#include "appleseedoslplugin/oslparamdlg.h"
#include "appleseedoslplugin/oslshadermetadata.h"
#include "appleseedoslplugin/osltexture.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "bump/bumpparammapdlgproc.h"
#include "bump/resource.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/material.h"
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// 3ds Max headers.
#include <iparamm2.h>

namespace asf = foundation;
namespace asr = renderer;

//
// Changing the values of these constants WILL break compatibility
// with 3ds Max files saved with older versions of the plugin.
//

const USHORT ChunkFileFormatVersion = 0x0001;

const USHORT ChunkMtlBase = 0x1000;

namespace
{
    const wchar_t* GenericOSLTextureFriendlyClassName = L"appleseed OSL texture";
}


//
// OSLMaterial class implementation.
//

Class_ID OSLMaterial::get_class_id()
{
    return m_classid;
}

OSLMaterial::OSLMaterial(Class_ID class_id, OSLPluginClassDesc* class_desc)
  : m_pblock(nullptr)
  , m_bump_pblock(nullptr)
  , m_classid(class_id)
  , m_class_desc(class_desc)
  , m_shader_info(nullptr)
{
    m_shader_info = class_desc->m_shader_info;
    m_texture_id_map = m_shader_info->m_texture_id_map;
    m_submaterial_map = m_shader_info->m_material_id_map;
    
    m_has_bump_params = m_shader_info->find_maya_attribute("normalCamera") != nullptr;
    m_has_normal_params = m_shader_info->find_param("Tn") != nullptr;

    class_desc->MakeAutoParamBlocks(this);
    Reset();
}

BaseInterface* OSLMaterial::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void OSLMaterial::DeleteThis()
{
    delete this;
}

void OSLMaterial::GetClassName(TSTR& s)
{
    s = m_class_desc->ClassName();
}

SClass_ID OSLMaterial::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID OSLMaterial::ClassID()
{
    return get_class_id();
}

int OSLMaterial::NumSubs()
{
    return 1;
}

Animatable* OSLMaterial::SubAnim(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

TSTR OSLMaterial::SubAnimName(int i)
{
    return i == 0 ? L"Parameters" : nullptr;
}

int OSLMaterial::SubNumToRefNum(int subNum)
{
    return subNum;
}

int OSLMaterial::NumParamBlocks()
{
    return m_has_bump_params ? 2 : 1;
}

IParamBlock2* OSLMaterial::GetParamBlock(int i)
{
    if (i == 0)
        return m_pblock;
    else if (i == 1)
        return m_bump_pblock;
    else
        return nullptr;
}

IParamBlock2* OSLMaterial::GetParamBlockByID(BlockID id)
{
    if (id == m_pblock->ID())
        return m_pblock;
    else if (id == m_bump_pblock->ID())
        return m_bump_pblock;
    else
        return nullptr;
}

int OSLMaterial::NumRefs()
{
    return m_has_bump_params ? 2 : 1;
}

RefTargetHandle OSLMaterial::GetReference(int i)
{
    if (i == 0)
        return m_pblock;
    else if (i == 1)
        return m_bump_pblock;
    else
        return nullptr;
}

void OSLMaterial::SetReference(int i, RefTargetHandle rt_arg)
{
    if (i == 0)
        m_pblock = static_cast<IParamBlock2*>(rt_arg);
    else if (i == 1)
        m_bump_pblock = static_cast<IParamBlock2*>(rt_arg);
}

RefResult OSLMaterial::NotifyRefChanged(
    const Interval&   /*changeInt*/,
    RefTargetHandle   hTarget,
    PartID&           partID,
    RefMessage        message,
    BOOL              /*propagate*/)
{
    switch (message)
    {
      case REFMSG_TARGET_DELETED:
        if (hTarget == m_pblock)
            m_pblock = nullptr;
        break;

      case REFMSG_CHANGE:
          if (hTarget == m_pblock)
              m_class_desc->GetParamBlockDesc(0)->InvalidateUI(m_pblock->LastNotifyParamID());
          else if (hTarget == m_bump_pblock)
              m_class_desc->GetParamBlockDesc(1)->InvalidateUI(m_bump_pblock->LastNotifyParamID());
        break;
    }

    return REF_SUCCEED;
}

RefTargetHandle OSLMaterial::Clone(RemapDir& remap)
{
    OSLMaterial* mnew = new OSLMaterial(m_classid, m_class_desc);
    *static_cast<MtlBase*>(mnew) = *static_cast<MtlBase*>(this);
    BaseClone(this, mnew, remap);

    mnew->ReplaceReference(0, remap.CloneRef(m_pblock));
    if (m_has_bump_params)
        mnew->ReplaceReference(1, remap.CloneRef(m_bump_pblock));

    return (RefTargetHandle)mnew;
}

int OSLMaterial::NumSubTexmaps()
{
    return static_cast<int>(m_texture_id_map.size());
}

Texmap* OSLMaterial::GetSubTexmap(int i)
{
    Texmap* tex_map = nullptr;
    Interval iv;

    if (i < m_texture_id_map.size())
    {
        if (m_has_bump_params && i == m_texture_id_map.size() - 1)
            m_bump_pblock->GetValue(m_texture_id_map[i].first, 0, tex_map, iv);
        else
            m_pblock->GetValue(m_texture_id_map[i].first, 0, tex_map, iv);
    }

    return tex_map;
}

void OSLMaterial::SetSubTexmap(int i, Texmap* tex_map)
{
    if (i < m_texture_id_map.size())
    {
        const int param_id = m_texture_id_map[i].first;
        if (m_has_bump_params && i == m_texture_id_map.size() - 1)
        {
            m_bump_pblock->SetValue(param_id, 0, tex_map);
            m_class_desc->GetParamBlockDesc(1)->InvalidateUI(param_id);
        }
        else
        {
            m_pblock->SetValue(param_id, 0, tex_map);
            if (m_pblock->GetParameterType(param_id - 1) == TYPE_POINT3)
            {
                IParamMap2* map = m_pblock->GetMap(0);
                if (map != nullptr)
                    map->SetText(param_id, tex_map == nullptr ? L"" : L"M");
            }
                
            m_class_desc->GetParamBlockDesc(0)->InvalidateUI(param_id);
        }

    }
}

int OSLMaterial::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

TSTR OSLMaterial::GetSubTexmapSlotName(int i)
{
    if (i < m_texture_id_map.size())
        return m_texture_id_map[i].second.c_str();
    return L"";
}

void OSLMaterial::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }
    m_params_validity.SetInfinite();
    
    for (const auto& tex_param : m_texture_id_map)
    {
        Texmap* tex_map = nullptr;
        if (m_has_bump_params && tex_param == m_texture_id_map.back())
            m_bump_pblock->GetValue(tex_param.first, t, tex_map, valid);
        else
            m_pblock->GetValue(tex_param.first, t, tex_map, valid);

        if (tex_map)
            tex_map->Update(t, valid);
    }

    valid = m_params_validity;
}

void OSLMaterial::Reset()
{
    m_params_validity.SetEmpty();
}

Interval OSLMaterial::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* OSLMaterial::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    ParamDlg* master_dlg = new OSLParamDlg(hwMtlEdit, imp, this, m_shader_info);
    return master_dlg;
}

IOResult OSLMaterial::Save(ISave* isave)
{
    bool success = true;

    isave->BeginChunk(ChunkFileFormatVersion);
    success &= write(isave, &FileFormatVersion, sizeof(FileFormatVersion));
    isave->EndChunk();

    isave->BeginChunk(ChunkMtlBase);
    success &= MtlBase::Save(isave) == IO_OK;
    isave->EndChunk();

    return success ? IO_OK : IO_ERROR;
}

IOResult OSLMaterial::Load(ILoad* iload)
{
    IOResult result = IO_OK;

    while (true)
    {
        result = iload->OpenChunk();
        if (result == IO_END)
            return IO_OK;
        if (result != IO_OK)
            break;

        switch (iload->CurChunkID())
        {
          case ChunkFileFormatVersion:
            {
                USHORT version;
                result = read<USHORT>(iload, &version);
            }
            break;

          case ChunkMtlBase:
            result = MtlBase::Load(iload);
            break;
        }

        if (result != IO_OK)
            break;

        result = iload->CloseChunk();
        if (result != IO_OK)
            break;
    }

    return result;
}


Color OSLMaterial::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color OSLMaterial::GetDiffuse(int mtlNum, BOOL backFace)
{
    Mtl* mtl = nullptr;
    Interval iv;
    if (!m_submaterial_map.empty())
        m_pblock->GetValue(m_submaterial_map[0].first, 0, mtl, iv);

    return mtl != nullptr ? mtl->GetDiffuse() : get_viewport_diffuse_color();
}

Color OSLMaterial::GetSpecular(int mtlNum, BOOL backFace)
{
    return get_viewport_specular_color();
}

float OSLMaterial::GetShininess(int mtlNum, BOOL backFace)
{
    return get_viewport_shininness_spread();
}

float OSLMaterial::GetShinStr(int mtlNum, BOOL backFace)
{
    return get_viewport_shininess_amount();
}

float OSLMaterial::GetXParency(int mtlNum, BOOL backFace)
{
    const float transparency = get_viewport_transparency_amount();
    return transparency > 0.9f ? 0.9f : transparency;
}

void OSLMaterial::SetAmbient(Color c, TimeValue t)
{
}

void OSLMaterial::SetDiffuse(Color c, TimeValue t)
{
}

void OSLMaterial::SetSpecular(Color c, TimeValue t)
{
}

void OSLMaterial::SetShininess(float v, TimeValue t)
{
}

void OSLMaterial::Shade(ShadeContext& sc)
{
}

int OSLMaterial::NumSubMtls()
{
    return static_cast<int>(m_submaterial_map.size());
}

Mtl* OSLMaterial::GetSubMtl(int i)
{
    Mtl* mtl = nullptr;
    Interval iv;
    if (i < m_submaterial_map.size())
        m_pblock->GetValue(m_submaterial_map[i].first, 0, mtl, iv);

    return mtl;
}

void OSLMaterial::SetSubMtl(int i, Mtl* m)
{
    if (i < m_submaterial_map.size())
    {
        const auto param_id = m_submaterial_map[i].first;
        m_pblock->SetValue(param_id, 0, m);
        m_class_desc->GetParamBlockDesc(0)->InvalidateUI(param_id);
    }
}

MSTR OSLMaterial::GetSubMtlSlotName(int i)
{
    if (i < m_submaterial_map.size())
        return m_submaterial_map[i].second.c_str();
    return L"";
}

int OSLMaterial::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool OSLMaterial::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> OSLMaterial::create_material(
    asr::Assembly&  assembly,
    const char*     name,
    const bool      use_max_procedural_maps)
{
    return use_max_procedural_maps
        ? create_builtin_material(assembly, name)
        : create_osl_material(assembly, name);
}

asf::auto_release_ptr<asr::Material> OSLMaterial::create_osl_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    //
    // Shader group.
    //
    const auto t = GetCOREInterface()->GetTime();
    asr::ParamArray params;

    auto shader_group_name = make_unique_name(assembly.shader_groups(), std::string(name) + "_shader_group");
    auto shader_group = asr::ShaderGroupFactory::create(shader_group_name.c_str());

    for (const auto& param_info : m_shader_info->m_params)
    {
        const MaxParam& max_param = param_info.m_max_param;
        Texmap* texmap = nullptr;
        if (max_param.m_connectable)
        {
            GetParamBlock(0)->GetValue(max_param.m_max_param_id + 1, t, texmap, FOREVER);
            if (texmap != nullptr)
            {
                float output_amount = get_output_amount(texmap, t);
                switch (max_param.m_param_type)
                {
                  case MaxParam::Float:
                    {
                        float constant_value = GetParamBlock(0)->GetFloat(max_param.m_max_param_id, t);
                        connect_float_texture(
                            shader_group.ref(),
                            name,
                            max_param.m_osl_param_name.c_str(),
                            texmap,
                            asf::mix(constant_value, 1.0f, output_amount));
                    }
                    break;
                  case MaxParam::Color:
                    {
                        Color constant_color = GetParamBlock(0)->GetColor(max_param.m_max_param_id, t);
                        connect_color_texture(
                            shader_group.ref(),
                            name,
                            max_param.m_osl_param_name.c_str(),
                            texmap,
                            constant_color);
                    }
                    break;

                  case MaxParam::VectorParam:
                  case MaxParam::NormalParam:
                  case MaxParam::PointParam:
                    {
                        if (is_osl_texture(texmap))
                        {
                            OSLTexture* osl_tex = static_cast<OSLTexture*>(texmap);
                            osl_tex->create_osl_texture(shader_group.ref(), name, max_param.m_osl_param_name.c_str());
                        }
                    }
                    break;
                }
            }
        }

        if (max_param.m_connectable && max_param.m_param_type == MaxParam::Closure)
        {
            Mtl* material = nullptr;
            GetParamBlock(0)->GetValue(max_param.m_max_param_id, t, material, FOREVER);
            if (material != nullptr)
            {
                connect_sub_mtl(assembly, shader_group.ref(), name, max_param.m_osl_param_name.c_str(), material);
            }
        }

        if (!max_param.m_connectable || texmap == nullptr)
        {
            switch (max_param.m_param_type)
            {
              case MaxParam::Float:
                {
                    const float param_value = GetParamBlock(0)->GetFloat(max_param.m_max_param_id, t);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(param_value));
                }
                break;

              case MaxParam::IntNumber:
              case MaxParam::IntCheckbox:
              case MaxParam::IntMapper:
                {
                    const int param_value = GetParamBlock(0)->GetInt(max_param.m_max_param_id, t);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(param_value));
                }
                break;

              case MaxParam::Color:
                {
                    const auto param_value = GetParamBlock(0)->GetColor(max_param.m_max_param_id, t);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(to_color3f(param_value)));
                }
                break;

              case MaxParam::VectorParam:
                {
                    const Point3 param_value = GetParamBlock(0)->GetPoint3(max_param.m_max_param_id, t);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(to_vector3f(param_value)));
                }
                break;

              case MaxParam::StringPopup:
                {
                    std::vector<std::string> fields;
                    asf::tokenize(param_info.m_options, "|", fields);

                    const int param_value = GetParamBlock(0)->GetInt(max_param.m_max_param_id, t);
                    params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(fields[param_value]));
                }
                break;

              case MaxParam::String:
                {
                    const wchar_t* str_value;
                    GetParamBlock(0)->GetValue(max_param.m_max_param_id, t, str_value, FOREVER);
                    if (str_value != nullptr)
                        params.insert(max_param.m_osl_param_name.c_str(), fmt_osl_expr(wide_to_utf8(str_value)));
                }
                break;
            }
        }
    }

    if (m_has_bump_params)
    {
        Texmap* bump_texmap = nullptr;
        IParamBlock2* bump_param_block = GetParamBlock(1);
        if (bump_param_block != nullptr)
        {
            get_paramblock_value_by_name(bump_param_block, L"bump_texmap", t, bump_texmap, FOREVER);
            if (bump_texmap != nullptr)
            {
                int bump_method = 0;
                int bump_up_vector = 0;
                float bump_amount = 0.0f;
                get_paramblock_value_by_name(bump_param_block, L"bump_method", t, bump_method, FOREVER);
                get_paramblock_value_by_name(bump_param_block, L"bump_amount", t, bump_amount, FOREVER);
                get_paramblock_value_by_name(bump_param_block, L"bump_up_vector", t, bump_up_vector, FOREVER);

                const auto* bump_param = m_shader_info->find_maya_attribute("normalCamera");

                if (bump_method == 0)
                {
                    // Bump mapping.
                    connect_bump_map(shader_group.ref(), name, bump_param->m_param_name.c_str(), "Tn", bump_texmap, bump_amount);
                }
                else if (m_has_normal_params)
                {
                    // Normal mapping.
                    connect_normal_map(shader_group.ref(), name, bump_param->m_param_name.c_str(), "Tn", bump_texmap, bump_up_vector);
                }
            }
        }
    }

    shader_group->add_shader("surface", m_shader_info->m_shader_name.c_str(), name, params);

    const auto closure_2_surface_name = asf::format("{0}_closure_2_surface_name", name);
    shader_group.ref().add_shader("shader", "as_max_closure2Surface", closure_2_surface_name.c_str(), asr::ParamArray());

    const int output_slot_index = GetParamBlock(0)->GetInt(m_shader_info->m_output_param.m_max_param_id, t);
    shader_group.ref().add_connection(
        name,
        m_shader_info->m_output_params[output_slot_index].m_param_name.c_str(),
        closure_2_surface_name.c_str(),
        "in_input");

    assembly.shader_groups().insert(shader_group);

    //
    // Material.
    //

    asr::ParamArray material_params;
    material_params.insert("osl_surface", shader_group_name);

    material_params.insert("alpha_map", 1.0f);
    return asr::OSLMaterialFactory().create(name, material_params);
}

asf::auto_release_ptr<asr::Material> OSLMaterial::create_builtin_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    return asr::GenericMaterialFactory().create(name, asr::ParamArray());
}

Color OSLMaterial::get_viewport_diffuse_color() const
{
    const OSLParamInfo* color_param = nullptr;
    const auto* color = m_shader_info->find_param("in_color");
    const auto* surf_transmittance = m_shader_info->find_param("in_surface_transmittance");
    const auto* face_reflectance = m_shader_info->find_param("in_face_reflectance");

    if (color != nullptr)
        color_param = color;
    else if (surf_transmittance != nullptr)
        color_param = surf_transmittance;
    else if (face_reflectance != nullptr)
        color_param = face_reflectance;

    if (color_param != nullptr)
        return m_pblock->GetColor(color_param->m_max_param.m_max_param_id, GetCOREInterface()->GetTime());

    return Color(0.0f, 0.0f, 0.0f);
}

Color OSLMaterial::get_viewport_specular_color() const
{
    const OSLParamInfo* specular_param = nullptr;
    const auto* specular_color = m_shader_info->find_param("in_specular_color");
    const auto* reflection_tint = m_shader_info->find_param("in_reflection_tint");

    if (specular_color != nullptr)
        specular_param = specular_color;
    else if (reflection_tint != nullptr)
        specular_param = reflection_tint;

    if (specular_param != nullptr)
        return m_pblock->GetColor(specular_param->m_max_param.m_max_param_id, GetCOREInterface()->GetTime());

    return Color(0.9f, 0.9f, 0.9f);
}

float OSLMaterial::get_viewport_shininness_spread() const
{
    const OSLParamInfo* roughness_param = nullptr;
    const auto* specular_roughness = m_shader_info->find_param("in_specular_roughness");
    const auto* roughness = m_shader_info->find_param("in_roughness");

    if (specular_roughness != nullptr)
        roughness_param = specular_roughness;
    else if (roughness != nullptr)
        roughness_param = roughness;

    if (roughness_param != nullptr)
        return (1.0f - m_pblock->GetFloat(roughness_param->m_max_param.m_max_param_id, GetCOREInterface()->GetTime()));

    return 0.6f;
}

float OSLMaterial::get_viewport_shininess_amount() const
{
    const OSLParamInfo* specular_param = nullptr;
    const auto* specular_weight = m_shader_info->find_param("in_specular_weight");
    const auto* specular_spread = m_shader_info->find_param("in_specular_spread");
    const auto* specular_amount = m_shader_info->find_param("in_specular_amount");

    if (specular_weight != nullptr)
        specular_param = specular_weight;
    else if (specular_spread != nullptr)
        specular_param = specular_spread;
    else if (specular_amount != nullptr)
        specular_param = specular_amount;

    if (specular_param != nullptr)
        return m_pblock->GetFloat(specular_param->m_max_param.m_max_param_id, GetCOREInterface()->GetTime());

    return 0.9f;
}

float OSLMaterial::get_viewport_transparency_amount() const
{
    float transparency_value = 0.0f;
    const auto* transmittance = m_shader_info->find_param("in_transmittance_amount");
    const auto* transparency = m_shader_info->find_param("in_transparency");

    if (transmittance != nullptr)
        transparency_value = m_pblock->GetFloat(transmittance->m_max_param.m_max_param_id, GetCOREInterface()->GetTime());
    else if (transparency != nullptr)
    {
        Color transp_color = m_pblock->GetColor(transparency->m_max_param.m_max_param_id, GetCOREInterface()->GetTime());
        transparency_value = (transp_color.r + transp_color.g + transp_color.b) / 3.0f;
    }
    return transparency_value;
}
