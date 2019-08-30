
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016-2018 Francois Beaune, The appleseedhq Organization
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
#include "appleseedlightmtl.h"

// appleseed-max headers.
#include "appleseedlightmtl/datachunks.h"
#include "appleseedlightmtl/resource.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"
#include "version.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/edf.h"
#include "renderer/api/material.h"
#include "renderer/api/scene.h"
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"
#include "foundation/utility/searchpaths.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <AssetManagement/AssetUser.h>
#include <color.h>
#include <iparamm2.h>
#include <stdmat.h>
#include <strclass.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Standard headers.
#include <algorithm>

// Windows headers.
#include <tchar.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const wchar_t* AppleseedLightMtlFriendlyClassName = L"appleseed Light Material";
}

AppleseedLightMtlClassDesc g_appleseed_lightmtl_classdesc;


//
// AppleseedLightMtl class implementation.
//

namespace
{
    enum { ParamBlockIdLightMtl };
    enum { ParamBlockRefLightMtl };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdLightColor       = 0,
        ParamIdLightColorTexmap = 1,
        ParamIdLightPower       = 2,
        ParamIdEmissionFront    = 3,
        ParamIdEmissionBack     = 4,
        ParamIdLightExposure    = 5
    };

    enum TexmapId
    {
        // Changing these value WILL break compatibility.
        TexmapIdLightColor      = 0,
        TexmapCount             // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        L"Light Color"
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdLightColorTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdLightMtl,                       // parameter block's ID
        L"appleseedLightMtlParams",                 // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_lightmtl_classdesc,            // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefLightMtl,                      // parameter block's reference number

        // --- P_AUTO_UI arguments ---
        IDD_FORMVIEW_PARAMS,                        // ID of the dialog template
        IDS_FORMVIEW_PARAMS_TITLE,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- Parameters specifications ---

        ParamIdLightColor, L"light_color", TYPE_RGBA, P_ANIMATABLE, IDS_LIGHT_COLOR,
            p_default, Color(1.0f, 1.0f, 1.0f),
            p_ui, TYPE_COLORSWATCH, IDC_SWATCH_LIGHT_COLOR,
        p_end,
        ParamIdLightColorTexmap, L"light_color_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_LIGHT_COLOR,
            p_subtexno, TexmapIdLightColor,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_LIGHT_COLOR,
        p_end,

        ParamIdLightPower, L"light_power", TYPE_FLOAT, P_ANIMATABLE, IDS_LIGHT_POWER,
            p_default, 1.0f,
            p_range, 0.0f, 1000000.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_LIGHT_POWER, IDC_SPINNER_LIGHT_POWER, SPIN_AUTOSCALE,
        p_end,

        ParamIdLightExposure, L"light_exposure", TYPE_FLOAT, P_ANIMATABLE, IDS_LIGHT_EXPOSURE,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_LIGHT_EXPOSURE, IDC_SPINNER_LIGHT_EXPOSURE, SPIN_AUTOSCALE,
        p_end,

        ParamIdEmissionFront, L"emission_front", TYPE_BOOL, 0, IDS_EMISSION_FRONT,
            p_default, TRUE,
            p_ui, TYPE_CHECKBUTTON, IDC_BUTTON_EMISSION_FRONT,
        p_end,
        ParamIdEmissionBack, L"emission_back", TYPE_BOOL, 0, IDS_EMISSION_BACK,
            p_default, FALSE,
            p_ui, TYPE_CHECKBUTTON, IDC_BUTTON_EMISSION_BACK,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedLightMtl::get_class_id()
{
    return Class_ID(0x35390cbc, 0x618003fb);
}

AppleseedLightMtl::AppleseedLightMtl()
  : m_pblock(nullptr)
  , m_light_color(1.0f, 1.0f, 1.0f)
  , m_light_color_texmap(nullptr)
  , m_light_power(1.0f)
  , m_emission_front(true)
  , m_emission_back(false)
{
    m_params_validity.SetEmpty();

    g_appleseed_lightmtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedLightMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedLightMtl::DeleteThis()
{
    delete this;
}

void AppleseedLightMtl::GetClassName(TSTR& s)
{
    s = L"appleseedLightMtl";
}

SClass_ID AppleseedLightMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedLightMtl::ClassID()
{
    return get_class_id();
}

int AppleseedLightMtl::NumSubs()
{
    return NumRefs();
}

Animatable* AppleseedLightMtl::SubAnim(int i)
{
    return GetReference(i);
}

TSTR AppleseedLightMtl::SubAnimName(int i)
{
    return i == ParamBlockRefLightMtl ? L"Parameters" : L"";
}

int AppleseedLightMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedLightMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedLightMtl::GetParamBlock(int i)
{
    return i == ParamBlockRefLightMtl ? m_pblock : nullptr;
}

IParamBlock2* AppleseedLightMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedLightMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedLightMtl::GetReference(int i)
{
    return i == ParamBlockRefLightMtl ? m_pblock : nullptr;
}

void AppleseedLightMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefLightMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedLightMtl::NotifyRefChanged(
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

RefTargetHandle AppleseedLightMtl::Clone(RemapDir& remap)
{
    AppleseedLightMtl* clone = new AppleseedLightMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(ParamBlockRefLightMtl, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedLightMtl::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedLightMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedLightMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);
}

int AppleseedLightMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedLightMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedLightMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdLightColor, t, m_light_color, m_params_validity);
        m_pblock->GetValue(ParamIdLightColorTexmap, t, m_light_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdLightPower, t, m_light_power, m_params_validity);
        float exposure;
        m_pblock->GetValue(ParamIdLightExposure, t, exposure, m_params_validity);
        m_light_power *= std::pow(2.0f, exposure);

        int emission_front;
        m_pblock->GetValue(ParamIdEmissionFront, t, emission_front, m_params_validity);
        m_emission_front = emission_front != 0;

        int emission_back;
        m_pblock->GetValue(ParamIdEmissionBack, t, emission_back, m_params_validity);
        m_emission_back = emission_back != 0;

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }

    valid &= m_params_validity;
}

void AppleseedLightMtl::Reset()
{
    g_appleseed_lightmtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedLightMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedLightMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    return g_appleseed_lightmtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
}

IOResult AppleseedLightMtl::Save(ISave* isave)
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

IOResult AppleseedLightMtl::Load(ILoad* iload)
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

Color AppleseedLightMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedLightMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return m_light_color;
}

Color AppleseedLightMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedLightMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedLightMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedLightMtl::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedLightMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedLightMtl::SetDiffuse(Color c, TimeValue t)
{
    m_pblock->SetValue(ParamIdLightColor, t, c);
    m_light_color = c;
}

void AppleseedLightMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedLightMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedLightMtl::Shade(ShadeContext& sc)
{
}

int AppleseedLightMtl::get_sides() const
{
    int sides = 0;

    if (m_emission_front)
        sides |= asr::ObjectInstance::FrontSide;

    if (m_emission_back)
        sides |= asr::ObjectInstance::BackSide;

    return sides;
}

bool AppleseedLightMtl::can_emit_light() const
{
    return true;
}

asf::auto_release_ptr<asr::Material> AppleseedLightMtl::create_material(
    asr::Assembly&      assembly,
    const char*         name,
    const bool          use_max_procedural_maps,
    const TimeValue     time)
{
    return
        use_max_procedural_maps
            ? create_builtin_material(assembly, name, time)
            : create_osl_material(assembly, name, time);
}

asf::auto_release_ptr<asr::Material> AppleseedLightMtl::create_osl_material(
    asr::Assembly&      assembly,
    const char*         name,
    const TimeValue     time)
{
    //
    // Shader group.
    //
    auto shader_group_name = make_unique_name(assembly.shader_groups(), std::string(name) + "_shader_group");
    auto shader_group = asr::ShaderGroupFactory::create(shader_group_name.c_str());

    connect_color_texture(shader_group.ref(), name, "Color", m_light_color_texmap, m_light_color, time);
    
    shader_group->add_shader("surface", "as_max_light_material", name, 
        asr::ParamArray()
            .insert("Color", fmt_osl_expr(to_color3f(m_light_color)))
            .insert("Emission", fmt_osl_expr(m_light_power)));

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

    return asr::OSLMaterialFactory().create(name, material_params);
}

asf::auto_release_ptr<asr::Material> AppleseedLightMtl::create_builtin_material(
    asr::Assembly&      assembly,
    const char*         name,
    const TimeValue     time)
{
    asr::ParamArray material_params;
    const bool use_max_procedural_maps = true;

    //
    // EDF.
    //

    asr::ParamArray edf_params;

    // Radiance.
    std::string instance_name = insert_texture_and_instance(assembly, m_light_color_texmap, use_max_procedural_maps, time);
    if (!instance_name.empty())
        edf_params.insert("radiance", instance_name);
    else
    {
        const auto color_name = std::string(name) + "_edf_radiance";
        insert_color(assembly, m_light_color, color_name.c_str());
        edf_params.insert("radiance", color_name);
    }

    // Radiance multiplier.
    edf_params.insert("radiance_multiplier", m_light_power);

    // EDF.
    const auto edf_name = std::string(name) + "_edf";
    assembly.edfs().insert(
        asr::DiffuseEDFFactory().create(edf_name.c_str(), edf_params));
    material_params.insert("edf", edf_name);

    //
    // Material.
    //

    return asr::GenericMaterialFactory().create(name, material_params);
}


//
// AppleseedLightMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedLightMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedLightMtlFriendlyClassName;
}

const MCHAR* AppleseedLightMtlBrowserEntryInfo::GetEntryCategory() const
{
    return L"Materials\\appleseed";
}

Bitmap* AppleseedLightMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedLightMtlClassDesc class implementation.
//

AppleseedLightMtlClassDesc::AppleseedLightMtlClassDesc()
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int AppleseedLightMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedLightMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedLightMtl();
}

const MCHAR* AppleseedLightMtlClassDesc::ClassName()
{
    return AppleseedLightMtlFriendlyClassName;
}

SClass_ID AppleseedLightMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedLightMtlClassDesc::ClassID()
{
    return AppleseedLightMtl::get_class_id();
}

const MCHAR* AppleseedLightMtlClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedLightMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedLightMtl";
}

FPInterface* AppleseedLightMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedLightMtlClassDesc::HInstance()
{
    return g_module;
}

bool AppleseedLightMtlClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
