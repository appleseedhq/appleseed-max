
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
#include "osltexture.h"

// appleseed-max headers.
#include "appleseedrenderer/appleseedrenderer.h"
#include "appleseedoslplugin/oslclassdesc.h"
#include "appleseedoslplugin/oslparamdlg.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
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
// OSLTexture class implementation.
//

ParamDlg* OSLTexture::m_uv_gen_dlg;

Class_ID OSLTexture::get_class_id()
{
    return m_classid;
}

OSLTexture::OSLTexture(Class_ID class_id, OSLPluginClassDesc* class_desc)
  : m_pblock(nullptr)
  , m_uv_gen(nullptr)
  , m_has_xyz_coords(false)
  , m_has_uv_coords(false)
  , m_classid(class_id)
  , m_class_desc(class_desc)
  , m_shader_info(nullptr)
{
    m_shader_info = m_class_desc->m_shader_info;
    m_texture_id_map = m_shader_info->m_texture_id_map;

    m_has_uv_coords = m_shader_info->find_param("in_uvCoord") != nullptr;
    m_has_xyz_coords = m_shader_info->find_param("in_placementMatrix") != nullptr;
    m_class_desc->MakeAutoParamBlocks(this);
    Reset();
}

void OSLTexture::DeleteThis()
{
    delete this;
}

void OSLTexture::GetClassName(TSTR& s)
{
    s = m_class_desc->ClassName();
}

SClass_ID OSLTexture::SuperClassID()
{
    return TEXMAP_CLASS_ID;
}

Class_ID OSLTexture::ClassID()
{
    return get_class_id();
}

int OSLTexture::NumSubs()
{
    if (m_has_uv_coords || m_has_xyz_coords)
        return 2;
    return 1;
}

Animatable* OSLTexture::SubAnim(int i)
{
    switch (i)
    {
      case 0:
        return m_pblock;
      case 1:
        if (m_has_uv_coords)
          return m_uv_gen;
        return nullptr;
      default:
        return nullptr;
    }
}

TSTR OSLTexture::SubAnimName(int i)
{
    switch (i)
    {
      case 0:
        return L"Parameters";
      case 1:
        if (m_has_uv_coords || m_has_xyz_coords)
            return L"Coordinates";
        return nullptr;
      default:
        return nullptr;
    }
}

int OSLTexture::SubNumToRefNum(int subNum)
{
    return subNum;
}

int OSLTexture::NumParamBlocks()
{
    return 1;
}

IParamBlock2* OSLTexture::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* OSLTexture::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int OSLTexture::NumRefs()
{
    if (m_has_uv_coords || m_has_xyz_coords)
        return 2;
    return 1;
}

RefTargetHandle OSLTexture::GetReference(int i)
{
    switch (i)
    {
      case 0:
        return m_pblock;
      case 1:
        if (m_has_uv_coords)
            return m_uv_gen;
        return nullptr;
      default:
        return nullptr;
    }
}

void OSLTexture::SetReference(int i, RefTargetHandle rt_arg)
{
    switch (i)
    {
      case 0:
        m_pblock = static_cast<IParamBlock2*>(rt_arg);
        break;
      case 1:
        if (m_has_uv_coords)
            m_uv_gen = static_cast<UVGen*>(rt_arg);
        break;
    }
}

RefResult OSLTexture::NotifyRefChanged(
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

RefTargetHandle OSLTexture::Clone(RemapDir& remap)
{
    OSLTexture* mnew = new OSLTexture(m_classid, m_class_desc);
    *static_cast<MtlBase*>(mnew) = *static_cast<MtlBase*>(this);
    BaseClone(this, mnew, remap);

    mnew->ReplaceReference(0, remap.CloneRef(m_pblock));
    if (m_has_uv_coords)
        mnew->ReplaceReference(1, remap.CloneRef(m_uv_gen));

    return (RefTargetHandle)mnew;
}

int OSLTexture::NumSubTexmaps()
{
    return static_cast<int>(m_texture_id_map.size());
}

Texmap* OSLTexture::GetSubTexmap(int i)
{
    Texmap* tex_map = nullptr;
    Interval iv;
    if (i < m_texture_id_map.size())
        m_pblock->GetValue(m_texture_id_map[i].first, 0, tex_map, iv);

    return tex_map;
}

void OSLTexture::SetSubTexmap(int i, Texmap* tex_map)
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

int OSLTexture::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

TSTR OSLTexture::GetSubTexmapSlotName(int i)
{
    if (i < m_texture_id_map.size())
        return m_texture_id_map[i].second.c_str();
    return L"";
}

void OSLTexture::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }
    m_params_validity.SetInfinite();
    
    if (m_uv_gen != nullptr)
        m_uv_gen->Update(t, valid);

    for (auto& tex_param : m_texture_id_map)
    {
        Texmap* tex_map = nullptr;
        m_pblock->GetValue(tex_param.first, t, tex_map, valid);
        if (tex_map)
            tex_map->Update(t, valid);
    }

    valid = m_params_validity;
}

void OSLTexture::Reset()
{
    if (m_has_uv_coords)
    {
        if (m_uv_gen != nullptr)
            m_uv_gen->Reset();
        else
            ReplaceReference(1, GetNewDefaultUVGen());
    }
    m_params_validity.SetEmpty();
}

Interval OSLTexture::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* OSLTexture::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    if (m_has_uv_coords)
    {
        if (m_uv_gen != nullptr)
            m_uv_gen_dlg = m_uv_gen->CreateParamDlg(hwMtlEdit, imp);
    }
    ParamDlg* master_dlg = new OSLParamDlg(hwMtlEdit, imp, this, m_shader_info);
    return master_dlg;
}

BOOL OSLTexture::SetDlgThing(ParamDlg* dlg)
{
    if (dlg == m_uv_gen_dlg && m_uv_gen != nullptr)
        m_uv_gen_dlg->SetThing(m_uv_gen);
    else
        return FALSE;
    return TRUE;
}

IOResult OSLTexture::Save(ISave* isave)
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

IOResult OSLTexture::Load(ILoad* iload)
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

AColor OSLTexture::EvalColor(ShadeContext& sc)
{
    const Color base_color(0.13f, 0.58f, 1.0f);
    return base_color;
}

Point3 OSLTexture::EvalNormalPerturb(ShadeContext& /*sc*/)
{
    return Point3(0.0f, 0.0f, 0.0f);
}

UVGen* OSLTexture::GetTheUVGen()
{
    if (m_has_uv_coords)
        return m_uv_gen;
    else
        return nullptr;
}

void OSLTexture::create_osl_texture(
    renderer::ShaderGroup& shader_group,
    const char* material_node_name,
    const char* material_input_name)
{
    const auto t = GetCOREInterface()->GetTime();
    asr::ParamArray params;
    auto layer_name = asf::format("{0}_{1}", material_node_name, material_input_name);

    for (const auto& param_info : m_shader_info->m_params)
    {
        const MaxParam& max_param = param_info.m_max_param;
        Texmap* texmap = nullptr;
        if (max_param.m_connectable)
        {
            GetParamBlock(0)->GetValue(max_param.m_max_param_id + 1, t, texmap, FOREVER);
            if (texmap != nullptr)
            {
                switch (max_param.m_param_type)
                {
                  case MaxParam::Float:
                    {
                        connect_float_texture(
                            shader_group,
                            layer_name.c_str(),
                            max_param.m_osl_param_name.c_str(),
                            texmap,
                            1.0f);
                    }
                    break;

                  case MaxParam::Color:
                    {
                        connect_color_texture(
                            shader_group,
                            layer_name.c_str(),
                            max_param.m_osl_param_name.c_str(),
                            texmap,
                            Color(1.0f, 1.0f, 1.0f));
                    }
                    break;

                  case MaxParam::VectorParam:
                  case MaxParam::NormalParam:
                  case MaxParam::PointParam:
                    {
                        if (is_osl_texture(texmap))
                        {
                            OSLTexture* osl_tex = static_cast<OSLTexture*>(texmap);
                            osl_tex->create_osl_texture(shader_group, layer_name.c_str(), max_param.m_osl_param_name.c_str());
                        }
                    }
                    break;
                }
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

    if (m_has_uv_coords)
    {
        auto uv_transform_layer_name = asf::format("{0}_{1}_uv_transform", material_node_name, material_input_name);
        shader_group.add_shader("shader", "as_max_uv_transform", uv_transform_layer_name.c_str(), get_uv_params(this));

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_outUV",
            layer_name.c_str(), "in_uvCoord");

        shader_group.add_connection(
            uv_transform_layer_name.c_str(), "out_outUvFilterSize",
            layer_name.c_str(), "in_uvFilterSize");
    }

    shader_group.add_shader("shader", m_shader_info->m_shader_name.c_str(), layer_name.c_str(), params);
    int output_slot_index = GetParamBlock(0)->GetInt(m_shader_info->m_output_param.m_max_param_id, t);
    m_shader_info->m_output_params[output_slot_index].m_param_name;
    
    //TODO: Choose right type of output based on expected input type.
    shader_group.add_connection(
        layer_name.c_str(),
        m_shader_info->m_output_params[output_slot_index].m_param_name.c_str(),
        material_node_name,
        material_input_name);
}
