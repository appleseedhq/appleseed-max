
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

#pragma once

// appleseed-max headers.
#include "iappleseedmtl.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include <color.h>
#include <IMaterialBrowserEntryInfo.h>
#include <IMtlRender_Compatibility.h>
#include <imtl.h>
#include <interval.h>
#include <iparamb2.h>
#include <maxtypes.h>
#include <ref.h>
#include <strbasic.h>
#undef base_type

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

class AppleseedGlassMtl
  : public Mtl
  , public IAppleseedMtl
{
  public:
    static Class_ID get_class_id();

    // Constructor.
    AppleseedGlassMtl();

    // InterfaceServer methods.
    virtual BaseInterface* GetInterface(Interface_ID id) override;

    // Animatable methods.
    virtual void DeleteThis() override;
    virtual void GetClassName(TSTR& s) override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual int NumSubs() override;
    virtual Animatable* SubAnim(int i) override;
    virtual TSTR SubAnimName(int i) override;
    virtual int SubNumToRefNum(int subNum) override;
    virtual int NumParamBlocks() override;
    virtual IParamBlock2* GetParamBlock(int i) override;
    virtual IParamBlock2* GetParamBlockByID(BlockID id) override;

    // ReferenceMaker methods.
    virtual int NumRefs() override;
    virtual RefTargetHandle GetReference(int i) override;
    virtual void SetReference(int i, RefTargetHandle rtarg) override;
    virtual RefResult NotifyRefChanged(
        const Interval&     changeInt,
        RefTargetHandle     hTarget,
        PartID&             partID,
        RefMessage          message,
        BOOL                propagate) override;

    // ReferenceTarget methods.
    virtual RefTargetHandle Clone(RemapDir& remap) override;

    // ISubMap methods.
    virtual int NumSubTexmaps() override;
    virtual Texmap* GetSubTexmap(int i) override;
    virtual void SetSubTexmap(int i, Texmap* texmap) override;
    virtual int MapSlotType(int i) override;
    virtual MSTR GetSubTexmapSlotName(int i) override;

    // MtlBase methods.
    virtual void Update(TimeValue t, Interval& valid) override;
    virtual void Reset() override;
    virtual Interval Validity(TimeValue t) override;
    virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp) override;
    virtual IOResult Save(ISave* isave) override;
    virtual IOResult Load(ILoad* iload) override;

    // Mtl methods.
    virtual Color GetAmbient(int mtlNum, BOOL backFace) override;
    virtual Color GetDiffuse(int mtlNum, BOOL backFace) override;
    virtual Color GetSpecular(int mtlNum, BOOL backFace) override;
    virtual float GetShininess(int mtlNum, BOOL backFace) override;
    virtual float GetShinStr(int mtlNum, BOOL backFace) override;
    virtual float GetXParency(int mtlNum, BOOL backFace) override;
    virtual void SetAmbient(Color c, TimeValue t) override;
    virtual void SetDiffuse(Color c, TimeValue t) override;
    virtual void SetSpecular(Color c, TimeValue t) override;
    virtual void SetShininess(float v, TimeValue t) override;
    virtual void Shade(ShadeContext& sc) override;

    // IAppleseedMtl methods.
    virtual int get_sides() const override;
    virtual bool can_emit_light() const override;
    virtual foundation::auto_release_ptr<renderer::Material> create_material(
        renderer::Assembly& assembly,
        const char*         name,
        bool                use_max_source) override;

  private:
    IParamBlock2*   m_pblock;
    Interval        m_params_validity;
    Color           m_surface_color;
    Texmap*         m_surface_color_texmap;
    Color           m_reflection_tint;
    Texmap*         m_reflection_tint_texmap;
    Color           m_refraction_tint;
    Texmap*         m_refraction_tint_texmap;
    float           m_ior;
    float           m_roughness;
    Texmap*         m_roughness_texmap;
    float           m_anisotropy;
    Texmap*         m_anisotropy_texmap;
    Color           m_volume_color;
    Texmap*         m_volume_color_texmap;
    float           m_scale;
    int             m_bump_method;
    Texmap*         m_bump_texmap;
    float           m_bump_amount;
    int             m_bump_up_vector;
};


//
// AppleseedGlassMtl material browser info.
//

class AppleseedGlassMtlBrowserEntryInfo
  : public IMaterialBrowserEntryInfo
{
  public:
    virtual const MCHAR* GetEntryName() const override;
    virtual const MCHAR* GetEntryCategory() const override;
    virtual Bitmap* GetEntryThumbnail() const override;
};


//
// AppleseedGlassMtl class descriptor.
//

class AppleseedGlassMtlClassDesc
  : public ClassDesc2
  , public IMtlRender_Compatibility_MtlBase
{
  public:
    AppleseedGlassMtlClassDesc();
    virtual int IsPublic() override;
    virtual void* Create(BOOL loading) override;
    virtual const MCHAR* ClassName() override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual const MCHAR* Category() override;
    virtual const MCHAR* InternalName() override;
    virtual FPInterface* GetInterface(Interface_ID id) override;
    virtual HINSTANCE HInstance() override;

    // IMtlRender_Compatibility_MtlBase methods.
    virtual bool IsCompatibleWithRenderer(ClassDesc& renderer_class_desc) override;

  private:
    AppleseedGlassMtlBrowserEntryInfo m_browser_entry_info;
};

extern AppleseedGlassMtlClassDesc g_appleseed_glassmtl_classdesc;
