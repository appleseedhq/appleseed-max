
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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
#include "appleseeddisneymtl.h"

// appleseed-max headers.
#include "appleseeddisneymtl/datachunks.h"
#include "appleseeddisneymtl/resource.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "bump/bumpparammapdlgproc.h"
#include "bump/resource.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
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
    const wchar_t* AppleseedDisneyMtlFriendlyClassName = L"appleseed Disney Material";
}

AppleseedDisneyMtlClassDesc g_appleseed_disneymtl_classdesc;


//
// AppleseedDisneyMtl class implementation.
//

namespace
{
    enum { ParamBlockIdDisneyMtl };
    enum { ParamBlockRefDisneyMtl };

    enum ParamMapId
    {
        ParamMapIdDisney,
        ParamMapIdBump
    };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdBaseColor            = 0,
        ParamIdBaseColorTexmap      = 1,
        ParamIdMetallic             = 2,
        ParamIdMetallicTexmap       = 3,
        ParamIdSpecular             = 4,
        ParamIdSpecularTexmap       = 5,
        ParamIdSpecularTint         = 6,
        ParamIdSpecularTintTexmap   = 7,
        ParamIdRoughness            = 8,
        ParamIdRoughnessTexmap      = 9,
        ParamIdAnisotropy           = 10,
        ParamIdAnisotropyTexmap     = 11,
        ParamIdClearcoat            = 12,
        ParamIdClearcoatTexmap      = 13,
        ParamIdClearcoatGloss       = 14,
        ParamIdClearcoatGlossTexmap = 15,
        ParamIdAlpha                = 16,
        ParamIdAlphaTexmap          = 17,
        ParamIdBumpMethod           = 18,
        ParamIdBumpTexmap           = 19,
        ParamIdBumpAmount           = 20,
        ParamIdBumpUpVector         = 21,
        ParamIdSheen                = 22,
        ParamIdSheenTexmap          = 23,
        ParamIdSheenTint            = 24,
        ParamIdSheenTintTexmap      = 25
    };

    enum TexmapId
    {
        // Changing these value WILL break compatibility.
        TexmapIdBase                = 0,
        TexmapIdMetallic            = 1,
        TexmapIdSpecular            = 2,
        TexmapIdSpecularTint        = 3,
        TexmapIdRoughness           = 4,
        TexmapIdSheen               = 5,
        TexmapIdSheenTint           = 6,
        TexmapIdAnisotropy          = 7,
        TexmapIdClearcoat           = 8,
        TexmapIdClearcoatGloss      = 9,
        TexmapIdAlpha               = 10,
        TexmapIdBumpMap             = 11,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        L"Base Color",
        L"Metallic",
        L"Specular",
        L"Specular Tint",
        L"Roughness",
        L"Sheen",
        L"Sheen Tint",
        L"Anisotropy",
        L"Clearcoat",
        L"Clearcoat Gloss",
        L"Alpha",
        L"Bump Map"
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdBaseColorTexmap,
        ParamIdMetallicTexmap,
        ParamIdSpecularTexmap,
        ParamIdSpecularTintTexmap,
        ParamIdRoughnessTexmap,
        ParamIdSheenTexmap,
        ParamIdSheenTintTexmap,
        ParamIdAnisotropyTexmap,
        ParamIdClearcoatTexmap,
        ParamIdClearcoatGlossTexmap,
        ParamIdAlphaTexmap,
        ParamIdBumpTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdDisneyMtl,                      // parameter block's ID
        L"appleseedDisneyMtlParams",                // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_disneymtl_classdesc,           // class descriptor
        P_AUTO_CONSTRUCT + P_MULTIMAP + P_AUTO_UI,  // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefDisneyMtl,                     // parameter block's reference number

        // --- P_MULTIMAP arguments ---
        2,                                          // number of rollups

        // --- P_AUTO_UI arguments for Disney rollup ---
        ParamMapIdDisney,
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

        // --- Parameters specifications for Disney rollup ---

        ParamIdBaseColor, L"base_color", TYPE_RGBA, P_ANIMATABLE, IDS_BASE_COLOR,
            p_default, Color(0.9f, 0.9f, 0.9f),
            p_ui, ParamMapIdDisney, TYPE_COLORSWATCH, IDC_SWATCH_BASE_COLOR,
        p_end,
        ParamIdBaseColorTexmap, L"base_color_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_BASE_COLOR,
            p_subtexno, TexmapIdBase,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_BASE_COLOR,
        p_end,

        ParamIdMetallic, L"metallic", TYPE_FLOAT, P_ANIMATABLE, IDS_METALLIC,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_METALLIC, IDC_SLIDER_METALLIC, 10.0f,
        p_end,
        ParamIdMetallicTexmap, L"metallic_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_METALLIC,
            p_subtexno, TexmapIdMetallic,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_METALLIC,
        p_end,

        ParamIdSpecular, L"specular", TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR,
            p_default, 90.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR, IDC_SLIDER_SPECULAR, 10.0f,
        p_end,
        ParamIdSpecularTexmap, L"specular_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_SPECULAR,
            p_subtexno, TexmapIdSpecular,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR,
        p_end,

        ParamIdSpecularTint, L"specular_tint", TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR_TINT,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR_TINT, IDC_SLIDER_SPECULAR_TINT, 10.0f,
        p_end,
        ParamIdSpecularTintTexmap, L"specular_tint_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_SPECULAR_TINT,
            p_subtexno, TexmapIdSpecularTint,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR_TINT,
        p_end,

        ParamIdRoughness, L"roughness", TYPE_FLOAT, P_ANIMATABLE, IDS_ROUGHNESS,
            p_default, 40.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ROUGHNESS, IDC_SLIDER_ROUGHNESS, 10.0f,
        p_end,
        ParamIdRoughnessTexmap, L"roughness_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ROUGHNESS,
            p_subtexno, TexmapIdRoughness,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ROUGHNESS,
        p_end,

        ParamIdSheen, L"sheen", TYPE_FLOAT, P_ANIMATABLE, IDS_SHEEN,
            p_default, 0.0f,
            p_range, 0.0f, 1000.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SHEEN, IDC_SLIDER_SHEEN, 10.0f,
        p_end,
        ParamIdSheenTexmap, L"sheen_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_SHEEN,
            p_subtexno, TexmapIdSheen,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SHEEN,
        p_end,

        ParamIdSheenTint, L"sheen_tint", TYPE_FLOAT, P_ANIMATABLE, IDS_SHEEN_TINT,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SHEEN_TINT, IDC_SLIDER_SHEEN_TINT, 10.0f,
        p_end,
        ParamIdSheenTintTexmap, L"sheen_tint_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_SHEEN_TINT,
            p_subtexno, TexmapIdSheenTint,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SHEEN_TINT,
        p_end,

        ParamIdAnisotropy, L"anisotropy", TYPE_FLOAT, P_ANIMATABLE, IDS_ANISOTROPY,
            p_default, 0.0f,
            p_range, -1.0f, 1.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ANISOTROPY, IDC_SLIDER_ANISOTROPY, 0.1f,
        p_end,
        ParamIdAnisotropyTexmap, L"anisotropy_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ANISOTROPY,
            p_subtexno, TexmapIdAnisotropy,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ANISOTROPY,
        p_end,

        ParamIdClearcoat, L"clearcoat", TYPE_FLOAT, P_ANIMATABLE, IDS_CLEARCOAT,
            p_default, 0.0f,
            p_range, 0.0f, 1000.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_CLEARCOAT, IDC_SLIDER_CLEARCOAT, 10.0f,
        p_end,
        ParamIdClearcoatTexmap, L"clearcoat_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_CLEARCOAT,
            p_subtexno, TexmapIdClearcoat,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_CLEARCOAT,
        p_end,

        ParamIdClearcoatGloss, L"clearcoat_gloss", TYPE_FLOAT, P_ANIMATABLE, IDS_CLEARCOAT_GLOSS,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_CLEARCOAT_GLOSS, IDC_SLIDER_CLEARCOAT_GLOSS, 10.0f,
        p_end,
        ParamIdClearcoatGlossTexmap, L"clearcoat_gloss_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_CLEARCOAT_GLOSS,
            p_subtexno, TexmapIdClearcoatGloss,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_CLEARCOAT_GLOSS,
        p_end,

        ParamIdAlpha, L"alpha", TYPE_FLOAT, P_ANIMATABLE, IDS_ALPHA,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, ParamMapIdDisney, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ALPHA, IDC_SLIDER_ALPHA, 10.0f,
        p_end,
        ParamIdAlphaTexmap, L"alpha_texmap", TYPE_TEXMAP, P_NO_AUTO_LABELS, IDS_TEXMAP_ALPHA,
            p_subtexno, TexmapIdAlpha,
            p_ui, ParamMapIdDisney, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ALPHA,
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

Class_ID AppleseedDisneyMtl::get_class_id()
{
    return Class_ID(0x331b1ff7, 0x16381b67);
}

AppleseedDisneyMtl::AppleseedDisneyMtl()
  : m_pblock(nullptr)
  , m_base_color(0.9f, 0.9f, 0.9f)
  , m_base_color_texmap(nullptr)
  , m_metallic(0.0f)
  , m_metallic_texmap(nullptr)
  , m_specular(90.0f)
  , m_specular_texmap(nullptr)
  , m_specular_tint(0.0f)
  , m_specular_tint_texmap(nullptr)
  , m_roughness(40.0f)
  , m_roughness_texmap(nullptr)
  , m_sheen(0.0f)
  , m_sheen_texmap(nullptr)
  , m_sheen_tint(0.0f)
  , m_sheen_tint_texmap(nullptr)
  , m_anisotropy(0.0f)
  , m_anisotropy_texmap(nullptr)
  , m_clearcoat(0.0f)
  , m_clearcoat_texmap(nullptr)
  , m_clearcoat_gloss(0.0f)
  , m_clearcoat_gloss_texmap(nullptr)
  , m_alpha(100.0f)
  , m_alpha_texmap(nullptr)
  , m_bump_method(0)
  , m_bump_texmap(nullptr)
  , m_bump_amount(1.0f)
  , m_bump_up_vector(1)
{
    m_params_validity.SetEmpty();

    g_appleseed_disneymtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedDisneyMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedDisneyMtl::DeleteThis()
{
    delete this;
}

void AppleseedDisneyMtl::GetClassName(TSTR& s)
{
    s = L"appleseedDisneyMtl";
}

SClass_ID AppleseedDisneyMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedDisneyMtl::ClassID()
{
    return get_class_id();
}

int AppleseedDisneyMtl::NumSubs()
{
    return NumRefs();
}

Animatable* AppleseedDisneyMtl::SubAnim(int i)
{
    return GetReference(i);
}

TSTR AppleseedDisneyMtl::SubAnimName(int i)
{
    return i == ParamBlockRefDisneyMtl ? L"Parameters" : L"";
}

int AppleseedDisneyMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedDisneyMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedDisneyMtl::GetParamBlock(int i)
{
    return i == ParamBlockRefDisneyMtl ? m_pblock : nullptr;
}

IParamBlock2* AppleseedDisneyMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedDisneyMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedDisneyMtl::GetReference(int i)
{
    return i == ParamBlockRefDisneyMtl ? m_pblock : nullptr;
}

void AppleseedDisneyMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefDisneyMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedDisneyMtl::NotifyRefChanged(
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

RefTargetHandle AppleseedDisneyMtl::Clone(RemapDir& remap)
{
    AppleseedDisneyMtl* clone = new AppleseedDisneyMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(ParamBlockRefDisneyMtl, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedDisneyMtl::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedDisneyMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedDisneyMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);

    IParamMap2* map = m_pblock->GetMap(ParamMapIdDisney);
    if (map != nullptr)
    {
        map->SetText(param_id, texmap == nullptr ? L"" : L"M");
    }
}

int AppleseedDisneyMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedDisneyMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedDisneyMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdBaseColor, t, m_base_color, m_params_validity);
        m_pblock->GetValue(ParamIdBaseColorTexmap, t, m_base_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdMetallic, t, m_metallic, m_params_validity);
        m_pblock->GetValue(ParamIdMetallicTexmap, t, m_metallic_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecular, t, m_specular, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularTexmap, t, m_specular_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecularTint, t, m_specular_tint, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularTintTexmap, t, m_specular_tint_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdRoughness, t, m_roughness, m_params_validity);
        m_pblock->GetValue(ParamIdRoughnessTexmap, t, m_roughness_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSheen, t, m_sheen, m_params_validity);
        m_pblock->GetValue(ParamIdSheenTexmap, t, m_sheen_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSheenTint, t, m_sheen_tint, m_params_validity);
        m_pblock->GetValue(ParamIdSheenTintTexmap, t, m_sheen_tint_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdAnisotropy, t, m_anisotropy, m_params_validity);
        m_pblock->GetValue(ParamIdAnisotropyTexmap, t, m_anisotropy_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdClearcoat, t, m_clearcoat, m_params_validity);
        m_pblock->GetValue(ParamIdClearcoatTexmap, t, m_clearcoat_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdClearcoatGloss, t, m_clearcoat_gloss, m_params_validity);
        m_pblock->GetValue(ParamIdClearcoatGlossTexmap, t, m_clearcoat_gloss_texmap, m_params_validity);

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

void AppleseedDisneyMtl::Reset()
{
    g_appleseed_disneymtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedDisneyMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedDisneyMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    ParamDlg* param_dialog = g_appleseed_disneymtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    DbgAssert(m_pblock != nullptr);
    update_map_buttons(m_pblock->GetMap(ParamMapIdDisney));
    g_block_desc.SetUserDlgProc(ParamMapIdBump, new BumpParamMapDlgProc());
    return param_dialog;
}

IOResult AppleseedDisneyMtl::Save(ISave* isave)
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

IOResult AppleseedDisneyMtl::Load(ILoad* iload)
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

Color AppleseedDisneyMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedDisneyMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return m_base_color;
}

Color AppleseedDisneyMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedDisneyMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedDisneyMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedDisneyMtl::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedDisneyMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedDisneyMtl::SetDiffuse(Color c, TimeValue t)
{
    m_pblock->SetValue(ParamIdBaseColor, t, c);
    m_base_color = c;
}

void AppleseedDisneyMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedDisneyMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedDisneyMtl::Shade(ShadeContext& sc)
{
}

int AppleseedDisneyMtl::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool AppleseedDisneyMtl::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> AppleseedDisneyMtl::create_material(
    asr::Assembly&  assembly,
    const char*     name,
    const bool      use_max_procedural_maps,
    const TimeValue time)
{
    return
        use_max_procedural_maps
            ? create_builtin_material(assembly, name, time)
            : create_osl_material(assembly, name, time);
}

asf::auto_release_ptr<asr::Material> AppleseedDisneyMtl::create_osl_material(
    asr::Assembly&  assembly,
    const char*     name,
    const TimeValue time)
{
    //
    // Shader group.
    //

    auto shader_group_name = make_unique_name(assembly.shader_groups(), std::string(name) + "_shader_group");
    auto shader_group = asr::ShaderGroupFactory::create(shader_group_name.c_str());

    connect_color_texture(shader_group.ref(), name, "BaseColor", m_base_color_texmap, m_base_color, time);
    connect_float_texture(shader_group.ref(), name, "Metallic", m_metallic_texmap, m_metallic / 100.0f, time);
    connect_float_texture(shader_group.ref(), name, "Specular", m_specular_texmap, m_specular / 100.0f, time);
    connect_float_texture(shader_group.ref(), name, "SpecularTint", m_specular_tint_texmap, m_specular_tint / 100.0f, time);
    connect_float_texture(shader_group.ref(), name, "Roughness", m_roughness_texmap, m_roughness / 100.0f, time);
    connect_float_texture(shader_group.ref(), name, "Sheen", m_sheen_texmap, m_sheen / 100.0f, time);
    connect_float_texture(shader_group.ref(), name, "SheenTint", m_sheen_tint_texmap, m_sheen_tint / 100.0f, time);
    connect_float_texture(shader_group.ref(), name, "Anisotropic", m_anisotropy_texmap, m_anisotropy, time);
    connect_float_texture(shader_group.ref(), name, "Clearcoat", m_clearcoat_texmap, m_clearcoat / 100.0f, time);
    connect_float_texture(shader_group.ref(), name, "ClearcoatGloss", m_clearcoat_gloss_texmap, m_clearcoat_gloss / 100.0f, time);

    if (m_bump_texmap != nullptr)
    {
        if (m_bump_method == 0)
        {
            // Bump mapping.
            connect_bump_map(shader_group.ref(), name, "Normal", "Tn", m_bump_texmap, m_bump_amount, time);
        }
        else
        {
            // Normal mapping.
            connect_normal_map(shader_group.ref(), name, "Normal", "Tn", m_bump_texmap, m_bump_up_vector, m_bump_amount, time);
        }
    }

    shader_group->add_shader("surface", "as_max_disney_material", name, 
        asr::ParamArray()
            .insert("BaseColor", fmt_osl_expr(to_color3f(m_base_color)))
            .insert("Metallic", fmt_osl_expr(m_metallic / 100.0f))
            .insert("Specular", fmt_osl_expr(m_specular / 100.0f))
            .insert("SpecularTint", fmt_osl_expr(m_specular_tint / 100.0f))
            .insert("Roughness", fmt_osl_expr(m_roughness / 100.0f))
            .insert("Sheen", fmt_osl_expr(m_sheen / 100.0f))
            .insert("SheenTint", fmt_osl_expr(m_sheen_tint / 100.0f))
            .insert("Anisotropic", fmt_osl_expr(m_anisotropy))
            .insert("Clearcoat", fmt_osl_expr(m_clearcoat / 100.0f))
            .insert("ClearcoatGloss", fmt_osl_expr(m_clearcoat_gloss / 100.0f)));

    std::string closure2surface_name = asf::format("{0}_closure2surface", name);
    shader_group.ref().add_shader("shader", "as_max_closure2surface", closure2surface_name.c_str(), asr::ParamArray());

    shader_group.ref().add_connection(
        name,
        "ClosureOut",
        closure2surface_name.c_str(),
        "in_input");

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
            time,
            asr::ParamArray(),
            asr::ParamArray()
                .insert("alpha_mode", "detect"));
    if (!instance_name.empty())
        material_params.insert("alpha_map", instance_name);
    else material_params.insert("alpha_map", m_alpha / 100.0f);

    return asr::OSLMaterialFactory().create(name, material_params);
}

asf::auto_release_ptr<asr::Material> AppleseedDisneyMtl::create_builtin_material(
    asr::Assembly&      assembly,
    const char*         name,
    const TimeValue     time)
{
    asr::ParamArray material_params;
    std::string instance_name;
    const bool use_max_procedural_maps = true;

    //
    // BRDF.
    //

    {
        asr::ParamArray bsdf_params;

        // Base color.
        instance_name = insert_texture_and_instance(assembly, m_base_color_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("base_color", instance_name);
        else
        {
            const auto color_name = std::string(name) + "_bsdf_base_color";
            insert_color(assembly, m_base_color, color_name.c_str());
            bsdf_params.insert("base_color", color_name);
        }

        // Specular Tint.
        instance_name = insert_texture_and_instance(assembly, m_specular_tint_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("specular_tint", instance_name);
        else bsdf_params.insert("specular_tint", m_specular_tint / 100.0f);

        // Sheen.
        instance_name = insert_texture_and_instance(assembly, m_sheen_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("sheen", instance_name);
        else bsdf_params.insert("sheen", m_sheen / 1000.0f);

        // Sheen Tint.
        instance_name = insert_texture_and_instance(assembly, m_sheen_tint_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("sheen_tint", instance_name);
        else bsdf_params.insert("sheen_tint", m_sheen_tint / 100.0f);

        // Clearcoat.
        instance_name = insert_texture_and_instance(assembly, m_clearcoat_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("clearcoat", instance_name);
        else bsdf_params.insert("clearcoat", m_clearcoat / 1000.0f);

        // Clearcoat Gloss.
        instance_name = insert_texture_and_instance(assembly, m_clearcoat_gloss_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("clearcoat", instance_name);
        else bsdf_params.insert("clearcoat", m_clearcoat_gloss / 100.0f);

        // Specular.
        instance_name = insert_texture_and_instance(assembly, m_specular_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("specular", instance_name);
        else bsdf_params.insert("specular", m_specular / 100.0f);

        // Metallic.
        instance_name = insert_texture_and_instance(assembly, m_metallic_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("metallic", instance_name);
        else bsdf_params.insert("metallic", m_metallic / 100.0f);

        // Roughness.
        instance_name = insert_texture_and_instance(assembly, m_roughness_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("roughness", instance_name);
        else bsdf_params.insert("roughness", m_roughness / 100.0f);

        // Anisotropic.
        instance_name = insert_texture_and_instance(assembly, m_anisotropy_texmap, use_max_procedural_maps, time);
        if (!instance_name.empty())
            bsdf_params.insert("anisotropic", instance_name);
        else bsdf_params.insert("anisotropic", m_anisotropy);

        // BRDF.
        const auto bsdf_name = std::string(name) + "_bsdf";
        assembly.bsdfs().insert(
            asr::DisneyBRDFFactory().create(bsdf_name.c_str(), bsdf_params));
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
        time,
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
        time,
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
            material_params.insert("bump_offset", 0.5f);
            break;

          case 1:
            material_params.insert("normal_map_up", m_bump_up_vector == 0 ? "y" : "z");
            break;
        }
    }

    return asr::GenericMaterialFactory().create(name, material_params);
}


//
// AppleseedDisneyMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedDisneyMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedDisneyMtlFriendlyClassName;
}

const MCHAR* AppleseedDisneyMtlBrowserEntryInfo::GetEntryCategory() const
{
    return L"Materials\\appleseed";
}

Bitmap* AppleseedDisneyMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedDisneyMtlClassDesc class implementation.
//

AppleseedDisneyMtlClassDesc::AppleseedDisneyMtlClassDesc()
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int AppleseedDisneyMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedDisneyMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedDisneyMtl();
}

const MCHAR* AppleseedDisneyMtlClassDesc::ClassName()
{
    return AppleseedDisneyMtlFriendlyClassName;
}

SClass_ID AppleseedDisneyMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedDisneyMtlClassDesc::ClassID()
{
    return AppleseedDisneyMtl::get_class_id();
}

const MCHAR* AppleseedDisneyMtlClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedDisneyMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedDisneyMtl";
}

FPInterface* AppleseedDisneyMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedDisneyMtlClassDesc::HInstance()
{
    return g_module;
}

bool AppleseedDisneyMtlClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
