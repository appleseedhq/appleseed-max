
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
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
#include "appleseedvolumemtl.h"

// appleseed-max headers.
#include "appleseedrenderer/appleseedrenderer.h"
#include "appleseedvolumemtl/datachunks.h"
#include "appleseedvolumemtl/resource.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"
#include "version.h"

// Build options header.
#include "renderer/api/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/material.h"
#include "renderer/api/scene.h"
#include "renderer/api/utility.h"
#include "renderer/api/volume.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"
#include "foundation/utility/searchpaths.h"

// 3ds Max headers.
#include <AssetManagement/AssetUser.h>
#include <color.h>
#include <iparamm2.h>
#include <stdmat.h>
#include <strclass.h>

// Windows headers.
#include <tchar.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const wchar_t* AppleseedVolumeMtlFriendlyClassName = L"appleseed Volume Material";
}

AppleseedVolumeMtlClassDesc g_appleseed_volumemtl_classdesc;


//
// AppleseedVolumeMtl class implementation.
//

namespace
{
    enum { ParamBlockIdVolumeMtl };
    enum { ParamBlockRefVolumeMtl };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdVolumeAbsorption             = 0,
        ParamIdVolumeAbsorptionMultiplier   = 1,
        ParamIdVolumeScattering             = 2,
        ParamIdVolumeScatteringMultiplier   = 3,
        ParamIdVolumePhaseFunctionModel     = 4,
        ParamIdVolumeAverageCosine          = 5,
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdVolumeMtl,                      // parameter block's ID
        L"appleseedVolumeMtlParams",                // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_volumemtl_classdesc,           // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefVolumeMtl,                     // parameter block's reference number

        // --- P_AUTO_UI arguments for Volume rollup ---
        IDD_FORMVIEW_VOLUME_PARAMS,                 // ID of the dialog template
        IDS_FORMVIEW_VOLUME_PARAMS_TITLE,           // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure
        
        // --- Parameters specifications for Volume rollup ---

        ParamIdVolumeAbsorption, L"volume_absorption", TYPE_RGBA, P_ANIMATABLE, IDS_VOLUME_ABSORTION,
            p_default, Color(0.5f, 0.5f, 0.5f),
            p_ui, TYPE_COLORSWATCH, IDC_VOLUME_ABSORTION,
        p_end,

        ParamIdVolumeAbsorptionMultiplier, L"volume_absorption_multiplier", TYPE_FLOAT, P_ANIMATABLE, IDS_VOLUME_ABSORTION_MULTIPLIER,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_VOLUME_ABSORTION_MULTIPLIER, IDC_SLIDER_VOLUME_ABSORTION_MULTIPLIER, 10.0f,
        p_end,

        ParamIdVolumeScattering, L"volume_scattering", TYPE_RGBA, P_ANIMATABLE, IDS_VOLUME_SCATTERING,
            p_default, Color(0.5f, 0.5f, 0.5f),
            p_ui, TYPE_COLORSWATCH, IDC_VOLUME_SCATTERING,
        p_end,

        ParamIdVolumeScatteringMultiplier, L"volume_scattering_multiplier", TYPE_FLOAT, P_ANIMATABLE, IDS_VOLUME_SCATTERING_MULTIPLIER,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_VOLUME_SCATTERING_MULTIPLIER, IDC_SPINNER_VOLUME_SCATTERING_MULTIPLIER, SPIN_AUTOSCALE,
        p_end,

        ParamIdVolumeAverageCosine, L"volume_average_cosine", TYPE_FLOAT, P_ANIMATABLE, IDS_VOLUME_AVERAGE_COSINE,
            p_default, 0.0f,
            p_range, -1.0f, 1.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_VOLUME_AVERAGE_COSINE, IDC_SPINNER_VOLUME_AVERAGE_COSINE, SPIN_AUTOSCALE,
        p_end,

        ParamIdVolumePhaseFunctionModel, L"volume_phase_function_model", TYPE_INT, 0, IDS_VOLUME_PHASE_FUNCTION_MODEL,
            p_ui, TYPE_INT_COMBOBOX, IDC_COMBO_VOLUME_PHASE_FUNCTION_MODEL,
            2, IDS_VOLUME_HENYEY_MODEL, IDS_VOLUME_ISOTROPIC_MODEL,
            p_vals, 0, 1,
            p_default, 0,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedVolumeMtl::get_class_id()
{
    return Class_ID(0x5c2e11d7, 0x34c07107);
}

AppleseedVolumeMtl::AppleseedVolumeMtl()
  : m_pblock(nullptr)
{
    m_params_validity.SetEmpty();

    g_appleseed_volumemtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedVolumeMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedVolumeMtl::DeleteThis()
{
    delete this;
}

void AppleseedVolumeMtl::GetClassName(TSTR& s)
{
    s = L"appleseedVolumeMtl";
}

SClass_ID AppleseedVolumeMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedVolumeMtl::ClassID()
{
    return get_class_id();
}

int AppleseedVolumeMtl::NumSubs()
{
    return NumRefs();
}

Animatable* AppleseedVolumeMtl::SubAnim(int i)
{
    return GetReference(i);
}

TSTR AppleseedVolumeMtl::SubAnimName(int i)
{
    return i == ParamBlockRefVolumeMtl ? L"Parameters" : L"";
}

int AppleseedVolumeMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedVolumeMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedVolumeMtl::GetParamBlock(int i)
{
    return i == ParamBlockRefVolumeMtl ? m_pblock : nullptr;
}

IParamBlock2* AppleseedVolumeMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedVolumeMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedVolumeMtl::GetReference(int i)
{
    return i == ParamBlockRefVolumeMtl ? m_pblock : nullptr;
}

void AppleseedVolumeMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefVolumeMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedVolumeMtl::NotifyRefChanged(
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

RefTargetHandle AppleseedVolumeMtl::Clone(RemapDir& remap)
{
    AppleseedVolumeMtl* clone = new AppleseedVolumeMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(ParamBlockRefVolumeMtl, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedVolumeMtl::NumSubTexmaps()
{
    return 0;
}

Texmap* AppleseedVolumeMtl::GetSubTexmap(int i)
{
    return nullptr;
}

void AppleseedVolumeMtl::SetSubTexmap(int i, Texmap* texmap)
{
}

int AppleseedVolumeMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedVolumeMtl::GetSubTexmapSlotName(int i)
{
    return L"";
}

void AppleseedVolumeMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }

    valid &= m_params_validity;
}

void AppleseedVolumeMtl::Reset()
{
    g_appleseed_volumemtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedVolumeMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedVolumeMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    ParamDlg* param_dialog = g_appleseed_volumemtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    DbgAssert(m_pblock != nullptr);
    return param_dialog;
}

IOResult AppleseedVolumeMtl::Save(ISave* isave)
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

IOResult AppleseedVolumeMtl::Load(ILoad* iload)
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

Color AppleseedVolumeMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedVolumeMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    const Color color = m_pblock->GetColor(ParamIdVolumeScattering, GetCOREInterface()->GetTime(), FOREVER);
    const float multiplier =  m_pblock->GetFloat(ParamIdVolumeScatteringMultiplier, GetCOREInterface()->GetTime(), FOREVER);
    return color * multiplier;
}

Color AppleseedVolumeMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedVolumeMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedVolumeMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedVolumeMtl::GetXParency(int mtlNum, BOOL backFace)
{
    float multiplier = m_pblock->GetFloat(ParamIdVolumeScatteringMultiplier, GetCOREInterface()->GetTime(), FOREVER);
    return asf::min(1 - (multiplier / 100.0f), 0.95f);
}

void AppleseedVolumeMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedVolumeMtl::SetDiffuse(Color c, TimeValue t)
{
    m_pblock->SetValue(ParamIdVolumeScattering, t, c);
}

void AppleseedVolumeMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedVolumeMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedVolumeMtl::Shade(ShadeContext& sc)
{
}

int AppleseedVolumeMtl::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool AppleseedVolumeMtl::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> AppleseedVolumeMtl::create_material(
    asr::Assembly&      assembly,
    const char*         name,
    const bool          use_max_procedural_maps,
    const TimeValue     time)
{
    asr::ParamArray material_params;

    //
    // Volume.
    //

    asr::ParamArray volume_params;

    // Absorption.
    Color absorption = m_pblock->GetColor(ParamIdVolumeAbsorption, time, m_params_validity);
    const auto absorption_color_name = std::string(name) + "_volume_absorption";
    insert_color(assembly, absorption, absorption_color_name.c_str());
    volume_params.insert("absorption", absorption_color_name);

    // Absorption Multiplier.
    float absorption_multiplier = m_pblock->GetFloat(ParamIdVolumeAbsorptionMultiplier, time, m_params_validity);
    volume_params.insert("absorption_multiplier", absorption_multiplier / 100.0f);

    // Scattering.
    Color scattering = m_pblock->GetColor(ParamIdVolumeScattering, time, m_params_validity);
    const auto scattering_color_name = std::string(name) + "_volume_scattering";
    insert_color(assembly, scattering, scattering_color_name.c_str());
    volume_params.insert("scattering", scattering_color_name);

    // Scattering Multiplier.
    float scattering_multiplier = m_pblock->GetFloat(ParamIdVolumeScatteringMultiplier, time, m_params_validity);
    volume_params.insert("scattering_multiplier", scattering_multiplier / 100.0f);

    // Phase Function Model.
    int phase_function_model = m_pblock->GetInt(ParamIdVolumePhaseFunctionModel, time, m_params_validity);
    volume_params.insert("phase_function_model", phase_function_model == 0 ? "henyey" : "isotropic");

    // Average Cosine.
    float average_cosine = m_pblock->GetFloat(ParamIdVolumeAverageCosine, time, m_params_validity);
    volume_params.insert("average_cosine", average_cosine);

    // Volume.
    const auto volume_name = std::string(name) + "_volume";
    assembly.volumes().insert(asr::GenericVolumeFactory().create(volume_name.c_str(), volume_params));
    
    material_params.insert("volume", volume_name);

    //
    // Material.
    //

    return asr::GenericMaterialFactory().create(name, material_params);
}


//
// AppleseedVolumeMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedVolumeMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedVolumeMtlFriendlyClassName;
}

const MCHAR* AppleseedVolumeMtlBrowserEntryInfo::GetEntryCategory() const
{
    return L"Materials\\appleseed";
}

Bitmap* AppleseedVolumeMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedVolumeMtlClassDesc class implementation.
//

AppleseedVolumeMtlClassDesc::AppleseedVolumeMtlClassDesc()
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int AppleseedVolumeMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedVolumeMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedVolumeMtl();
}

const MCHAR* AppleseedVolumeMtlClassDesc::ClassName()
{
    return AppleseedVolumeMtlFriendlyClassName;
}

SClass_ID AppleseedVolumeMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedVolumeMtlClassDesc::ClassID()
{
    return AppleseedVolumeMtl::get_class_id();
}

const MCHAR* AppleseedVolumeMtlClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedVolumeMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedVolumeMtl";
}

FPInterface* AppleseedVolumeMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedVolumeMtlClassDesc::HInstance()
{
    return g_module;
}

bool AppleseedVolumeMtlClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
