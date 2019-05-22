
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

// Interface header.
#include "appleseedobjpropsmod.h"

// appleseed-max headers.
#include "appleseedobjpropsmod/resource.h"
#include "main.h"
#include "utilities.h"

// 3ds Max headers.
#include "_beginmaxheaders.h"
#include <modstack.h>
#include <paramtype.h>
#include "_endmaxheaders.h"

namespace asf = foundation;
namespace asr = renderer;

AppleseedObjPropsModClassDesc g_appleseed_objpropsmod_classdesc;


//
// AppleseedObjPropsMod class implementation.
//

namespace
{
    enum { ParamBlockIdObjPropsMod };
    enum { ParamBlockRefObjPropsMod };

    enum ParamMapId
    {
        ParamMapIdVisibility
    };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdVisibilityCamera         = 0,
        ParamIdVisibilityLight          = 1,
        ParamIdVisibilityShadow         = 2,
        ParamIdVisibilityTransparency   = 3,
        ParamIdVisibilityProbe          = 4,
        ParamIdVisibilityDiffuse        = 5,
        ParamIdVisibilityGlossy         = 6,
        ParamIdVisibilitySpecular       = 7,
        ParamIdVisibilitySSS            = 8,
        ParamIdSSSSet                   = 9,
        ParamIdOptimizeForInstancing    = 10,
        ParamIdMediumPriority           = 11,
        ParamIdPhotonTarget             = 12
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdObjPropsMod,                    // parameter block's ID
        L"appleseedObjPropsModParams",              // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_objpropsmod_classdesc,         // class descriptor
        P_AUTO_CONSTRUCT + P_MULTIMAP + P_AUTO_UI,  // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefObjPropsMod,                   // parameter block's reference number

        // --- P_MULTIMAP arguments ---
        1,                                          // number of rollups

        // --- P_AUTO_UI arguments for Visibility rollup ---
        ParamMapIdVisibility,
        IDD_FORMVIEW_PARAMS,                        // ID of the dialog template
        IDS_FORMVIEW_PARAMS_TITLE,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- Parameters specifications ---

        ParamIdVisibilityCamera, L"visibility_camera", TYPE_BOOL, 0, IDS_VISIBILITY_CAMERA,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_CAMERA,
        p_end,
        ParamIdVisibilityLight, L"visibility_light", TYPE_BOOL, 0, IDS_VISIBILITY_LIGHT,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_LIGHT,
        p_end,
        ParamIdVisibilityShadow, L"visibility_shadow", TYPE_BOOL, 0, IDS_VISIBILITY_SHADOW,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_SHADOW,
        p_end,
        ParamIdVisibilityTransparency, L"visibility_transparency", TYPE_BOOL, 0, IDS_VISIBILITY_TRANSPARENCY,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_TRANSPARENCY,
        p_end,
        ParamIdVisibilityProbe, L"visibility_probe", TYPE_BOOL, 0, IDS_VISIBILITY_PROBE,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_PROBE,
        p_end,
        ParamIdVisibilityDiffuse, L"visibility_diffuse", TYPE_BOOL, 0, IDS_VISIBILITY_DIFFUSE,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_DIFFUSE,
        p_end,
        ParamIdVisibilityGlossy, L"visibility_glossy", TYPE_BOOL, 0, IDS_VISIBILITY_GLOSSY,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_GLOSSY,
        p_end,
        ParamIdVisibilitySpecular, L"visibility_specular", TYPE_BOOL, 0, IDS_VISIBILITY_SPECULAR,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_SPECULAR,
        p_end,
        ParamIdVisibilitySSS, L"visibility_sss", TYPE_BOOL, 0, IDS_VISIBILITY_SSS,
            p_default, TRUE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_VISIBILITY_SSS,
        p_end,
        ParamIdOptimizeForInstancing, L"optimize_for_instancing", TYPE_BOOL, 0, IDS_OPTIMIZE_FOR_INSTANCING,
            p_default, FALSE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_OPTIMIZE_FOR_INSTANCING,
        p_end,
        ParamIdSSSSet, L"sss_set", TYPE_STRING, 0, IDS_SSS_SET,
            p_ui, ParamMapIdVisibility, TYPE_EDITBOX, IDC_SSS_SET,
        p_end,
        ParamIdMediumPriority, L"medium_priority", TYPE_INT, P_TRANSIENT, 0,
            p_ui, ParamMapIdVisibility, TYPE_SPINNER, EDITTYPE_INT, IDS_TEXT_MEDIUM_PRIORITY, IDS_SPINNER_MEDIUM_PRIORITY, SPIN_AUTOSCALE,
            p_default, 0, p_range, -128, 127,
        p_end,
        ParamIdPhotonTarget, L"photon_target", TYPE_BOOL, 0, IDS_PHOTON_TARGET,
            p_default, FALSE,
            p_ui, ParamMapIdVisibility, TYPE_SINGLECHECKBOX, IDC_BUTTON_PHOTON_TARGET,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedObjPropsMod::get_class_id()
{
    return Class_ID(0x39d46f81, 0x89e7ccd);
}

AppleseedObjPropsMod::AppleseedObjPropsMod()
  : m_pblock(nullptr)
{
    g_appleseed_objpropsmod_classdesc.MakeAutoParamBlocks(this);
}

void AppleseedObjPropsMod::DeleteThis()
{
    delete this;
}

void AppleseedObjPropsMod::GetClassName(TSTR& s)
{
    s = L"appleseedObjPropsMod";
}

SClass_ID AppleseedObjPropsMod::SuperClassID()
{
    return OSM_CLASS_ID;
}

Class_ID AppleseedObjPropsMod::ClassID()
{
    return get_class_id();
}

void AppleseedObjPropsMod::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
    g_appleseed_objpropsmod_classdesc.BeginEditParams(ip, this, flags, prev);
}

void AppleseedObjPropsMod::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
{
    g_appleseed_objpropsmod_classdesc.EndEditParams(ip, this, flags, next);
}

int AppleseedObjPropsMod::NumSubs()
{
    return NumRefs();
}

Animatable* AppleseedObjPropsMod::SubAnim(int i)
{
    return GetReference(i);
}

TSTR AppleseedObjPropsMod::SubAnimName(int i)
{
    return i == ParamBlockRefObjPropsMod ? L"Parameters" : L"";
}

int AppleseedObjPropsMod::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedObjPropsMod::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedObjPropsMod::GetParamBlock(int i)
{
    return i == ParamBlockRefObjPropsMod ? m_pblock : nullptr;
}

IParamBlock2* AppleseedObjPropsMod::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedObjPropsMod::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedObjPropsMod::GetReference(int i)
{
    return i == ParamBlockRefObjPropsMod ? m_pblock : nullptr;
}

void AppleseedObjPropsMod::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefObjPropsMod)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedObjPropsMod::NotifyRefChanged(
    const Interval&     changeInt,
    RefTargetHandle     hTarget,
    PartID&             partID,
    RefMessage          message,
    BOOL                propagate)
{
    return REF_SUCCEED;
}

RefTargetHandle AppleseedObjPropsMod::Clone(RemapDir& remap)
{
    AppleseedObjPropsMod* clone = new AppleseedObjPropsMod();
    clone->ReplaceReference(ParamBlockRefObjPropsMod, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

CreateMouseCallBack* AppleseedObjPropsMod::GetCreateMouseCallBack()
{
    return nullptr;
} 

const MCHAR* AppleseedObjPropsMod::GetObjectName()
{
    // Name that appears in the modifier stack.
    return L"appleseed Object Properties";
}

void AppleseedObjPropsMod::NotifyPostCollapse(INode* node, Object* obj, IDerivedObject* derObj, int index)
{
   Object* bo = node->GetObjectRef();
   IDerivedObject* derived_obj;

   if (bo->SuperClassID() == GEN_DERIVOB_CLASS_ID) 
       derived_obj = static_cast<IDerivedObject*>(bo);
   else
   {
      derived_obj = CreateDerivedObject(obj);
      node->SetObjectRef(derived_obj);
   } 

   // Add ourselves to the top of the stack.
   derived_obj->AddModifier(this, nullptr, derived_obj->NumModifiers());
}

ChannelMask AppleseedObjPropsMod::ChannelsUsed()
{
    return 0;
}

ChannelMask AppleseedObjPropsMod::ChannelsChanged()
{
    return 0;
}

Class_ID AppleseedObjPropsMod::InputType()
{
    return defObjectClassID;
}

void AppleseedObjPropsMod::ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node)
{
    // Nothing to do.
}

asr::VisibilityFlags::Type AppleseedObjPropsMod::get_visibility_flags(const TimeValue t) const
{
    asr::VisibilityFlags::Type flags = 0;

    if (m_pblock->GetInt(ParamIdVisibilityCamera, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::CameraRay;

    if (m_pblock->GetInt(ParamIdVisibilityLight, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::LightRay;

    if (m_pblock->GetInt(ParamIdVisibilityShadow, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::ShadowRay;

    if (m_pblock->GetInt(ParamIdVisibilityTransparency, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::TransparencyRay;

    if (m_pblock->GetInt(ParamIdVisibilityProbe, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::ProbeRay;

    if (m_pblock->GetInt(ParamIdVisibilityDiffuse, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::DiffuseRay;

    if (m_pblock->GetInt(ParamIdVisibilityGlossy, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::GlossyRay;

    if (m_pblock->GetInt(ParamIdVisibilitySpecular, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::SpecularRay;

    if (m_pblock->GetInt(ParamIdVisibilitySSS, t, FOREVER) != 0)
        flags |= asr::VisibilityFlags::SubsurfaceRay;

    return flags;
}

std::string AppleseedObjPropsMod::get_sss_set(const TimeValue t) const
{
    const MCHAR* str_value;
    m_pblock->GetValue(ParamIdSSSSet, t, str_value, FOREVER);
    return str_value != nullptr ? wide_to_utf8(str_value) : std::string();
}

int AppleseedObjPropsMod::get_medium_priority(const TimeValue t) const
{
   return m_pblock->GetInt(ParamIdMediumPriority, t, FOREVER);
}


//
// AppleseedObjPropsModClassDesc class implementation.
//

int AppleseedObjPropsModClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedObjPropsModClassDesc::Create(BOOL loading)
{
    return new AppleseedObjPropsMod();
}

const MCHAR* AppleseedObjPropsModClassDesc::ClassName()
{
    // Name that appears in the list of available modifiers.
    return L"appleseed Object Properties";
}

SClass_ID AppleseedObjPropsModClassDesc::SuperClassID()
{
    return OSM_CLASS_ID;
}

Class_ID AppleseedObjPropsModClassDesc::ClassID()
{
    return AppleseedObjPropsMod::get_class_id();
}

const MCHAR* AppleseedObjPropsModClassDesc::Category()
{
    return L"appleseed";
}

const MCHAR* AppleseedObjPropsModClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedObjPropsMod";
}

HINSTANCE AppleseedObjPropsModClassDesc::HInstance()
{
    return g_module;
}
