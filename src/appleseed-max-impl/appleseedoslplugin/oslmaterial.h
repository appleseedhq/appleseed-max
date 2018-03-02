
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
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

#pragma once

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"

// appleseed-max headers.
#include "appleseedoslplugin/oslshadermetadata.h"
#include "iappleseedmtl.h"

// 3ds Max headers.
#include <imtl.h>
#include <iparamb2.h>
#include <maxtypes.h>
#undef base_type

// Forward declarations.
namespace renderer { class Material; }
namespace renderer { class ShaderGroup; }
class OSLPluginClassDesc;

class OSLMaterial
  : public Mtl
  , public IAppleseedMtl
{
  public:
    // Constructor.
    OSLMaterial(Class_ID class_id, OSLPluginClassDesc* class_desc);

    Class_ID get_class_id();

    // InterfaceServer methods.
    virtual BaseInterface* GetInterface(Interface_ID id) override;
    
    // Animatable methods.
    void DeleteThis() override;
    void GetClassName(TSTR& s) override;
    SClass_ID SuperClassID() override;
    Class_ID  ClassID() override;
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

    // Sub-material part of Mtl methods
    virtual int NumSubMtls() override;
    virtual Mtl* GetSubMtl(int i) override;
    virtual void SetSubMtl(int i, Mtl *m) override;
    virtual MSTR GetSubMtlSlotName(int i) override;

    // IAppleseedMtl methods.
    virtual int get_sides() const override;
    virtual bool can_emit_light() const override;
    virtual foundation::auto_release_ptr<renderer::Material> create_material(
        renderer::Assembly& assembly,
        const char*         name,
        const bool          use_max_procedural_maps) override;

  protected:
    virtual void SetReference(int i, RefTargetHandle rtarg) override;

  private:
    Class_ID                m_classid;
    OSLPluginClassDesc*     m_class_desc;
    bool                    m_has_bump_params;
    Interval                m_params_validity;
    IParamBlock2*           m_pblock;
    IParamBlock2*           m_bump_pblock;
    const OSLShaderInfo*    m_shader_info;
    IdNameVector            m_submaterial_map;
    IdNameVector            m_texture_id_map;

    foundation::auto_release_ptr<renderer::Material> create_osl_material(
        renderer::Assembly& assembly,
        const char*         name);

    foundation::auto_release_ptr<renderer::Material> create_builtin_material(
        renderer::Assembly& assembly,
        const char*         name);
};
