
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
#include "appleseedosltexture/oslshadermetadata.h"
#include "appleseedosltexture/oslparamdlg.h"
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
    const wchar_t* GenericOSLTextureFriendlyClassName = L"appleseed osl texture";
}


//
// OSLMtlBase class implementation.
//

Class_ID OSLMaterial::get_class_id()
{
    return m_classid;
}

OSLMaterial::OSLMaterial(Class_ID class_id, ClassDesc2* class_desc)
    : m_pblock(nullptr)
    , m_classid(class_id)
    , m_class_desc(class_desc)
    , m_shader_info(nullptr)
{
    m_shader_info = static_cast<OSLPluginClassDesc*>(m_class_desc)->m_shader_info;
    m_texture_id_map = m_shader_info->m_texture_id_map;
    m_submaterial_map = m_shader_info->m_material_id_map;
    
    auto tn_vec = m_shader_info->findParam("Tn");
    m_has_bump_params = tn_vec != nullptr;

    m_class_desc->MakeAutoParamBlocks(this);
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
    s = static_cast<OSLPluginClassDesc*>(m_class_desc)->ClassName();
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
    switch (i)
    {
      case 0:
        return m_pblock;
      default:
        return nullptr;
    }
}

TSTR OSLMaterial::SubAnimName(int i)
{
    switch (i)
    {
      case 0:
        return L"Parameters";
      default:
        return nullptr;
    }
}

int OSLMaterial::SubNumToRefNum(int subNum)
{
    return subNum;
}

int OSLMaterial::NumParamBlocks()
{
    return 1;
}

IParamBlock2* OSLMaterial::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* OSLMaterial::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int OSLMaterial::NumRefs()
{
    return 1;
}

RefTargetHandle OSLMaterial::GetReference(int i)
{
    switch (i)
    {
      case 0:
        return m_pblock;
      default:
        return nullptr;
    }
}

void OSLMaterial::SetReference(int i, RefTargetHandle rt_arg)
{
    switch (i)
    {
      case 0:
        m_pblock = static_cast<IParamBlock2*>(rt_arg);
        break;
      default:
        break;
    }
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
        m_pblock->GetValue(m_texture_id_map[i].first, 0, tex_map, iv);

    return tex_map;
}

void OSLMaterial::SetSubTexmap(int i, Texmap* tex_map)
{
    if (i < m_texture_id_map.size())
    {
        const auto param_id = m_texture_id_map[i].first;
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
    
    for (auto& tex_param : m_texture_id_map)
    {
        Texmap* tex_map = nullptr;
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
    // Will need normal map rollout here
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
    return Color(0.0f, 0.0f, 0.0f);
}

Color OSLMaterial::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float OSLMaterial::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float OSLMaterial::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float OSLMaterial::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
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
    return
        use_max_procedural_maps
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
    auto params = asr::ParamArray();

    auto shader_group_name = make_unique_name(assembly.shader_groups(), std::string(name) + "_shader_group");
    auto shader_group = asr::ShaderGroupFactory::create(shader_group_name.c_str());

    for (auto& param_info : m_shader_info->m_params)
    {
        const MaxParam& max_param = param_info.m_max_param;
        Texmap* texmap = nullptr;
        if (max_param.connectable)
        {
            GetParamBlock(0)->GetValue(max_param.max_param_id + 1, t, texmap, FOREVER);
            if (texmap != nullptr)
            {
                switch (max_param.param_type)
                {
                  case MaxParam::Float:
                    {
                        connect_float_texture(
                            shader_group.ref(),
                            name,
                            max_param.osl_param_name.c_str(),
                            texmap,
                            GetParamBlock(0)->GetFloat(max_param.max_param_id, t));
                    }
                    break;
                  case MaxParam::Color:
                    {
                        connect_color_texture(
                            shader_group.ref(),
                            name,
                            max_param.osl_param_name.c_str(),
                            texmap,
                            GetParamBlock(0)->GetColor(max_param.max_param_id, t));
                    }
                    break;
                  case MaxParam::VectorParam:
                    {
                    //connect_vector_output(
                    //    shader_group,
                    //      layer_name.c_str(),
                    //      max_param.osl_param_name.c_str(),
                    //      texmap);
                    }
                    break;
                    }
            }
        }

        if (max_param.connectable && max_param.param_type == MaxParam::Closure)
        {
            Mtl* material = nullptr;
            GetParamBlock(0)->GetValue(max_param.max_param_id, t, material, FOREVER);
            if (material != nullptr)
            {
                connect_sub_mtl(assembly, shader_group.ref(), name, max_param.osl_param_name.c_str(), material);
            }
        }

        if (!max_param.connectable || texmap == nullptr)
        {
            switch (max_param.param_type)
            {
            case MaxParam::Float:
            {
                float param_value = GetParamBlock(0)->GetFloat(max_param.max_param_id, t);
                params.insert(max_param.osl_param_name.c_str(), fmt_osl_expr(param_value));
            }
            break;
            case MaxParam::IntNumber:
            case MaxParam::IntCheckbox:
            case MaxParam::IntMapper:
            {
                int param_value = GetParamBlock(0)->GetInt(max_param.max_param_id, t);
                params.insert(max_param.osl_param_name.c_str(), fmt_osl_expr(param_value));
            }
            break;
            case MaxParam::Color:
            {
                auto param_value = GetParamBlock(0)->GetColor(max_param.max_param_id, t);
                params.insert(max_param.osl_param_name.c_str(), fmt_osl_expr(to_color3f(param_value)));
            }
            break;
            case MaxParam::VectorParam:
            {
                Point3 param_value = GetParamBlock(0)->GetPoint3(max_param.max_param_id, t);
                params.insert(max_param.osl_param_name.c_str(), fmt_osl_expr(to_vector3f(param_value)));
            }
            break;

            case MaxParam::StringPopup:
            {
                std::vector<std::string> fields;
                asf::tokenize(param_info.options, "|", fields);

                int param_value = GetParamBlock(0)->GetInt(max_param.max_param_id, t);
                params.insert(max_param.osl_param_name.c_str(), fmt_osl_expr(fields[param_value]));
            }
            break;
            case MaxParam::String:
                break;
            }
        }
    }

    if (m_has_bump_params)
    {
        Texmap* bump_texmap = nullptr;
        get_paramblock_value_by_name(GetParamBlock(0), L"bump_texmap", t, bump_texmap, FOREVER);
        if (bump_texmap != nullptr)
        {
            int bump_method = 0;
            int bump_up_vector = 0;
            float bump_amount = 0.0f;
            get_paramblock_value_by_name(GetParamBlock(0), L"bump_method", t, bump_method, FOREVER);
            get_paramblock_value_by_name(GetParamBlock(0), L"bump_amount", t, bump_amount, FOREVER);
            get_paramblock_value_by_name(GetParamBlock(0), L"bump_up_vector", t, bump_up_vector, FOREVER);

            char* bump_input = m_shader_info->findParam("in_bump_normal_substrate") != nullptr ? 
                "in_bump_normal_substrate" : "in_bump_normal";

            if (bump_texmap != nullptr)
            {
                if (bump_method == 0)
                {
                    // Bump mapping.
                    connect_bump_map(shader_group.ref(), name, bump_input, "Tn", bump_texmap, bump_amount);
                }
                else
                {
                    // Normal mapping.
                    connect_normal_map(shader_group.ref(), name, bump_input, "Tn", bump_texmap, bump_up_vector);
                }
            }
        }
    }

    shader_group->add_shader("surface", m_shader_info->m_shader_name.c_str(), name, params);

    auto closure_2_surface_name = asf::format("{0}_closure_2_surface_name", name);
    shader_group.ref().add_shader("shader", "as_max_closure2Surface", closure_2_surface_name.c_str(), asr::ParamArray());

    int output_slot_index = GetParamBlock(0)->GetInt(m_shader_info->m_output_param.max_param_id, t);

    shader_group.ref().add_connection(
        name,
        m_shader_info->m_output_params[output_slot_index].paramName.c_str(),
        closure_2_surface_name.c_str(),
        "in_input");

    assembly.shader_groups().insert(shader_group);

    //
    // Material.
    //

    asr::ParamArray material_params;
    material_params.insert("osl_surface", shader_group_name);

    //const std::string instance_name =
    //    insert_texture_and_instance(
    //        assembly,
    //        m_alpha_texmap,
    //        false,
    //        asr::ParamArray(),
    //        asr::ParamArray()
    //        .insert("alpha_mode", "detect"));
    //if (!instance_name.empty())
    //    material_params.insert("alpha_map", instance_name);
    //else material_params.insert("alpha_map", m_alpha / 100.0f);

    material_params.insert("alpha_map", 1.0f);
    return asr::OSLMaterialFactory().create(name, material_params);
}

asf::auto_release_ptr<asr::Material> OSLMaterial::create_builtin_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    //
    // Material.
    //

    return asr::GenericMaterialFactory().create(name, asr::ParamArray());
}
