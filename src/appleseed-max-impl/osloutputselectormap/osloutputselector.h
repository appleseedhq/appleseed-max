
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

// Build options header.
#include "renderer/api/buildoptions.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include <IMaterialBrowserEntryInfo.h>
#include <IMtlRender_Compatibility.h>
#include <imtl.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <istdplug.h>
#include <maxtypes.h>
#include <stdmat.h>
#undef base_type

class OSLOutputSelector
  : public Texmap
{
  public:
    static Class_ID get_class_id();

    // Constructor.
    OSLOutputSelector();

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

    // Texmap methods.
    RGBA EvalColor(ShadeContext& sc) override;
    Point3 EvalNormalPerturb(ShadeContext& sc) override;

  protected:
    void SetReference(int i, RefTargetHandle rtarg) override;

  private:
    IParamBlock2*   m_pblock;          // ref 0
    Interval        m_params_validity;
};


//
// OSLOutputSelector material browser info.
//

class OSLOutputSelectorBrowserEntryInfo
  : public IMaterialBrowserEntryInfo
{
  public:
    const MCHAR* GetEntryName() const override;
    const MCHAR* GetEntryCategory() const override;
    Bitmap* GetEntryThumbnail() const override;
};


//
// OSLOutputSelector class descriptor.
//

class OSLOutputSelectorClassDesc
  : public ClassDesc2
  , public IMtlRender_Compatibility_MtlBase
{
  public:
    OSLOutputSelectorClassDesc();
    int IsPublic() override;
    void* Create(BOOL loading) override;
    const wchar_t* ClassName() override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    const wchar_t* Category() override;
    const wchar_t* InternalName() override;
    FPInterface* GetInterface(Interface_ID id) override;
    HINSTANCE HInstance() override;

    // IMtlRender_Compatibility_MtlBase methods.
    bool IsCompatibleWithRenderer(ClassDesc& renderer_class_desc) override;

  private:
    OSLOutputSelectorBrowserEntryInfo m_browser_entry_info;
};

extern OSLOutputSelectorClassDesc g_appleseed_outputselector_classdesc;
