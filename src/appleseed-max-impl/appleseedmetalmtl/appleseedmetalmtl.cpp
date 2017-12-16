
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
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
#include "appleseedmetalmtl.h"

// appleseed-max headers.
#include "appleseedmetalmtl/datachunks.h"
#include "appleseedmetalmtl/resource.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "bump/bumpparammapdlgproc.h"
#include "bump/resource.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/bsdf.h"
#include "renderer/api/material.h"
#include "renderer/api/scene.h"
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"

// 3ds Max headers.
#include <color.h>
#include <iparamm2.h>
#include <stdmat.h>
#include <strclass.h>

// Standard headers.
#include <string>

// Windows headers.
#include <tchar.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const wchar_t* AppleseedMetalMtlFriendlyClassName = L"appleseed Metal Material";
}

AppleseedMetalMtlClassDesc g_appleseed_metalmtl_classdesc;


//
// AppleseedMetalMtl class implementation.
//

namespace
{
    enum { ParamBlockIdMetalMtl };
    enum { ParamBlockRefMetalMtl };

    enum ParamMapId
    {
        ParamMapIdMetal,
        ParamMapIdBump
    };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdNormalReflectance        = 0,
        ParamIdNormalReflectanceTexmap  = 1,
        ParamIdEdgeTint                 = 2,
        ParamIdEdgeTintTexmap           = 3,
        ParamIdReflectance              = 4,
        ParamIdReflectanceTexmap        = 5,
        ParamIdRoughness                = 6,
        ParamIdRoughnessTexmap          = 7,
        ParamIdAnisotropy               = 8,
        ParamIdAnisotropyTexmap         = 9,
        ParamIdAlpha                    = 10,
        ParamIdAlphaTexmap              = 11,
        ParamIdBumpMethod               = 12,
        ParamIdBumpTexmap               = 13,
        ParamIdBumpAmount               = 14,
        ParamIdBumpUpVector             = 15,
    };

    enum TexmapId
    {
        // Changing these value WILL break compatibility.
        TexmapIdNormalReflectance   = 0,
        TexmapIdEdgeTint            = 1,
        TexmapIdReflectance         = 2,
        TexmapIdRoughness           = 3,
        TexmapIdAnisotropy          = 4,
        TexmapIdAlpha               = 5,
        TexmapIdBumpMap             = 6,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        L"Normal Reflectance",
        L"Edge Tint",
        L"Reflectance",
        L"Roughness",
        L"Anisotropy",
        L"Alpha",
        L"Bump Map"
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdNormalReflectanceTexmap,
        ParamIdEdgeTintTexmap,
        ParamIdReflectanceTexmap,
        ParamIdRoughnessTexmap,
        ParamIdAnisotropyTexmap,
        ParamIdAlphaTexmap,
        ParamIdBumpTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdMetalMtl,                       // parameter block's ID
        L"appleseedMetalMtlParams",                // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_metalmtl_classdesc,            // class descriptor
        P_AUTO_CONSTRUCT + P_MULTIMAP + P_AUTO_UI,  // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefMetalMtl,                      // parameter block's reference number

        // --- P_MULTIMAP arguments ---
        2,                                          // number of rollups

        // --- P_AUTO_UI arguments for Metal rollup ---
        ParamMapIdMetal,
        IDD_FORMVIEW_PARAMS,                        // ID of the dialog template
        IDS_FORMVIEW_PARAMS_TITLE,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- P_AUTO_UI arguments for Bump rollup ---
        ParamMapIdBump,
        IDD_FORMVIEW_BUMP_PARAMS,                   // ID of the dialog template
        IDS_FORMVIEW_BUMP_PARAMS_TITLE,             // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- Parameters specifications for Metal rollup ---

        ParamIdNormalReflectance, L"normal_reflectance", TYPE_RGBA, P_ANIMATABLE, IDS_NORMAL_REFLECTANCE,
            p_default, Color(0.92f, 0.92f, 0.92f),
            p_ui, ParamMapIdMetal, TYPE_COLORSWATCH, IDC_SWATCH_NORMAL_REFLECTANCE,
        p_end,
        ParamIdNormalReflectanceTexmap, L"normal_reflectance_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_NORMAL_REFLECTANCE,
            p_subtexno, TexmapIdNormalReflectance,
            p_ui, ParamMapIdMetal, TYPE_TEXMAPBUTTON, IDC_TEXMAP_NORMAL_REFLECTANCE,
        p_end,

        ParamIdEdgeTint, L"edge_tint", TYPE_RGBA, P_ANIMATABLE, IDS_EDGE_TINT,
            p_default, Color(0.98f, 0.98f, 0.98f),
            p_ui, ParamMapIdMetal, TYPE_COLORSWATCH, IDC_SWATCH_EDGE_TINT,
            p_end,
        ParamIdEdgeTintTexmap, L"edge_tint_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_EDGE_TINT,
            p_subtexno, TexmapIdEdgeTint,
            p_ui, ParamMapIdMetal, TYPE_TEXMAPBUTTON, IDC_TEXMAP_EDGE_TINT,
        p_end,

        ParamIdReflectance, L"reflectance", TYPE_FLOAT, P_ANIMATABLE, IDS_REFLECTANCE,
            p_default, 80.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdMetal, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_REFLECTANCE, IDC_SLIDER_REFLECTANCE, 10.0f,
        p_end,
        ParamIdReflectanceTexmap, L"reflectance_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_REFLECTANCE,
            p_subtexno, TexmapIdReflectance,
            p_ui, ParamMapIdMetal, TYPE_TEXMAPBUTTON, IDC_TEXMAP_REFLECTANCE,
        p_end,

        ParamIdRoughness, L"roughness", TYPE_FLOAT, P_ANIMATABLE, IDS_ROUGHNESS,
            p_default, 10.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdMetal, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ROUGHNESS, IDC_SLIDER_ROUGHNESS, 10.0f,
        p_end,
        ParamIdRoughnessTexmap, L"roughness_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ROUGHNESS,
            p_subtexno, TexmapIdRoughness,
            p_ui, ParamMapIdMetal, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ROUGHNESS,
        p_end,

        ParamIdAnisotropy, L"anisotropy", TYPE_FLOAT, P_ANIMATABLE, IDS_ANISOTROPY,
            p_default, 0.0f,
            p_range, -1.0f, 1.0f,
            p_ui, ParamMapIdMetal, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ANISOTROPY, IDC_SLIDER_ANISOTROPY, 0.1f,
        p_end,
        ParamIdAnisotropyTexmap, L"anisotropy_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ANISOTROPY,
            p_subtexno, TexmapIdAnisotropy,
            p_ui, ParamMapIdMetal, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ANISOTROPY,
        p_end,
        
        ParamIdAlpha, L"alpha", TYPE_FLOAT, P_ANIMATABLE, IDS_ALPHA,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdMetal, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ALPHA, IDC_SLIDER_ALPHA, 10.0f,
        p_end,
        ParamIdAlphaTexmap, L"alpha_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ALPHA,
            p_subtexno, TexmapIdAlpha,
            p_ui, ParamMapIdMetal, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ALPHA,
        p_end,

        // --- Parameters specifications for Bump rollup ---

        ParamIdBumpMethod, L"bump_method", TYPE_INT, 0, IDS_BUMP_METHOD,
            p_ui, ParamMapIdBump, TYPE_INT_COMBOBOX, IDC_COMBO_BUMP_METHOD,
            2, IDS_COMBO_BUMP_METHOD_BUMPMAP, IDS_COMBO_BUMP_METHOD_NORMALMAP,
            p_vals, 0, 1,
            p_default, 0,
        p_end,

        ParamIdBumpTexmap, L"bump_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_BUMP_MAP,
            p_subtexno, TexmapIdBumpMap,
            p_ui, ParamMapIdBump, TYPE_TEXMAPBUTTON, IDC_TEXMAP_BUMP_MAP,
        p_end,

        ParamIdBumpAmount, L"bump_amount", TYPE_FLOAT, P_ANIMATABLE, IDS_BUMP_AMOUNT,
            p_default, 1.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdBump, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_BUMP_AMOUNT, IDC_SPINNER_BUMP_AMOUNT, SPIN_AUTOSCALE,
        p_end,

        ParamIdBumpUpVector, L"bump_up_vector", TYPE_INT, 0, IDS_BUMP_UP_VECTOR,
            p_ui, ParamMapIdBump, TYPE_INT_COMBOBOX, IDC_COMBO_BUMP_UP_VECTOR,
            2, IDS_COMBO_BUMP_UP_VECTOR_Y, IDS_COMBO_BUMP_UP_VECTOR_Z,
            p_vals, 0, 1,
            p_default, 1,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedMetalMtl::get_class_id()
{
    return Class_ID(0x4f357819, 0x6a656388);
}

AppleseedMetalMtl::AppleseedMetalMtl()
  : m_pblock(nullptr)
  , m_normal_reflectance_color(0.92f, 0.92f, 0.92f)
  , m_normal_reflectance_color_texmap(nullptr)
  , m_edge_tint_color(0.98f, 0.98f, 0.98f)
  , m_edge_tint_color_texmap(nullptr)
  , m_reflectance(80.0f)
  , m_reflectance_texmap(nullptr)
  , m_roughness(10.0f)
  , m_roughness_texmap(nullptr)
  , m_anisotropy(0.0f)
  , m_anisotropy_texmap(nullptr)
  , m_alpha(100.0f)
  , m_alpha_texmap(nullptr)
  , m_bump_method(0)
  , m_bump_texmap(nullptr)
  , m_bump_amount(1.0f)
  , m_bump_up_vector(1)
{
    m_params_validity.SetEmpty();

    g_appleseed_metalmtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedMetalMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedMetalMtl::DeleteThis()
{
    delete this;
}

void AppleseedMetalMtl::GetClassName(TSTR& s)
{
    s = L"appleseedMetalMtl";
}

SClass_ID AppleseedMetalMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedMetalMtl::ClassID()
{
    return get_class_id();
}

int AppleseedMetalMtl::NumSubs()
{
    return NumRefs();
}

Animatable* AppleseedMetalMtl::SubAnim(int i)
{
    return GetReference(i);
}

TSTR AppleseedMetalMtl::SubAnimName(int i)
{
    return i == ParamBlockRefMetalMtl ? L"Parameters" : L"";
}

int AppleseedMetalMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedMetalMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedMetalMtl::GetParamBlock(int i)
{
    return i == ParamBlockRefMetalMtl ? m_pblock : nullptr;
}

IParamBlock2* AppleseedMetalMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedMetalMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedMetalMtl::GetReference(int i)
{
    return i == ParamBlockRefMetalMtl ? m_pblock : nullptr;
}

void AppleseedMetalMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefMetalMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedMetalMtl::NotifyRefChanged(
    const Interval&     changeInt,
    RefTargetHandle     hTarget,
    PartID&             partID,
    RefMessage          message,
    BOOL                propagate)
{
    switch (message)
    {
      case REFMSG_CHANGE:
        m_params_validity.SetEmpty();
        if (hTarget == m_pblock)
            g_block_desc.InvalidateUI(m_pblock->LastNotifyParamID());
        break;
    }

    return REF_SUCCEED;
}

RefTargetHandle AppleseedMetalMtl::Clone(RemapDir& remap)
{
    AppleseedMetalMtl* clone = new AppleseedMetalMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(ParamBlockRefMetalMtl, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedMetalMtl::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedMetalMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedMetalMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);

    IParamMap2* map = m_pblock->GetMap(ParamMapIdMetal);
    if (map != nullptr)
    {
        map->SetText(param_id, texmap == nullptr ? L"" : L"M");
    }
}

int AppleseedMetalMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedMetalMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedMetalMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdNormalReflectance, t, m_normal_reflectance_color, m_params_validity);
        m_pblock->GetValue(ParamIdNormalReflectanceTexmap, t, m_normal_reflectance_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdEdgeTint, t, m_edge_tint_color, m_params_validity);
        m_pblock->GetValue(ParamIdEdgeTintTexmap, t, m_edge_tint_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdReflectance, t, m_reflectance, m_params_validity);
        m_pblock->GetValue(ParamIdReflectanceTexmap, t, m_reflectance_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdRoughness, t, m_roughness, m_params_validity);
        m_pblock->GetValue(ParamIdRoughnessTexmap, t, m_roughness_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdAnisotropy, t, m_anisotropy, m_params_validity);
        m_pblock->GetValue(ParamIdAnisotropyTexmap, t, m_anisotropy_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdAlpha, t, m_alpha, m_params_validity);
        m_pblock->GetValue(ParamIdAlphaTexmap, t, m_alpha_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdBumpMethod, t, m_bump_method, m_params_validity);
        m_pblock->GetValue(ParamIdBumpTexmap, t, m_bump_texmap, m_params_validity);
        m_pblock->GetValue(ParamIdBumpAmount, t, m_bump_amount, m_params_validity);
        m_pblock->GetValue(ParamIdBumpUpVector, t, m_bump_up_vector, m_params_validity);

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }

    valid &= m_params_validity;
}

void AppleseedMetalMtl::Reset()
{
    g_appleseed_metalmtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedMetalMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedMetalMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    ParamDlg* param_dialog = g_appleseed_metalmtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    DbgAssert(m_pblock != nullptr);
    update_map_buttons(m_pblock->GetMap(ParamMapIdMetal));
    g_block_desc.SetUserDlgProc(ParamMapIdBump, new BumpParamMapDlgProc());
    return param_dialog;
}

IOResult AppleseedMetalMtl::Save(ISave* isave)
{
    bool success = true;

    isave->BeginChunk(ChunkFileFormatVersion);
    success &= write(isave, FileFormatVersion);
    isave->EndChunk();

    isave->BeginChunk(ChunkMtlBase);
    success &= MtlBase::Save(isave) == IO_OK;
    isave->EndChunk();

    return success ? IO_OK : IO_ERROR;
}

IOResult AppleseedMetalMtl::Load(ILoad* iload)
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

Color AppleseedMetalMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedMetalMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return m_normal_reflectance_color;
}

Color AppleseedMetalMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedMetalMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedMetalMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedMetalMtl::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedMetalMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedMetalMtl::SetDiffuse(Color c, TimeValue t)
{
    Color nc;
    Interval iv;
    m_pblock->SetValue(ParamIdNormalReflectance, t, c);

    m_pblock->GetValue(ParamIdNormalReflectance, t, nc, iv);
    m_normal_reflectance_color = nc;
}

void AppleseedMetalMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedMetalMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedMetalMtl::Shade(ShadeContext& sc)
{
}

int AppleseedMetalMtl::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool AppleseedMetalMtl::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> AppleseedMetalMtl::create_material(
    asr::Assembly&  assembly,
    const char*     name,
    const bool      use_max_procedural_maps)
{
    return
        use_max_procedural_maps
            ? create_builtin_material(assembly, name)
            : create_osl_material(assembly, name);
}

asf::auto_release_ptr<asr::Material> AppleseedMetalMtl::create_osl_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    //
    // Shader group.
    //

    auto shader_group_name = make_unique_name(assembly.shader_groups(), std::string(name) + "_shader_group");
    auto shader_group = asr::ShaderGroupFactory::create(shader_group_name.c_str());

    connect_color_texture(shader_group.ref(), name, "NormalReflectance", m_normal_reflectance_color_texmap, m_normal_reflectance_color);
    connect_color_texture(shader_group.ref(), name, "EdgeTint", m_edge_tint_color_texmap, m_edge_tint_color);
    connect_float_texture(shader_group.ref(), name, "Reflectance", m_reflectance_texmap, m_reflectance / 100.0f);
    connect_float_texture(shader_group.ref(), name, "Roughness", m_roughness_texmap, m_roughness / 100.0f);
    connect_float_texture(shader_group.ref(), name, "Anisotropic", m_anisotropy_texmap, m_anisotropy);

    if (is_bitmap_texture(m_bump_texmap))
    {
        if (m_bump_method == 0)
        {
            // Bump mapping.
            connect_bump_map(shader_group.ref(), name, "Normal", "Tn", m_bump_texmap, m_bump_amount);
        }
        else
        {
            // Normal mapping.
            connect_normal_map(shader_group.ref(), name, "Normal", "Tn", m_bump_texmap, m_bump_up_vector);
        }
    }

    // Must come last.
    shader_group->add_shader("surface", "as_max_metal_material", name, asr::ParamArray());

    assembly.shader_groups().insert(shader_group);

    //
    // Material.
    //

    asr::ParamArray material_params;
    material_params.insert("osl_surface", shader_group_name);

    const std::string instance_name =
        insert_texture_and_instance(
            assembly,
            m_alpha_texmap,
            false,
            asr::ParamArray(),
            asr::ParamArray()
                .insert("alpha_mode", "detect"));
    if (!instance_name.empty())
        material_params.insert("alpha_map", instance_name);
    else material_params.insert("alpha_map", m_alpha / 100.0f);

    return asr::OSLMaterialFactory().create(name, material_params);
}

asf::auto_release_ptr<asr::Material> AppleseedMetalMtl::create_builtin_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    asr::ParamArray material_params;
    std::string instance_name;
    const bool use_max_procedural_maps = true;

    //
    // BRDF.
    //

    {
        asr::ParamArray bsdf_params;
        bsdf_params.insert("mdf", "ggx");

        // Normal Reflectance.
        instance_name = insert_texture_and_instance(assembly, m_normal_reflectance_color_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("normal_reflectance", instance_name);
        else
        {
            const auto color_name = std::string(name) + "_bsdf_normal_reflectance_color";
            insert_color(assembly, m_normal_reflectance_color, color_name.c_str());
            bsdf_params.insert("normal_reflectance", color_name);
        }

        // Edge Tint.
        instance_name = insert_texture_and_instance(assembly, m_edge_tint_color_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("edge_tint", instance_name);
        else
        {
            const auto color_name = std::string(name) + "_bsdf_edge_tint";
            insert_color(assembly, m_edge_tint_color, color_name.c_str());
            bsdf_params.insert("edge_tint", color_name);
        }

        // Reflectance.
        instance_name = insert_texture_and_instance(assembly, m_reflectance_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("reflectance_multiplier", instance_name);
        else bsdf_params.insert("reflectance_multiplier", m_reflectance / 100.0f);

        // Roughness.
        instance_name = insert_texture_and_instance(assembly, m_roughness_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("roughness", instance_name);
        else bsdf_params.insert("roughness", m_roughness / 100.0f);

        // Anisotropic.
        instance_name = insert_texture_and_instance(assembly, m_anisotropy_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("anisotropic", instance_name);
        else bsdf_params.insert("anisotropic", m_anisotropy);

        // BRDF.
        const auto bsdf_name = std::string(name) + "_bsdf";
        assembly.bsdfs().insert(
            asr::MetalBRDFFactory().create(bsdf_name.c_str(), bsdf_params));
        material_params.insert("bsdf", bsdf_name);
    }

    //
    // Material.
    //

    // Alpha map.
    instance_name = insert_texture_and_instance(
        assembly,
        m_alpha_texmap,
        use_max_procedural_maps,
        asr::ParamArray(),
        asr::ParamArray()
            .insert("alpha_mode", "detect"));
    if (!instance_name.empty())
        material_params.insert("alpha_map", instance_name);
    else material_params.insert("alpha_map", m_alpha / 100.0f);

    // Displacement.
    instance_name = insert_texture_and_instance(
        assembly,
        m_bump_texmap,
        use_max_procedural_maps,
        asr::ParamArray()
            .insert("color_space", "linear_rgb"));
    if (!instance_name.empty())
    {
        material_params.insert("displacement_method", m_bump_method == 0 ? "bump" : "normal");
        material_params.insert("displacement_map", instance_name);

        switch (m_bump_method)
        {
          case 0:
            material_params.insert("bump_amplitude", m_bump_amount);
            material_params.insert("bump_offset", 0.5f / 512);
            break;

          case 1:
            material_params.insert("normal_map_up", m_bump_up_vector == 0 ? "y" : "z");
            break;
        }
    }

    return asr::GenericMaterialFactory().create(name, material_params);
}


//
// AppleseedMetalMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedMetalMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedMetalMtlFriendlyClassName;
}

const MCHAR* AppleseedMetalMtlBrowserEntryInfo::GetEntryCategory() const
{
    return L"Materials\\appleseed";
}

Bitmap* AppleseedMetalMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedMetalMtlClassDesc class implementation.
//

AppleseedMetalMtlClassDesc::AppleseedMetalMtlClassDesc()
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int AppleseedMetalMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedMetalMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedMetalMtl();
}

const MCHAR* AppleseedMetalMtlClassDesc::ClassName()
{
    return AppleseedMetalMtlFriendlyClassName;
}

SClass_ID AppleseedMetalMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedMetalMtlClassDesc::ClassID()
{
    return AppleseedMetalMtl::get_class_id();
}

const MCHAR* AppleseedMetalMtlClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedMetalMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedMetalMtl";
}

FPInterface* AppleseedMetalMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedMetalMtlClassDesc::HInstance()
{
    return g_module;
}

bool AppleseedMetalMtlClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
