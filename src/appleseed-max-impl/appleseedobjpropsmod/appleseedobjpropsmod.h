
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017-2018 Francois Beaune, The appleseedhq Organization
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

// appleseed.renderer headers.
#include "renderer/api/scene.h"

// 3ds Max headers.
#include "_beginmaxheaders.h"
#include <iparamb2.h>
#include <maxtypes.h>
#include <object.h>
#include <ref.h>
#include <strbasic.h>
#include <strclass.h>
#include "_endmaxheaders.h"

class AppleseedObjPropsMod
  : public OSModifier
{
  public:
    static Class_ID get_class_id();

    // Constructor.
    AppleseedObjPropsMod();

    // Animatable methods.
    void DeleteThis() override;
    void GetClassName(TSTR& s) override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev = nullptr) override;
    void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next = nullptr) override;
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

    // BaseObject methods.
    CreateMouseCallBack* GetCreateMouseCallBack() override;
    const MCHAR* GetObjectName() override;
    void NotifyPostCollapse(INode* node, Object* obj, IDerivedObject* derObj, int index) override;

    // Modifier methods.
    ChannelMask ChannelsUsed() override;
    ChannelMask ChannelsChanged() override;
    Class_ID InputType() override;
    void ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node) override;

    renderer::VisibilityFlags::Type get_visibility_flags(const TimeValue t) const;
    std::string get_sss_set(const TimeValue t) const;
    int get_medium_priority(const TimeValue t) const;

  private:
    IParamBlock2*   m_pblock;
};


//
// AppleseedObjPropsMod class descriptor.
//

class AppleseedObjPropsModClassDesc
  : public ClassDesc2
{
  public:
    int IsPublic() override;
    void* Create(BOOL loading) override;
    const MCHAR* ClassName() override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    const MCHAR* Category() override;
    const MCHAR* InternalName() override;
    HINSTANCE HInstance() override;
};

extern AppleseedObjPropsModClassDesc g_appleseed_objpropsmod_classdesc;
