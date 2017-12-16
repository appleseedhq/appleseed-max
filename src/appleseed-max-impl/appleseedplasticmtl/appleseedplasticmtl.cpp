
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
#include "appleseedplasticmtl.h"

// appleseed-max headers.
#include "appleseedplasticmtl/datachunks.h"
#include "appleseedplasticmtl/resource.h"
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
    const wchar_t* AppleseedPlasticMtlFriendlyClassName = L"appleseed Plastic Material";
}

AppleseedPlasticMtlClassDesc g_appleseed_plasticmtl_classdesc;


//
// AppleseedPlasticMtl class implementation.
//

namespace
{
    enum { ParamBlockIdPlasticMtl };
    enum { ParamBlockRefPlasticMtl };

    enum ParamMapId
    {
        ParamMapIdPlastic,
        ParamMapIdBump
    };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdSpecular                 = 0,
        ParamIdSpecularTexmap           = 1,
        ParamIdSpecularWeight           = 2,
        ParamIdSpecularWeightTexmap     = 3,
        ParamIdRoughness                = 4,
        ParamIdRoughnessTexmap          = 5,
        ParamIdHighlightFalloff         = 6,
        ParamIdIOR                      = 7,
        ParamIdDiffuse                  = 8,
        ParamIdDiffuseTexmap            = 9,
        ParamIdDiffuseWeight            = 10,
        ParamIdDiffuseWeightTexmap      = 11,
        ParamIdScattering               = 12,
        ParamIdAlpha                    = 13,
        ParamIdAlphaTexmap              = 14,
        ParamIdBumpMethod               = 15,
        ParamIdBumpTexmap               = 16,
        ParamIdBumpAmount               = 17,
        ParamIdBumpUpVector             = 18,
    };

    enum TexmapId
    {
        // Changing these value WILL break compatibility.
        TexmapIdSpecular                = 0,
        TexmapIdSpecularWeight          = 1,
        TexmapIdRoughness               = 2,
        TexmapIdDiffuse                 = 3,
        TexmapIdDiffuseWeight           = 4,
        TexmapIdAlpha                   = 5,
        TexmapIdBumpMap                 = 6,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        L"Specular",
        L"Specular Weight",
        L"Roughness",
        L"Diffuse",
        L"Diffuse Weight",
        L"Alpha",
        L"Bump Map"
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdSpecularTexmap,
        ParamIdSpecularWeightTexmap,
        ParamIdRoughnessTexmap,
        ParamIdDiffuseTexmap,
        ParamIdDiffuseWeightTexmap,
        ParamIdAlphaTexmap,
        ParamIdBumpTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdPlasticMtl,                     // parameter block's ID
        L"appleseedPlasticMtlParams",               // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_plasticmtl_classdesc,          // class descriptor
        P_AUTO_CONSTRUCT + P_MULTIMAP + P_AUTO_UI,  // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefPlasticMtl,                      // parameter block's reference number

        // --- P_MULTIMAP arguments ---
        2,                                          // number of rollups

        // --- P_AUTO_UI arguments for Plastic rollup ---
        ParamMapIdPlastic,
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

        // --- Parameters specifications for Plastic rollup ---

        ParamIdSpecular, L"specular", TYPE_RGBA, P_ANIMATABLE, IDS_SPECULAR,
            p_default, Color(1.0f, 1.0f, 1.0f),
            p_ui, ParamMapIdPlastic, TYPE_COLORSWATCH, IDC_SWATCH_SPECULAR,
        p_end,
        ParamIdSpecularTexmap, L"specular_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_SPECULAR,
            p_subtexno, TexmapIdSpecular,
            p_ui, ParamMapIdPlastic, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR,
        p_end,

        ParamIdSpecularWeight, L"specular_weight", TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR_WEIGHT,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdPlastic, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR_WEIGHT, IDC_SLIDER_SPECULAR_WEIGHT, 10.0f,
        p_end,
        ParamIdSpecularWeightTexmap, L"specular_weight_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_SPECULAR_WEIGHT,
            p_subtexno, TexmapIdSpecularWeight,
            p_ui, ParamMapIdPlastic, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR_WEIGHT,
        p_end,

        ParamIdRoughness, L"roughness", TYPE_FLOAT, P_ANIMATABLE, IDS_ROUGHNESS,
            p_default, 10.0f,
            p_range, 0.0f, 100.0f,
        p_ui, ParamMapIdPlastic, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ROUGHNESS, IDC_SLIDER_ROUGHNESS, 10.0f,
        p_end,
            ParamIdRoughnessTexmap, L"roughness_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ROUGHNESS,
            p_subtexno, TexmapIdRoughness,
            p_ui, ParamMapIdPlastic, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ROUGHNESS,
        p_end,

        ParamIdHighlightFalloff, L"highlight_falloff", TYPE_FLOAT, P_ANIMATABLE, IDS_HIGHLIGHT_FALLOFF,
            p_default, 40.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdPlastic, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_HIGHLIGHT_FALLOFF, IDC_SLIDER_HIGHLIGHT_FALLOFF, 10.0f,
        p_end,

        ParamIdIOR, L"ior", TYPE_FLOAT, P_ANIMATABLE, IDS_IOR,
            p_default, 1.5f,
            p_range, 1.0f, 2.5f,
            p_ui, ParamMapIdPlastic, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_IOR, IDC_SLIDER_IOR, 0.1f,
        p_end,

        ParamIdDiffuse, L"diffuse", TYPE_RGBA, P_ANIMATABLE, IDS_DIFFUSE,
            p_default, Color(0.5f, 0.5f, 0.5f),
            p_ui, ParamMapIdPlastic, TYPE_COLORSWATCH, IDC_SWATCH_DIFFUSE,
            p_end,
        ParamIdDiffuseTexmap, L"diffuse_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_DIFFUSE,
            p_subtexno, TexmapIdDiffuse,
            p_ui, ParamMapIdPlastic, TYPE_TEXMAPBUTTON, IDC_TEXMAP_DIFFUSE,
        p_end,

        ParamIdDiffuseWeight, L"diffuse_weight", TYPE_FLOAT, P_ANIMATABLE, IDS_DIFFUSE_WEIGHT,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdPlastic, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_DIFFUSE_WEIGHT, IDC_SLIDER_DIFFUSE_WEIGHT, 10.0f,
            p_end,
        ParamIdDiffuseWeightTexmap, L"diffuse_weight_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_DIFFUSE_WEIGHT,
            p_subtexno, TexmapIdDiffuseWeight,
            p_ui, ParamMapIdPlastic, TYPE_TEXMAPBUTTON, IDC_TEXMAP_DIFFUSE_WEIGHT,
        p_end,

        ParamIdScattering, L"scattering", TYPE_FLOAT, P_ANIMATABLE, IDS_SCATTERING,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdPlastic, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SCATTERING, IDC_SLIDER_SCATTERING, 10.0f,
        p_end,

        ParamIdAlpha, L"alpha", TYPE_FLOAT, P_ANIMATABLE, IDS_ALPHA,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdPlastic, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ALPHA, IDC_SLIDER_ALPHA, 10.0f,
        p_end,
        ParamIdAlphaTexmap, L"alpha_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ALPHA,
            p_subtexno, TexmapIdAlpha,
            p_ui, ParamMapIdPlastic, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ALPHA,
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

Class_ID AppleseedPlasticMtl::get_class_id()
{
    return Class_ID(0x2101509a, 0x603c08c0);
}

AppleseedPlasticMtl::AppleseedPlasticMtl()
  : m_pblock(nullptr)
  , m_specular(1.0f, 1.0f, 1.0f)
  , m_specular_texmap(nullptr)
  , m_specular_weight(100.0f)
  , m_specular_weight_texmap(nullptr)
  , m_diffuse(0.5f, 0.5f, 0.5f)
  , m_diffuse_texmap(nullptr)
  , m_diffuse_weight(100.0f)
  , m_diffuse_weight_texmap(nullptr)
  , m_roughness(10.0f)
  , m_roughness_texmap(nullptr)
  , m_highlight_falloff(40.0f)
  , m_ior(1.5f)
  , m_scattering(100.0f)
  , m_alpha(100.0f)
  , m_alpha_texmap(nullptr)
  , m_bump_method(0)
  , m_bump_texmap(nullptr)
  , m_bump_amount(1.0f)
  , m_bump_up_vector(1)
{
    m_params_validity.SetEmpty();

    g_appleseed_plasticmtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedPlasticMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedPlasticMtl::DeleteThis()
{
    delete this;
}

void AppleseedPlasticMtl::GetClassName(TSTR& s)
{
    s = L"appleseedPlasticMtl";
}

SClass_ID AppleseedPlasticMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedPlasticMtl::ClassID()
{
    return get_class_id();
}

int AppleseedPlasticMtl::NumSubs()
{
    return NumRefs();
}

Animatable* AppleseedPlasticMtl::SubAnim(int i)
{
    return GetReference(i);
}

TSTR AppleseedPlasticMtl::SubAnimName(int i)
{
    return i == ParamBlockRefPlasticMtl ? L"Parameters" : L"";
}

int AppleseedPlasticMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedPlasticMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedPlasticMtl::GetParamBlock(int i)
{
    return i == ParamBlockRefPlasticMtl ? m_pblock : nullptr;
}

IParamBlock2* AppleseedPlasticMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedPlasticMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedPlasticMtl::GetReference(int i)
{
    return i == ParamBlockRefPlasticMtl ? m_pblock : nullptr;
}

void AppleseedPlasticMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefPlasticMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedPlasticMtl::NotifyRefChanged(
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

RefTargetHandle AppleseedPlasticMtl::Clone(RemapDir& remap)
{
    AppleseedPlasticMtl* clone = new AppleseedPlasticMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(ParamBlockRefPlasticMtl, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedPlasticMtl::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedPlasticMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedPlasticMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);

    IParamMap2* map = m_pblock->GetMap(ParamMapIdPlastic);
    if (map != nullptr)
    {
        map->SetText(param_id, texmap == nullptr ? L"" : L"M");
    }
}

int AppleseedPlasticMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedPlasticMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedPlasticMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdSpecular, t, m_specular, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularTexmap, t, m_specular_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecularWeight, t, m_specular_weight, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularWeightTexmap, t, m_specular_weight_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdDiffuse, t, m_diffuse, m_params_validity);
        m_pblock->GetValue(ParamIdDiffuseTexmap, t, m_diffuse_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdRoughness, t, m_roughness, m_params_validity);
        m_pblock->GetValue(ParamIdRoughnessTexmap, t, m_roughness_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdDiffuseWeight, t, m_diffuse_weight, m_params_validity);
        m_pblock->GetValue(ParamIdDiffuseWeightTexmap, t, m_diffuse_weight_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdHighlightFalloff, t, m_highlight_falloff, m_params_validity);

        m_pblock->GetValue(ParamIdIOR, t, m_ior, m_params_validity);

        m_pblock->GetValue(ParamIdScattering, t, m_scattering, m_params_validity);

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

void AppleseedPlasticMtl::Reset()
{
    g_appleseed_plasticmtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedPlasticMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedPlasticMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    ParamDlg* param_dialog = g_appleseed_plasticmtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    DbgAssert(m_pblock != nullptr);
    update_map_buttons(m_pblock->GetMap(ParamMapIdPlastic));
    g_block_desc.SetUserDlgProc(ParamMapIdBump, new BumpParamMapDlgProc());
    return param_dialog;
}

IOResult AppleseedPlasticMtl::Save(ISave* isave)
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

IOResult AppleseedPlasticMtl::Load(ILoad* iload)
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

Color AppleseedPlasticMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedPlasticMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return m_diffuse;
}

Color AppleseedPlasticMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedPlasticMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedPlasticMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedPlasticMtl::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedPlasticMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedPlasticMtl::SetDiffuse(Color c, TimeValue t)
{
    Color nc;
    Interval iv;
    m_pblock->SetValue(ParamIdDiffuse, t, c);

    m_pblock->GetValue(ParamIdDiffuse, t, nc, iv);
    m_diffuse = nc;
}

void AppleseedPlasticMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedPlasticMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedPlasticMtl::Shade(ShadeContext& sc)
{
}

int AppleseedPlasticMtl::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool AppleseedPlasticMtl::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> AppleseedPlasticMtl::create_material(
    asr::Assembly&  assembly,
    const char*     name,
    const bool      use_max_procedural_maps)
{
    return
        use_max_procedural_maps
            ? create_builtin_material(assembly, name)
            : create_osl_material(assembly, name);
}

asf::auto_release_ptr<asr::Material> AppleseedPlasticMtl::create_osl_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    //
    // Shader group.
    //

    auto shader_group_name = make_unique_name(assembly.shader_groups(), std::string(name) + "_shader_group");
    auto shader_group = asr::ShaderGroupFactory::create(shader_group_name.c_str());

    connect_color_texture(shader_group.ref(), name, "SpecularColor", m_specular_texmap, m_specular);
    connect_color_texture(shader_group.ref(), name, "DiffuseColor", m_diffuse_texmap, m_diffuse);
    connect_float_texture(shader_group.ref(), name, "SpecularWeight", m_specular_weight_texmap, m_specular_weight / 100.0f);
    connect_float_texture(shader_group.ref(), name, "DiffuseWeight", m_diffuse_weight_texmap, m_diffuse_weight / 100.0f);
    connect_float_texture(shader_group.ref(), name, "Roughness", m_roughness_texmap, m_roughness / 100.0f);

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
    shader_group->add_shader("surface", "as_max_plastic_material", name, 
        asr::ParamArray()
        .insert("Spread", fmt_osl_expr(m_highlight_falloff / 100.0f))
        .insert("Scattering", fmt_osl_expr(m_scattering / 100.0f))
        .insert("IOR", fmt_osl_expr(m_ior)));

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

asf::auto_release_ptr<asr::Material> AppleseedPlasticMtl::create_builtin_material(
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
        bsdf_params.insert("highlight_falloff", m_highlight_falloff / 100.0f);
        bsdf_params.insert("ior", m_ior);
        bsdf_params.insert("internal_scattering", m_scattering / 100.0f);

        // Specular.
        instance_name = insert_texture_and_instance(assembly, m_specular_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("specular_reflectance", instance_name);
        else
        {
            const auto color_name = std::string(name) + "_bsdf_normal_reflectance_color";
            insert_color(assembly, m_specular, color_name.c_str());
            bsdf_params.insert("specular_reflectance", color_name);
        }

        // Diffuse.
        instance_name = insert_texture_and_instance(assembly, m_diffuse_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("diffuse_reflectance", instance_name);
        else
        {
            const auto color_name = std::string(name) + "_bsdf_edge_tint";
            insert_color(assembly, m_diffuse, color_name.c_str());
            bsdf_params.insert("diffuse_reflectance", color_name);
        }

        // Specular weight.
        instance_name = insert_texture_and_instance(assembly, m_specular_weight_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("specular_reflectance_multiplier", instance_name);
        else bsdf_params.insert("specular_reflectance_multiplier", m_specular_weight / 100.0f);

        // Diffuse weight.
        instance_name = insert_texture_and_instance(assembly, m_diffuse_weight_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("diffuse_reflectance_multiplier", instance_name);
        else bsdf_params.insert("diffuse_reflectance_multiplier", m_diffuse_weight / 100.0f);

        // Roughness.
        instance_name = insert_texture_and_instance(assembly, m_roughness_texmap, use_max_procedural_maps);
        if (!instance_name.empty())
            bsdf_params.insert("roughness", instance_name);
        else bsdf_params.insert("roughness", m_roughness / 100.0f);

        // BRDF.
        const auto bsdf_name = std::string(name) + "_bsdf";
        assembly.bsdfs().insert(
            asr::PlasticBRDFFactory().create(bsdf_name.c_str(), bsdf_params));
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
// AppleseedPlasticMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedPlasticMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedPlasticMtlFriendlyClassName;
}

const MCHAR* AppleseedPlasticMtlBrowserEntryInfo::GetEntryCategory() const
{
    return L"Materials\\appleseed";
}

Bitmap* AppleseedPlasticMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedPlasticMtlClassDesc class implementation.
//

AppleseedPlasticMtlClassDesc::AppleseedPlasticMtlClassDesc()
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int AppleseedPlasticMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedPlasticMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedPlasticMtl();
}

const MCHAR* AppleseedPlasticMtlClassDesc::ClassName()
{
    return AppleseedPlasticMtlFriendlyClassName;
}

SClass_ID AppleseedPlasticMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedPlasticMtlClassDesc::ClassID()
{
    return AppleseedPlasticMtl::get_class_id();
}

const MCHAR* AppleseedPlasticMtlClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedPlasticMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedPlasticMtl";
}

FPInterface* AppleseedPlasticMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedPlasticMtlClassDesc::HInstance()
{
    return g_module;
}

bool AppleseedPlasticMtlClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
