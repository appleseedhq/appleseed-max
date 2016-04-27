
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016 Francois Beaune, The appleseedhq Organization
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
#include "appleseedsssmtl.h"

// appleseed-max headers.
#include "sssmtl/appleseedsssmtlparamdlg.h"
#include "sssmtl/datachunks.h"
#include "sssmtl/resource.h"
#include "main.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/bssrdf.h"
#include "renderer/api/color.h"
#include "renderer/api/material.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"

// 3ds Max headers.
#include <assert1.h>
#include <color.h>
#include <iparamm2.h>
#include <stdmat.h>
#include <strclass.h>

// Windows headers.
#include <tchar.h>

// Standard headers.
#include <algorithm>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const TCHAR* AppleseedSSSMtlFriendlyClassName = _T("appleseed SSS Material");
}

AppleseedSSSMtlClassDesc g_appleseed_sssmtl_classdesc;


//
// AppleseedSSSMtl class implementation.
//

namespace
{
    enum { ParamBlockIdSSSMtl };
    enum { ParamBlockRefSSSMtl };

    enum ParamId
    {
        ParamIdBaseColor,
        ParamIdBaseColorTexmap,
        ParamIdScatterDistance,
        ParamIdIOR
    };

    enum TexmapId
    {
        TexmapIdBase,
        TexmapCount
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        _T("Base Color")
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdBaseColorTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdSSSMtl,                         // parameter block's ID
        _T("appleseedSSSMtlParams"),                // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_sssmtl_classdesc,              // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefSSSMtl,                        // parameter block's reference number

        // --- P_AUTO_UI arguments ---
        IDD_FORMVIEW_PARAMS,                        // ID of the dialog template
        IDS_FORMVIEW_PARAMS_TITLE,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- Parameters specifications ---

        ParamIdBaseColor, _T("base_color"), TYPE_RGBA, P_ANIMATABLE, IDS_BASE_COLOR,
            p_default, Color(0.9f, 0.9f, 0.9f),
            p_ui, TYPE_COLORSWATCH, IDC_SWATCH_BASE_COLOR,
        p_end,
        ParamIdBaseColorTexmap, _T("base_color_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_BASE_COLOR,
            p_subtexno, TexmapIdBase,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_BASE_COLOR,
        p_end,

        ParamIdScatterDistance, _T("scatter_distance"), TYPE_FLOAT, P_ANIMATABLE, IDS_SCATTER_DISTANCE,
            p_default, 5.0f,
            p_range, 0.001f, 10000.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SCATTER_DISTANCE, IDC_SLIDER_SCATTER_DISTANCE, 10.0f,
        p_end,

        ParamIdIOR, _T("ior"), TYPE_FLOAT, P_ANIMATABLE, IDS_IOR,
            p_default, 1.3f,
            p_range, 0.5f, 2.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_IOR, IDC_SLIDER_IOR, 0.1f,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedSSSMtl::get_class_id()
{
    return Class_ID(0x6b4d5bb5, 0x49f24f8f);
}

AppleseedSSSMtl::AppleseedSSSMtl()
  : m_pblock(nullptr)
  , m_base_color(0.0f, 0.0f, 0.0f)
  , m_base_color_texmap(nullptr)
  , m_scatter_distance(0.0f)
  , m_ior(0.0f)
{
    m_params_validity.SetEmpty();

    g_appleseed_sssmtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedSSSMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedSSSMtl::DeleteThis()
{
    delete this;
}

void AppleseedSSSMtl::GetClassName(TSTR& s)
{
    s = _T("appleseedSSSMtl");
}

SClass_ID AppleseedSSSMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedSSSMtl::ClassID()
{
    return get_class_id();
}

int AppleseedSSSMtl::NumSubs()
{
    return 1;
}

Animatable* AppleseedSSSMtl::SubAnim(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

TSTR AppleseedSSSMtl::SubAnimName(int i)
{
    return i == 0 ? _T("Parameters") : _T("");
}

int AppleseedSSSMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedSSSMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedSSSMtl::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* AppleseedSSSMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedSSSMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedSSSMtl::GetReference(int i)
{
    return i == ParamBlockRefSSSMtl ? m_pblock : nullptr;
}

void AppleseedSSSMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefSSSMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedSSSMtl::NotifyRefChanged(
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
        break;
    }

    return REF_SUCCEED;
}

RefTargetHandle AppleseedSSSMtl::Clone(RemapDir& remap)
{
    AppleseedSSSMtl* clone = new AppleseedSSSMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(0, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedSSSMtl::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedSSSMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedSSSMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);
}

int AppleseedSSSMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedSSSMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedSSSMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdBaseColor, t, m_base_color, m_params_validity);
        m_pblock->GetValue(ParamIdBaseColorTexmap, t, m_base_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdScatterDistance, t, m_scatter_distance, m_params_validity);
        m_pblock->GetValue(ParamIdIOR, t, m_ior, m_params_validity);

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }

    valid &= m_params_validity;
}

void AppleseedSSSMtl::Reset()
{
    g_appleseed_sssmtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedSSSMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedSSSMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    return g_appleseed_sssmtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
}

IOResult AppleseedSSSMtl::Save(ISave* isave)
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

IOResult AppleseedSSSMtl::Load(ILoad* iload)
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

Color AppleseedSSSMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedSSSMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return m_base_color;
}

Color AppleseedSSSMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedSSSMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedSSSMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedSSSMtl::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedSSSMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedSSSMtl::SetDiffuse(Color c, TimeValue t)
{
}

void AppleseedSSSMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedSSSMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedSSSMtl::Shade(ShadeContext& sc)
{
}

asf::auto_release_ptr<asr::Material> AppleseedSSSMtl::create_material(asr::Assembly& assembly, const char* name)
{
    // todo: use texture if set.
    const auto reflectance_name = std::string(name) + "_reflectance";
    assembly.colors().insert(
        asr::ColorEntityFactory::create(
            reflectance_name.c_str(),
            asr::ParamArray()
                .insert("color_space", "linear_rgb")
                .insert("color", to_color3f(m_base_color))));

    const float scatter_distance = std::max(m_scatter_distance, 0.1f);

    const auto bssrdf_name = std::string(name) + "_bssrdf";
    assembly.bssrdfs().insert(
        asr::NormalizedDiffusionBSSRDFFactory::static_create(
            bssrdf_name.c_str(),
            asr::ParamArray()
                .insert("weight", "1.0")
                .insert("reflectance", reflectance_name)
                .insert("dmfp", scatter_distance)
                .insert("outside_ior", "1.0")
                .insert("inside_ior", m_ior)));

    auto material =
        asr::GenericMaterialFactory::static_create(
            name,
            asr::ParamArray()
                .insert("bssrdf", bssrdf_name));

    return material;
}


//
// AppleseedSSSMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedSSSMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedSSSMtlFriendlyClassName;
}

const MCHAR* AppleseedSSSMtlBrowserEntryInfo::GetEntryCategory() const
{
    return _T("Materials\\appleseed");
}

Bitmap* AppleseedSSSMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedSSSMtlClassDesc class implementation.
//

int AppleseedSSSMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedSSSMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedSSSMtl();
}

const MCHAR* AppleseedSSSMtlClassDesc::ClassName()
{
    return AppleseedSSSMtlFriendlyClassName;
}

SClass_ID AppleseedSSSMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedSSSMtlClassDesc::ClassID()
{
    return AppleseedSSSMtl::get_class_id();
}

const MCHAR* AppleseedSSSMtlClassDesc::Category()
{
    return _T("");
}

const MCHAR* AppleseedSSSMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return _T("appleseed_sssmtl");
}

FPInterface* AppleseedSSSMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedSSSMtlClassDesc::HInstance()
{
    return g_module;
}
