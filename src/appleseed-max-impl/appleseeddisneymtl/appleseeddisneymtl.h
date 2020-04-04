
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

#pragma once

// appleseed-max-common headers.
#include "appleseed-max-common/iappleseedmtl.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.foundation headers.
#include "foundation/memory/autoreleaseptr.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <color.h>
#include <IMaterialBrowserEntryInfo.h>
#include <IMtlRender_Compatibility.h>
#include <imtl.h>
#include <interval.h>
#include <iparamb2.h>
#include <maxtypes.h>
#include <ref.h>
#include <strbasic.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Forward declarations.
namespace renderer  { class Material; }
class BaseInterface;
class Bitmap;
class Color;
class FPInterface;
class ILoad;
class Interval;
class ISave;
class ShadeContext;
class Texmap;

class AppleseedDisneyMtl
  : public Mtl
  , public IAppleseedMtl
{
  public:
    static Class_ID get_class_id();

    // Constructor.
    AppleseedDisneyMtl();

    // InterfaceServer methods.
    BaseInterface* GetInterface(Interface_ID id) override;

    // Animatable methods.
    void DeleteThis() override;
    void GetClassName(TSTR& s) override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    int NumSubs() override;
    Animatable* SubAnim(int i) override;
    TSTR SubAnimName(int i) override;
    int SubNumToRefNum(int subNum) override;
    int NumParamBlocks() override;
    IParamBlock2* GetParamBlock(int i) override;
    IParamBlock2* GetParamBlockByID(BlockID id) override;

    // ReferenceMaker methods.
    int NumRefs() override;
    RefTargetHandle GetReference(int i) override;
    void SetReference(int i, RefTargetHandle rtarg) override;
    RefResult NotifyRefChanged(
        const Interval&     changeInt,
        RefTargetHandle     hTarget,
        PartID&             partID,
        RefMessage          message,
        BOOL                propagate) override;

    // ReferenceTarget methods.
    RefTargetHandle Clone(RemapDir& remap) override;

    // ISubMap methods.
    int NumSubTexmaps() override;
    Texmap* GetSubTexmap(int i) override;
    void SetSubTexmap(int i, Texmap* texmap) override;
    int MapSlotType(int i) override;
    MSTR GetSubTexmapSlotName(int i) override;

    // MtlBase methods.
    void Update(TimeValue t, Interval& valid) override;
    void Reset() override;
    Interval Validity(TimeValue t) override;
    ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp) override;
    IOResult Save(ISave* isave) override;
    IOResult Load(ILoad* iload) override;

    // Mtl methods.
    Color GetAmbient(int mtlNum, BOOL backFace) override;
    Color GetDiffuse(int mtlNum, BOOL backFace) override;
    Color GetSpecular(int mtlNum, BOOL backFace) override;
    float GetShininess(int mtlNum, BOOL backFace) override;
    float GetShinStr(int mtlNum, BOOL backFace) override;
    float GetXParency(int mtlNum, BOOL backFace) override;
    void SetAmbient(Color c, TimeValue t) override;
    void SetDiffuse(Color c, TimeValue t) override;
    void SetSpecular(Color c, TimeValue t) override;
    void SetShininess(float v, TimeValue t) override;
    void Shade(ShadeContext& sc) override;

    // IAppleseedMtl methods.
    int get_sides() const override;
    bool can_emit_light() const override;
    foundation::auto_release_ptr<renderer::Material> create_material(
        renderer::Assembly& assembly,
        const char*         name,
        const bool          use_max_procedural_maps,
        const TimeValue     time) override;

  private:
    IParamBlock2*   m_pblock;
    Interval        m_params_validity;
    Color           m_base_color;
    Texmap*         m_base_color_texmap;
    float           m_metallic;
    Texmap*         m_metallic_texmap;
    float           m_specular;
    Texmap*         m_specular_texmap;
    float           m_specular_tint;
    Texmap*         m_specular_tint_texmap;
    float           m_roughness;
    Texmap*         m_roughness_texmap;
    float           m_sheen;
    Texmap*         m_sheen_texmap;
    float           m_sheen_tint;
    Texmap*         m_sheen_tint_texmap;
    float           m_anisotropy;
    Texmap*         m_anisotropy_texmap;
    float           m_clearcoat;
    Texmap*         m_clearcoat_texmap;
    float           m_clearcoat_gloss;
    Texmap*         m_clearcoat_gloss_texmap;
    float           m_alpha;
    Texmap*         m_alpha_texmap;
    int             m_bump_method;
    Texmap*         m_bump_texmap;
    float           m_bump_amount;
    int             m_bump_up_vector;

    foundation::auto_release_ptr<renderer::Material> create_osl_material(
        renderer::Assembly& assembly,
        const char*         name,
        const TimeValue     time);

    foundation::auto_release_ptr<renderer::Material> create_builtin_material(
        renderer::Assembly& assembly,
        const char*         name,
        const TimeValue     time);
};


//
// AppleseedDisneyMtl material browser info.
//

class AppleseedDisneyMtlBrowserEntryInfo
  : public IMaterialBrowserEntryInfo
{
  public:
    const MCHAR* GetEntryName() const override;
    const MCHAR* GetEntryCategory() const override;
    Bitmap* GetEntryThumbnail() const override;
};


//
// AppleseedDisneyMtl class descriptor.
//

class AppleseedDisneyMtlClassDesc
  : public ClassDesc2
  , public IMtlRender_Compatibility_MtlBase
{
  public:
    AppleseedDisneyMtlClassDesc();
    int IsPublic() override;
    void* Create(BOOL loading) override;
    const MCHAR* ClassName() override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    const MCHAR* Category() override;
    const MCHAR* InternalName() override;
    FPInterface* GetInterface(Interface_ID id) override;
    HINSTANCE HInstance() override;

    // IMtlRender_Compatibility_MtlBase methods.
    bool IsCompatibleWithRenderer(ClassDesc& renderer_class_desc) override;

  private:
    AppleseedDisneyMtlBrowserEntryInfo m_browser_entry_info;
};

extern AppleseedDisneyMtlClassDesc g_appleseed_disneymtl_classdesc;
