
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
#include "renderer/api/bsdf.h"
#include "renderer/api/bssrdf.h"
#include "renderer/api/color.h"
#include "renderer/api/material.h"
#include "renderer/api/scene.h"
#include "renderer/api/texture.h"
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

    enum MapId
    {
        MapIdSSS,
        MapIdSpecular
    };

    enum ParamId
    {
        ParamIdSSSColor,
        ParamIdSSSColorTexmap,
        ParamIdSSSScatteringColor,
        ParamIdSSSScatteringColorTexmap,
        ParamIdSSSAmount,
        ParamIdSSSScale,
        ParamIdSSSIOR,
        ParamIdSpecularColor,
        ParamIdSpecularColorTexmap,
        ParamIdSpecularAmount,
        ParamIdSpecularAmountTexmap,
        ParamIdSpecularRoughness,
        ParamIdSpecularRoughnessTexmap,
        ParamIdSpecularAnisotropy,
        ParamIdSpecularAnisotropyTexmap
    };

    enum TexmapId
    {
        TexmapIdSSSColor,
        TexmapIdSSSScatteringColor,
        TexmapIdSpecularColor,
        TexmapIdSpecularAmount,
        TexmapIdSpecularRoughness,
        TexmapIdSpecularAnisotropy,
        TexmapCount // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        _T("SSS Color"),
        _T("SSS Scattering Color")
        _T("Specular Color"),
        _T("Specular Amount"),
        _T("Specular Roughness"),
        _T("Specular Anisotropy")
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdSSSColorTexmap,
        ParamIdSSSScatteringColorTexmap,
        ParamIdSpecularColorTexmap,
        ParamIdSpecularAmountTexmap,
        ParamIdSpecularRoughnessTexmap,
        ParamIdSpecularAnisotropyTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdSSSMtl,                         // parameter block's ID
        _T("appleseedSSSMtlParams"),                // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_sssmtl_classdesc,              // class descriptor
        P_AUTO_CONSTRUCT + P_MULTIMAP + P_AUTO_UI,  // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefSSSMtl,                        // parameter block's reference number

        // --- P_MULTIMAP arguments ---
        2,                                          // number of rollups

        // --- P_AUTO_UI arguments for SSS rollup ---
        MapIdSSS,
        IDD_FORMVIEW_SSS_PARAMS,                    // ID of the dialog template
        IDS_FORMVIEW_SSS_PARAMS_TITLE,              // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- P_AUTO_UI arguments for Specular rollup ---
        MapIdSpecular,
        IDD_FORMVIEW_SPECULAR_PARAMS,               // ID of the dialog template
        IDS_FORMVIEW_SPECULAR_PARAMS_TITLE,         // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- Parameters specifications for SSS rollup ---

        ParamIdSSSColor, _T("sss_color"), TYPE_RGBA, P_ANIMATABLE, IDS_SSS_COLOR,
            p_default, Color(0.9f, 0.9f, 0.9f),
            p_ui, MapIdSSS, TYPE_COLORSWATCH, IDC_SWATCH_SSS_COLOR,
        p_end,
        ParamIdSSSColorTexmap, _T("sss_color_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SSS_COLOR,
            p_subtexno, TexmapIdSSSColor,
            p_ui, MapIdSSS, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SSS_COLOR,
        p_end,

        ParamIdSSSScatteringColor, _T("sss_scattering_color"), TYPE_RGBA, P_ANIMATABLE, IDS_SSS_SCATTERING_COLOR,
            p_default, Color(0.9f, 0.9f, 0.9f),
            p_ui, MapIdSSS, TYPE_COLORSWATCH, IDC_SWATCH_SSS_SCATTERING_COLOR,
        p_end,
        ParamIdSSSScatteringColorTexmap, _T("sss_scattering_color_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SSS_SCATTERING_COLOR,
            p_subtexno, TexmapIdSSSScatteringColor,
            p_ui, MapIdSSS, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SSS_SCATTERING_COLOR,
        p_end,

        ParamIdSSSAmount, _T("sss_amount"), TYPE_FLOAT, P_ANIMATABLE, IDS_SSS_AMOUNT,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, MapIdSSS, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SSS_AMOUNT, IDC_SLIDER_SSS_AMOUNT, 10.0f,
        p_end,

        ParamIdSSSScale, _T("sss_scale"), TYPE_FLOAT, P_ANIMATABLE, IDS_SSS_SCALE,
            p_default, 5.0f,
            p_range, 0.001f, 1000.0f,
            p_ui, MapIdSSS, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SSS_SCALE, IDC_SLIDER_SSS_SCALE, 10.0f,
        p_end,

        ParamIdSSSIOR, _T("sss_ior"), TYPE_FLOAT, P_ANIMATABLE, IDS_SSS_IOR,
            p_default, 1.3f,
            p_range, 1.0f, 2.5f,
            p_ui, MapIdSSS, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SSS_IOR, IDC_SLIDER_SSS_IOR, 0.1f,
        p_end,

        // --- Parameters specifications for Specular rollup ---

        ParamIdSpecularColor, _T("specular_color"), TYPE_RGBA, P_ANIMATABLE, IDS_SPECULAR_COLOR,
            p_default, Color(0.9f, 0.9f, 0.9f),
            p_ui, MapIdSpecular, TYPE_COLORSWATCH, IDC_SWATCH_SPECULAR_COLOR,
        p_end,
        ParamIdSpecularColorTexmap, _T("specular_color_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SPECULAR_COLOR,
            p_subtexno, TexmapIdSpecularColor,
            p_ui, MapIdSpecular, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR_COLOR,
        p_end,

        ParamIdSpecularAmount, _T("specular_amount"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR_AMOUNT,
            p_default, 100.0f,
            p_range, 0.0f, 100.0f,
            p_ui, MapIdSpecular, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR_AMOUNT, IDC_SLIDER_SPECULAR_AMOUNT, 10.0f,
        p_end,
        ParamIdSpecularAmountTexmap, _T("specular_amount_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SPECULAR_AMOUNT,
            p_subtexno, TexmapIdSpecularAmount,
            p_ui, MapIdSpecular, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR_AMOUNT,
        p_end,

        ParamIdSpecularRoughness, _T("specular_roughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR_ROUGHNESS,
            p_default, 40.0f,
            p_range, 0.0f, 100.0f,
            p_ui, MapIdSpecular, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR_ROUGHNESS, IDC_SLIDER_SPECULAR_ROUGHNESS, 10.0f,
        p_end,
        ParamIdSpecularRoughnessTexmap, _T("specular_roughness_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SPECULAR_ROUGHNESS,
            p_subtexno, TexmapIdSpecularRoughness,
            p_ui, MapIdSpecular, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR_ROUGHNESS,
        p_end,

        ParamIdSpecularAnisotropy, _T("specular_anisotropy"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR_ANISOTROPY,
            p_default, 0.0f,
            p_range, -1.0f, 1.0f,
            p_ui, MapIdSpecular, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR_ANISOTROPY, IDC_SLIDER_SPECULAR_ANISOTROPY, 0.1f,
        p_end,
        ParamIdSpecularAnisotropyTexmap, _T("specular_anisotropy_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SPECULAR_ANISOTROPY,
            p_subtexno, TexmapIdSpecularAnisotropy,
            p_ui, MapIdSpecular, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR_ANISOTROPY,
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
  , m_sss_color(0.5f, 0.5f, 0.5f)
  , m_sss_color_texmap(nullptr)
  , m_sss_scattering_color(0.5f, 0.5f, 0.5f)
  , m_sss_scattering_color_texmap(nullptr)
  , m_sss_amount(100.0f)
  , m_sss_scale(1.0f)
  , m_sss_ior(1.3f)
  , m_specular_color(0.9f, 0.9f, 0.9f)
  , m_specular_color_texmap(nullptr)
  , m_specular_amount(100.0f)
  , m_specular_amount_texmap(nullptr)
  , m_specular_roughness(40.0f)
  , m_specular_roughness_texmap(nullptr)
  , m_specular_anisotropy(0.0f)
  , m_specular_anisotropy_texmap(nullptr)
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

        m_pblock->GetValue(ParamIdSSSColor, t, m_sss_color, m_params_validity);
        m_pblock->GetValue(ParamIdSSSColorTexmap, t, m_sss_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSSSScatteringColor, t, m_sss_scattering_color, m_params_validity);
        m_pblock->GetValue(ParamIdSSSScatteringColorTexmap, t, m_sss_scattering_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSSSAmount, t, m_sss_amount, m_params_validity);
        m_pblock->GetValue(ParamIdSSSScale, t, m_sss_scale, m_params_validity);
        m_pblock->GetValue(ParamIdSSSIOR, t, m_sss_ior, m_params_validity);

        m_pblock->GetValue(ParamIdSpecularColor, t, m_specular_color, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularColorTexmap, t, m_specular_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecularAmount, t, m_specular_amount, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularAmountTexmap, t, m_specular_amount_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecularRoughness, t, m_specular_roughness, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularRoughnessTexmap, t, m_specular_roughness_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecularAnisotropy, t, m_specular_anisotropy, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularAnisotropyTexmap, t, m_specular_anisotropy_texmap, m_params_validity);

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
    return m_sss_color;
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

namespace
{
    void insert_color(asr::Assembly& assembly, const Color& color, const char* name)
    {
        assembly.colors().insert(
            asr::ColorEntityFactory::create(
                name,
                asr::ParamArray()
                    .insert("color_space", "linear_rgb")
                    .insert("color", to_color3f(color))));
    }

    std::string insert_texture_and_instance(asr::Assembly& assembly, Texmap* texmap)
    {
        BitmapTex* bitmap_tex = static_cast<BitmapTex*>(texmap);
        const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());

        const std::string texture_name = wide_to_utf8(bitmap_tex->GetName());
        assembly.textures().insert(
            asr::DiskTexture2dFactory::static_create(
                texture_name.c_str(),
                asr::ParamArray()
                    .insert("filename", filepath)
                    .insert("color_space", "srgb"),     // todo: fix
                asf::SearchPaths()));

        const std::string texture_instance_name = texture_name + "_inst";
        assembly.texture_instances().insert(
            asr::TextureInstanceFactory::create(
                texture_instance_name.c_str(),
                asr::ParamArray(),
                texture_name.c_str()));

        return texture_instance_name;
    }
}

int AppleseedSSSMtl::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool AppleseedSSSMtl::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> AppleseedSSSMtl::create_material(asr::Assembly& assembly, const char* name)
{
    asr::ParamArray material_params;

    //
    // BSSRDF.
    //

    {
        asr::ParamArray bssrdf_params;
        bssrdf_params.insert("weight", m_sss_amount / 100.0f);
        bssrdf_params.insert("dmfp_multiplier", std::max(m_sss_scale, 0.1f));
        bssrdf_params.insert("ior", m_sss_ior);

        // Diffuse mean free path.
        if (is_bitmap_texture(m_sss_scattering_color_texmap))
            bssrdf_params.insert("dmfp", insert_texture_and_instance(assembly, m_sss_scattering_color_texmap));
        else
        {
            const auto color_name = std::string(name) + "_bssrdf_dmfp";
            insert_color(assembly, m_sss_scattering_color, color_name.c_str());
            bssrdf_params.insert("dmfp", color_name);
        }

        // Reflectance.
        if (is_bitmap_texture(m_sss_color_texmap))
            bssrdf_params.insert("reflectance", insert_texture_and_instance(assembly, m_sss_color_texmap));
        else
        {
            const auto color_name = std::string(name) + "_bssrdf_reflectance";
            insert_color(assembly, m_sss_color, color_name.c_str());
            bssrdf_params.insert("reflectance", color_name);
        }

        const auto bssrdf_name = std::string(name) + "_bssrdf";
        assembly.bssrdfs().insert(
            asr::NormalizedDiffusionBSSRDFFactory::static_create(bssrdf_name.c_str(), bssrdf_params));
        material_params.insert("bssrdf", bssrdf_name);
    }

    //
    // BRDF.
    //

    {
        asr::ParamArray brdf_params;
        brdf_params.insert("mdf", "ggx");
        brdf_params.insert("ior", m_sss_ior);

        // Reflectance.
        if (is_bitmap_texture(m_specular_color_texmap))
            brdf_params.insert("reflectance", insert_texture_and_instance(assembly, m_specular_color_texmap));
        else
        {
            const auto color_name = std::string(name) + "_brdf_reflectance";
            insert_color(assembly, m_specular_color, color_name.c_str());
            brdf_params.insert("reflectance", color_name);
        }

        // Reflectance multiplier.
        if (is_bitmap_texture(m_specular_amount_texmap))
            brdf_params.insert("reflectance_multiplier", insert_texture_and_instance(assembly, m_specular_amount_texmap));
        else brdf_params.insert("reflectance_multiplier", m_specular_amount / 100.0f);

        // Roughness.
        if (is_bitmap_texture(m_specular_roughness_texmap))
            brdf_params.insert("roughness", insert_texture_and_instance(assembly, m_specular_roughness_texmap));
        else brdf_params.insert("roughness", m_specular_roughness / 100.0f);

        // Anisotropy.
        if (is_bitmap_texture(m_specular_anisotropy_texmap))
            brdf_params.insert("anisotropic", insert_texture_and_instance(assembly, m_specular_anisotropy_texmap));
        else brdf_params.insert("anisotropic", m_specular_anisotropy);

        const auto brdf_name = std::string(name) + "_brdf";
        assembly.bsdfs().insert(
            asr::GlossyBRDFFactory::static_create(brdf_name.c_str(), brdf_params));
        material_params.insert("bsdf", brdf_name);
    }

    //
    // Material.
    //

    return asr::GenericMaterialFactory::static_create(name, material_params);
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
