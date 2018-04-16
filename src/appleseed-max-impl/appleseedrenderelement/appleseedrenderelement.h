
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

// 3ds Max headers.
#include <iparamb2.h>
#include <renderelements.h>

// Forward declarations.
namespace renderer
{
    class AOVFactoryRegistrar;
}

class AppleseedRenderElement
  : public IRenderElement
{
  public:
    static Class_ID get_class_id();

    // Constructor.
    explicit AppleseedRenderElement(const bool loading);

    // Animatable methods.
    void GetClassName(TSTR& s) override;
    Class_ID  ClassID() override;
    SClass_ID SuperClassID() override;
    int NumSubs() override;
    Animatable* SubAnim(int i) override;
    TSTR SubAnimName(int i) override;
    int SubNumToRefNum(int subNum) override;
    int NumParamBlocks() override;
    IParamBlock2* GetParamBlock(int i) override;
    IParamBlock2* GetParamBlockByID(BlockID id) override;
    void DeleteThis() override;

    // ReferenceTarget methods.
    RefResult NotifyRefChanged(
        const Interval&     changeInt,
        RefTargetHandle     hTarget,
        PartID&             partID,
        RefMessage          message,
        BOOL                propagate) override;
    RefTargetHandle	Clone(RemapDir	&remap) override;

    // ReferenceMaker methods.
    int NumRefs() override;
    RefTargetHandle GetReference(int i) override;
    void SetReference(int i, RefTargetHandle rtarg) override;
    IOResult Load(ILoad* iload) override;
    IOResult Save(ISave* isave) override;

    // IRenderElement methods.
    void* GetInterface(ULONG id) override;
    void ReleaseInterface(ULONG id, void* i) override;
    void SetEnabled(BOOL enabled) override;
    BOOL IsEnabled() const override;
    void SetFilterEnabled(BOOL filterEnabled) override;
    BOOL IsFilterEnabled() const override;
    BOOL BlendOnMultipass() const override;
    BOOL AtmosphereApplied() const override;
    BOOL ShadowsApplied() const override;
    void SetElementName(const MCHAR* newName) override;
    const MCHAR* ElementName() const override;
    void SetPBBitmap(PBBitmap*& pPBBitmap) const override;
    void GetPBBitmap(PBBitmap*& pPBBitmap) const override;
    IRenderElementParamDlg* CreateParamDialog(IRendParams* imp) override;

  private:
    IParamBlock2* m_pblock;          // ref 0
};


//
// AppleseedRECompatible.
//

#if MAX_RELEASE < 19000
class AppleseedRECompatible
  : public IRenderElementCompatible
{
  public:
    BOOL IsCompatible(IRenderElement* render_element) override
    {
        return render_element->ClassID() == AppleseedRenderElement::get_class_id();
    }
};
#endif


//
// AppleseedRenderElement class descriptor.
//

class AppleseedRenderElementClassDesc
  : public ClassDesc2
{
  public:
    AppleseedRenderElementClassDesc();
    int IsPublic() override;
    void* Create(BOOL loading) override;
    const wchar_t* ClassName() override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    const wchar_t* Category() override;
    const wchar_t* InternalName() override;
    HINSTANCE HInstance() override;
};

extern AppleseedRenderElementClassDesc g_appleseed_renderelement_classdesc;
extern renderer::AOVFactoryRegistrar g_aov_factory_registrar;
