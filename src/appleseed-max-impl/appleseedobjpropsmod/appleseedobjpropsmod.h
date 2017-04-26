
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017 Francois Beaune, The appleseedhq Organization
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
#include <maxtypes.h>
#include <object.h>
#include <ref.h>
#include <strbasic.h>
#include <strclass.h>
#undef base_type

class AppleseedObjPropsMod
  : public OSModifier
{
  public:
    static Class_ID get_class_id();

    // Constructor.
    AppleseedObjPropsMod();

    // Animatable methods.
    virtual void DeleteThis() override;
    virtual void GetClassName(TSTR& s) override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev = nullptr) override;
    virtual void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next = nullptr) override;
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

    // BaseObject methods.
    virtual CreateMouseCallBack* GetCreateMouseCallBack() override;
    virtual const MCHAR* GetObjectName() override;
    virtual void NotifyPostCollapse(INode* node, Object* obj, IDerivedObject* derObj, int index);

    // Modifier methods.
    virtual ChannelMask ChannelsUsed() override;
    virtual ChannelMask ChannelsChanged() override;
    virtual Class_ID InputType() override;
    virtual void ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node) override;

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
    virtual int IsPublic() override;
    virtual void* Create(BOOL loading) override;
    virtual const MCHAR* ClassName() override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual const MCHAR* Category() override;
    virtual const MCHAR* InternalName() override;
    virtual HINSTANCE HInstance() override;
};

extern AppleseedObjPropsModClassDesc g_appleseed_objpropsmod_classdesc;
