
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016-2017 Francois Beaune, The appleseedhq Organization
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
#include "appleseedglassmtl.h"

// appleseed-max headers.
#include "appleseedglassmtl/datachunks.h"
#include "appleseedglassmtl/resource.h"
#include "bump/bumpparammapdlgproc.h"
#include "bump/resource.h"
#include "main.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/bsdf.h"
#include "renderer/api/material.h"
#include "renderer/api/scene.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"
#include "foundation/utility/searchpaths.h"

// 3ds Max headers.
#include <AssetManagement/AssetUser.h>
#include <color.h>
#include <iparamm2.h>
#include <stdmat.h>
#include <strclass.h>
#include <hsv.h>

// Windows headers.
#include <tchar.h>

// Standard headers.
#include <algorithm>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const TCHAR* AppleseedGlassMtlFriendlyClassName = _T("appleseed Glass Material");
}

AppleseedGlassMtlClassDesc g_appleseed_glassmtl_classdesc;


//
// AppleseedGlassMtl class implementation.
//

namespace
{
    enum { ParamBlockIdGlassMtl };
    enum { ParamBlockRefGlassMtl };

    enum MapId
    {
        ParamMapIdGlass,
        ParamMapIdBump
    };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdSurfaceColor         = 0,
        ParamIdSurfaceColorTexmap   = 1,
        ParamIdReflectionTint       = 2,
        ParamIdReflectionTintTexmap = 3,
        ParamIdRefractionTint       = 4,
        ParamIdRefractionTintTexmap = 5,
        ParamIdIOR                  = 6,
        ParamIdRoughness            = 7,
        ParamIdRoughnessTexmap      = 8,
        ParamIdAnisotropy           = 9,
        ParamIdAnisotropyTexmap     = 10,
        ParamIdVolumeColor          = 11,
        ParamIdVolumeColorTexmap    = 12,
        ParamIdScale                = 13,
        ParamIdBumpMethod           = 14,
        ParamIdBumpTexmap           = 15,
        ParamIdBumpAmount           = 16,
        ParamIdBumpUpVector         = 17
    };

    enum TexmapId
    {
        // Changing these value WILL break compatibility.
        TexmapIdSurfaceColor        = 0,
        TexmapIdReflectionTint      = 1,
        TexmapIdRefractionTint      = 2,
        TexmapIdRoughness           = 3,
        TexmapIdAnisotropy          = 4,
        TexmapIdVolumeColor         = 5,
        TexmapIdBumpMap             = 6,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        _T("Surface Color"),
        _T("Reflection Tint")
        _T("Refraction Tint"),
        _T("Roughness"),
        _T("Anisotropy"),
        _T("Volume Color"),
        _T("Bump Map")
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdSurfaceColorTexmap,
        ParamIdReflectionTintTexmap,
        ParamIdRefractionTintTexmap,
        ParamIdRoughnessTexmap,
        ParamIdAnisotropyTexmap,
        ParamIdVolumeColorTexmap,
        ParamIdBumpTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdGlassMtl,                       // parameter block's ID
        _T("appleseedGlassMtlParams"),              // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_glassmtl_classdesc,            // class descriptor
        P_AUTO_CONSTRUCT + P_MULTIMAP + P_AUTO_UI,  // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefGlassMtl,                      // parameter block's reference number

        // --- P_MULTIMAP arguments ---
        2,                                          // number of rollups

        // --- P_AUTO_UI arguments for Glass rollup ---
        ParamMapIdGlass,
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

        // --- Parameters specifications for Glass rollup ---

        ParamIdSurfaceColor, _T("surface_color"), TYPE_RGBA, P_ANIMATABLE, IDS_SURFACE_COLOR,
            p_default, Color(1.0f, 1.0f, 1.0f),
            p_ui, ParamMapIdGlass, TYPE_COLORSWATCH, IDC_SWATCH_SURFACE_COLOR,
        p_end,
        ParamIdSurfaceColorTexmap, _T("surface_color_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SURFACE_COLOR,
            p_subtexno, TexmapIdSurfaceColor,
            p_ui, ParamMapIdGlass, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SURFACE_COLOR,
        p_end,

        ParamIdReflectionTint, _T("reflection_tint"), TYPE_RGBA, P_ANIMATABLE, IDS_REFLECTION_TINT,
            p_default, Color(1.0f, 1.0f, 1.0f),
            p_ui, ParamMapIdGlass, TYPE_COLORSWATCH, IDC_SWATCH_REFLECTION_TINT,
        p_end,
        ParamIdReflectionTintTexmap, _T("reflection_tint_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_REFLECTION_TINT,
            p_subtexno, TexmapIdReflectionTint,
            p_ui, ParamMapIdGlass, TYPE_TEXMAPBUTTON, IDC_TEXMAP_REFLECTION_TINT,
        p_end,

        ParamIdRefractionTint, _T("refraction_tint"), TYPE_RGBA, P_ANIMATABLE, IDS_REFRACTION_TINT,
            p_default, Color(1.0f, 1.0f, 1.0f),
            p_ui, ParamMapIdGlass, TYPE_COLORSWATCH, IDC_SWATCH_REFRACTION_TINT,
        p_end,
        ParamIdRefractionTintTexmap, _T("refraction_tint_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_REFRACTION_TINT,
            p_subtexno, TexmapIdRefractionTint,
            p_ui, ParamMapIdGlass, TYPE_TEXMAPBUTTON, IDC_TEXMAP_REFRACTION_TINT,
        p_end,

        ParamIdIOR, _T("ior"), TYPE_FLOAT, P_ANIMATABLE, IDS_IOR,
            p_default, 1.5f,
            p_range, 1.0f, 4.0f,
            p_ui, ParamMapIdGlass, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_IOR, IDC_SLIDER_IOR, 0.1f,
        p_end,

        ParamIdRoughness, _T("roughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_ROUGHNESS,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdGlass, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ROUGHNESS, IDC_SLIDER_ROUGHNESS, 10.0f,
        p_end,
        ParamIdRoughnessTexmap, _T("roughness_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_ROUGHNESS,
            p_subtexno, TexmapIdRoughness,
            p_ui, ParamMapIdGlass, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ROUGHNESS,
        p_end,

        ParamIdAnisotropy, _T("anisotropy"), TYPE_FLOAT, P_ANIMATABLE, IDS_ANISOTROPY,
            p_default, 0.0f,
            p_range, -1.0f, 1.0f,
            p_ui, ParamMapIdGlass, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ANISOTROPY, IDC_SLIDER_ANISOTROPY, 0.1f,
        p_end,
        ParamIdAnisotropyTexmap, _T("anisotropy_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_ANISOTROPY,
            p_subtexno, TexmapIdAnisotropy,
            p_ui, ParamMapIdGlass, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ANISOTROPY,
        p_end,

        ParamIdVolumeColor, _T("volume_color"), TYPE_RGBA, P_ANIMATABLE, IDS_VOLUME_COLOR,
            p_default, Color(1.0f, 1.0f, 1.0f),
            p_ui, ParamMapIdGlass, TYPE_COLORSWATCH, IDC_SWATCH_VOLUME_COLOR,
        p_end,
        ParamIdVolumeColorTexmap, _T("volume_color_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_VOLUME_COLOR,
            p_subtexno, TexmapIdVolumeColor,
            p_ui, ParamMapIdGlass, TYPE_TEXMAPBUTTON, IDC_TEXMAP_VOLUME_COLOR,
        p_end,

        ParamIdScale, _T("scale"), TYPE_FLOAT, P_ANIMATABLE, IDS_SCALE,
            p_default, 0.0f,
            p_range, 0.0f, 1000000.0f,
            p_ui, ParamMapIdGlass, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_SCALE, IDC_SPINNER_SCALE, SPIN_AUTOSCALE,
        p_end,

        // --- Parameters specifications for Bump rollup ---

        ParamIdBumpMethod, _T("bump_method"), TYPE_INT, 0, IDS_BUMP_METHOD,
            p_ui, ParamMapIdBump, TYPE_INT_COMBOBOX, IDC_COMBO_BUMP_METHOD,
            2, IDS_COMBO_BUMP_METHOD_BUMPMAP, IDS_COMBO_BUMP_METHOD_NORMALMAP,
            p_vals, 0, 1,
            p_default, 0,
        p_end,

        ParamIdBumpTexmap, _T("bump_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_BUMP_MAP,
            p_subtexno, TexmapIdBumpMap,
            p_ui, ParamMapIdBump, TYPE_TEXMAPBUTTON, IDC_TEXMAP_BUMP_MAP,
        p_end,

        ParamIdBumpAmount, _T("bump_amount"), TYPE_FLOAT, P_ANIMATABLE, IDS_BUMP_AMOUNT,
            p_default, 1.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdBump, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_BUMP_AMOUNT, IDC_SPINNER_BUMP_AMOUNT, SPIN_AUTOSCALE,
        p_end,

        ParamIdBumpUpVector, _T("bump_up_vector"), TYPE_INT, 0, IDS_BUMP_UP_VECTOR,
            p_ui, ParamMapIdBump, TYPE_INT_COMBOBOX, IDC_COMBO_BUMP_UP_VECTOR,
            2, IDS_COMBO_BUMP_UP_VECTOR_Y, IDS_COMBO_BUMP_UP_VECTOR_Z,
            p_vals, 0, 1,
            p_default, 1,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedGlassMtl::get_class_id()
{
    return Class_ID(0x6f1a3138, 0x417230b5);
}

AppleseedGlassMtl::AppleseedGlassMtl()
  : m_pblock(nullptr)
  , m_surface_color(1.0f, 1.0f, 1.0f)
  , m_surface_color_texmap(nullptr)
  , m_reflection_tint(1.0f, 1.0f, 1.0f)
  , m_reflection_tint_texmap(nullptr)
  , m_refraction_tint(1.0f, 1.0f, 1.0f)
  , m_refraction_tint_texmap(nullptr)
  , m_ior(1.5f)
  , m_roughness(0.0f)
  , m_roughness_texmap(nullptr)
  , m_anisotropy(0.0f)
  , m_anisotropy_texmap(nullptr)
  , m_volume_color(1.0f, 1.0f, 1.0f)
  , m_volume_color_texmap(nullptr)
  , m_scale(0.0f)
  , m_bump_method(0)
  , m_bump_texmap(nullptr)
  , m_bump_amount(1.0f)
  , m_bump_up_vector(1)
{
    m_params_validity.SetEmpty();

    g_appleseed_glassmtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedGlassMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedGlassMtl::DeleteThis()
{
    delete this;
}

void AppleseedGlassMtl::GetClassName(TSTR& s)
{
    s = _T("appleseedGlassMtl");
}

SClass_ID AppleseedGlassMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedGlassMtl::ClassID()
{
    return get_class_id();
}

int AppleseedGlassMtl::NumSubs()
{
    return 1;
}

Animatable* AppleseedGlassMtl::SubAnim(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

TSTR AppleseedGlassMtl::SubAnimName(int i)
{
    return i == 0 ? _T("Parameters") : _T("");
}

int AppleseedGlassMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedGlassMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedGlassMtl::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* AppleseedGlassMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedGlassMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedGlassMtl::GetReference(int i)
{
    return i == ParamBlockRefGlassMtl ? m_pblock : nullptr;
}

void AppleseedGlassMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefGlassMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedGlassMtl::NotifyRefChanged(
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

RefTargetHandle AppleseedGlassMtl::Clone(RemapDir& remap)
{
    AppleseedGlassMtl* clone = new AppleseedGlassMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(0, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedGlassMtl::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedGlassMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedGlassMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);
}

int AppleseedGlassMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedGlassMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedGlassMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdSurfaceColor, t, m_surface_color, m_params_validity);
        m_pblock->GetValue(ParamIdSurfaceColorTexmap, t, m_surface_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdReflectionTint, t, m_reflection_tint, m_params_validity);
        m_pblock->GetValue(ParamIdReflectionTintTexmap, t, m_reflection_tint_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdRefractionTint, t, m_refraction_tint, m_params_validity);
        m_pblock->GetValue(ParamIdRefractionTintTexmap, t, m_refraction_tint_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdIOR, t, m_ior, m_params_validity);

        m_pblock->GetValue(ParamIdRoughness, t, m_roughness, m_params_validity);
        m_pblock->GetValue(ParamIdRoughnessTexmap, t, m_roughness_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdAnisotropy, t, m_anisotropy, m_params_validity);
        m_pblock->GetValue(ParamIdAnisotropyTexmap, t, m_anisotropy_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdVolumeColor, t, m_volume_color, m_params_validity);
        m_pblock->GetValue(ParamIdVolumeColorTexmap, t, m_volume_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdScale, t, m_scale, m_params_validity);

        m_pblock->GetValue(ParamIdBumpMethod, t, m_bump_method, m_params_validity);
        m_pblock->GetValue(ParamIdBumpTexmap, t, m_bump_texmap, m_params_validity);
        m_pblock->GetValue(ParamIdBumpAmount, t, m_bump_amount, m_params_validity);
        m_pblock->GetValue(ParamIdBumpUpVector, t, m_bump_up_vector, m_params_validity);

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }

    valid &= m_params_validity;
}

void AppleseedGlassMtl::Reset()
{
    g_appleseed_glassmtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedGlassMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedGlassMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    ParamDlg* param_dialog = g_appleseed_glassmtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    g_block_desc.SetUserDlgProc(ParamMapIdBump, new BumpParamMapDlgProc());
    return param_dialog;
}

IOResult AppleseedGlassMtl::Save(ISave* isave)
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

IOResult AppleseedGlassMtl::Load(ILoad* iload)
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

Color AppleseedGlassMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedGlassMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return m_surface_color;
}

Color AppleseedGlassMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedGlassMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedGlassMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedGlassMtl::GetXParency(int mtlNum, BOOL backFace)
{
    Color hsv = RGBtoHSV (m_refraction_tint);
    return hsv.b;
}

void AppleseedGlassMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedGlassMtl::SetDiffuse(Color c, TimeValue t)
{
}

void AppleseedGlassMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedGlassMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedGlassMtl::Shade(ShadeContext& sc)
{
}

int AppleseedGlassMtl::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool AppleseedGlassMtl::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> AppleseedGlassMtl::create_material(asr::Assembly& assembly, const char* name)
{
    asr::ParamArray material_params;

    //
    // BRDF.
    //

    {
        asr::ParamArray bsdf_params;
        bsdf_params.insert("mdf", "ggx");

        // Surface transmittance.
        if (is_bitmap_texture(m_surface_color_texmap))
            bsdf_params.insert("surface_transmittance", insert_texture_and_instance(assembly, m_surface_color_texmap));
        else
        {
            const auto color_name = std::string(name) + "_bsdf_surface_transmittance";
            insert_color(assembly, m_surface_color, color_name.c_str());
            bsdf_params.insert("surface_transmittance", color_name);
        }

        // Reflection tint.
        if (is_bitmap_texture(m_reflection_tint_texmap))
            bsdf_params.insert("reflection_tint", insert_texture_and_instance(assembly, m_reflection_tint_texmap));
        else
        {
            const auto color_name = std::string(name) + "_bsdf_reflection_tint";
            insert_color(assembly, m_reflection_tint, color_name.c_str());
            bsdf_params.insert("reflection_tint", color_name);
        }

        // Refraction tint.
        if (is_bitmap_texture(m_refraction_tint_texmap))
            bsdf_params.insert("refraction_tint", insert_texture_and_instance(assembly, m_refraction_tint_texmap));
        else
        {
            const auto color_name = std::string(name) + "_bsdf_refraction_tint";
            insert_color(assembly, m_refraction_tint, color_name.c_str());
            bsdf_params.insert("refraction_tint", color_name);
        }

        // IOR.
        bsdf_params.insert("ior", m_ior);

        // Roughness.
        if (is_bitmap_texture(m_roughness_texmap))
            bsdf_params.insert("roughness", insert_texture_and_instance(assembly, m_roughness_texmap));
        else bsdf_params.insert("roughness", m_roughness / 100.0f);

        // Anisotropy.
        if (is_bitmap_texture(m_anisotropy_texmap))
            bsdf_params.insert("anisotropic", insert_texture_and_instance(assembly, m_anisotropy_texmap));
        else bsdf_params.insert("anisotropic", m_anisotropy);

        // Volume transmittance.
        if (is_bitmap_texture(m_volume_color_texmap))
            bsdf_params.insert("volume_transmittance", insert_texture_and_instance(assembly, m_volume_color_texmap));
        else
        {
            const auto color_name = std::string(name) + "_bsdf_volume_transmittance";
            insert_color(assembly, m_volume_color, color_name.c_str());
            bsdf_params.insert("volume_transmittance", color_name);
        }

        // Volume transmittance distance.
        bsdf_params.insert("volume_transmittance_distance", m_scale);

        const auto bsdf_name = std::string(name) + "_bsdf";
        assembly.bsdfs().insert(
            asr::GlassBSDFFactory::static_create(bsdf_name.c_str(), bsdf_params));
        material_params.insert("bsdf", bsdf_name);
    }

    //
    // Material.
    //

    if (is_bitmap_texture(m_bump_texmap))
    {
        material_params.insert("displacement_method", m_bump_method == 0 ? "bump" : "normal");
        material_params.insert(
            "displacement_map",
            insert_texture_and_instance(
                assembly,
                m_bump_texmap,
                asr::ParamArray()
                    .insert("color_space", "linear_rgb")));

        switch (m_bump_method)
        {
          case 0:
            material_params.insert("bump_amplitude", m_bump_amount);
            break;

          case 1:
            material_params.insert("normal_map_up", m_bump_up_vector == 0 ? "y" : "z");
            break;
        }
    }

    return asr::GenericMaterialFactory::static_create(name, material_params);
}


//
// AppleseedGlassMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedGlassMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedGlassMtlFriendlyClassName;
}

const MCHAR* AppleseedGlassMtlBrowserEntryInfo::GetEntryCategory() const
{
    return _T("Materials\\appleseed");
}

Bitmap* AppleseedGlassMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedGlassMtlClassDesc class implementation.
//

int AppleseedGlassMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedGlassMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedGlassMtl();
}

const MCHAR* AppleseedGlassMtlClassDesc::ClassName()
{
    return AppleseedGlassMtlFriendlyClassName;
}

SClass_ID AppleseedGlassMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedGlassMtlClassDesc::ClassID()
{
    return AppleseedGlassMtl::get_class_id();
}

const MCHAR* AppleseedGlassMtlClassDesc::Category()
{
    return _T("");
}

const MCHAR* AppleseedGlassMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return _T("appleseedGlassMtl");
}

FPInterface* AppleseedGlassMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedGlassMtlClassDesc::HInstance()
{
    return g_module;
}
