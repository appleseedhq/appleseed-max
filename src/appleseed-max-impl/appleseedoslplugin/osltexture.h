
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

#pragma once

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"

// appleseed-max headers.
#include "appleseedoslplugin/oslshadermetadata.h"

// 3ds Max headers.
#include <imtl.h>
#include <iparamb2.h>
#include <maxtypes.h>
#undef base_type

// Standard headers.
#include <string>
#include <vector>

// Forward declarations.
namespace renderer { class ShaderGroup; }
class OSLPluginClassDesc;

class OSLTexture
  : public Texmap
{
  public:
    // Constructor.
    OSLTexture(Class_ID class_id, OSLPluginClassDesc* class_desc);

    Class_ID get_class_id();

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
    BOOL SetDlgThing(ParamDlg* dlg) override;
    IOResult Save(ISave* isave) override;
    IOResult Load(ILoad* iload) override;

    // Texmap methods.
    RGBA EvalColor(ShadeContext& sc) override;
    Point3 EvalNormalPerturb(ShadeContext& sc) override;
    UVGen* GetTheUVGen() override;

    // Create OSL shader and connect to a given output.
    void create_osl_texture(
        renderer::ShaderGroup&  shader_group,
        const char*             material_node_name,
        const char*             material_input_name,
        const int               output_slot_index);

    void create_osl_texture(
        renderer::ShaderGroup&  shader_group,
        const char*             material_node_name,
        const char*             material_input_name);

    std::vector<std::string> get_output_names() const;

  protected:
    virtual void SetReference(int i, RefTargetHandle rtarg) override;

  private:
    OSLPluginClassDesc*     m_class_desc;
    Class_ID                m_classid;
    bool                    m_has_uv_coords;
    bool                    m_has_xyz_coords;
    Interval                m_params_validity;
    IParamBlock2*           m_pblock;
    const OSLShaderInfo*    m_shader_info;    
    IdNameVector            m_texture_id_map;
    UVGen*                  m_uv_gen;
    static ParamDlg*        m_uv_gen_dlg;
};
