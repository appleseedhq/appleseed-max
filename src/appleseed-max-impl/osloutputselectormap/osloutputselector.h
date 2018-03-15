
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
    virtual void DeleteThis() override;
    virtual void GetClassName(TSTR& s) override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID  ClassID() override;
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

    // Texmap methods.
    virtual RGBA EvalColor(ShadeContext& sc) override;
    virtual Point3 EvalNormalPerturb(ShadeContext& sc) override;

  protected:
    virtual void SetReference(int i, RefTargetHandle rtarg) override;

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
    virtual const MCHAR* GetEntryName() const override;
    virtual const MCHAR* GetEntryCategory() const override;
    virtual Bitmap* GetEntryThumbnail() const override;
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
    virtual int IsPublic() override;
    virtual void* Create(BOOL loading) override;
    virtual const wchar_t* ClassName() override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual const wchar_t* Category() override;
    virtual const wchar_t* InternalName() override;
    virtual FPInterface* GetInterface(Interface_ID id) override;
    virtual HINSTANCE HInstance() override;

    // IMtlRender_Compatibility_MtlBase methods.
    virtual bool IsCompatibleWithRenderer(ClassDesc& renderer_class_desc) override;

  private:
    OSLOutputSelectorBrowserEntryInfo m_browser_entry_info;
};

extern OSLOutputSelectorClassDesc g_appleseed_outputselector_classdesc;
