
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

// Interface header.
#include "appleseedrenderelement.h"

// appleseed-max headers.
#include "appleseedrenderelement/resource.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "main.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/aov.h"

// 3ds Max headers.
#include <iparamm2.h>

// Standard headers.
#include <string>
#include <vector>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const wchar_t* AppleseedRenderElementFriendlyClassName = L"appleseed_AOV";

    enum { ParamBlockIdRenderElement };
    enum { ParamBlockRefRenderElement };

    enum ParamId
    {
        ParamIdEnableOn         = 0,
        ParamIdElementBitmap    = 1,
        ParamIdElementName      = 2,
        ParamIdAOVIndex         = 3,
    };
}

AppleseedRenderElementClassDesc g_appleseed_renderelement_classdesc;
asr::AOVFactoryRegistrar g_aov_factory_registrar;


//
// AppleseedRenderElement class implementation.
//

namespace
{
    
    std::vector<std::string> get_aov_names()
    {
        auto factories = asf::array_vector<std::vector<asr::IAOVFactory*>>(g_aov_factory_registrar.get_factories());
        std::vector<std::string> aov_names;
        for (const auto& factory : factories)
            aov_names.push_back(factory->get_model_metadata().get("label"));

        return aov_names;
    }

    void init_combo_box(IParamMap2* param_map)
    {
        HWND dlg_hwnd = nullptr;
        if (param_map != nullptr)
            dlg_hwnd = param_map->GetHWnd();
        if (dlg_hwnd == nullptr)
            return;

        SendMessage(GetDlgItem(dlg_hwnd, IDC_COMBO_AOV_NAME), CB_RESETCONTENT, 0, 0);
        if (true)
        {
            auto aov_names = get_aov_names();

            for (const auto& name : aov_names)
            {
                SendMessage(
                    GetDlgItem(dlg_hwnd, IDC_COMBO_AOV_NAME),
                    CB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>(utf8_to_wide(name).c_str()));
            }

            int output_index = param_map->GetParamBlock()->GetInt(ParamIdAOVIndex, 0, FOREVER);
            DbgAssert(output_index >= 1);
            output_index -= 1;
            if (output_index < aov_names.size())
                SendMessage(GetDlgItem(dlg_hwnd, IDC_COMBO_AOV_NAME), CB_SETCURSEL, output_index, 0);
            else
                SendMessage(GetDlgItem(dlg_hwnd, IDC_COMBO_AOV_NAME), CB_SETCURSEL, -1, 0);
        }
    }

    class AppleseedRenderElementParamDlgProc
      : public ParamMap2UserDlgProc
    {
      public:
        void DeleteThis() override
        {
        }

        INT_PTR DlgProc(
            TimeValue   t,
            IParamMap2* map,
            HWND        hwnd,
            UINT        umsg,
            WPARAM      wparam,
            LPARAM      lparam) override
        {
            switch (umsg)
            {
              case WM_INITDIALOG:
                init_combo_box(map);
                return TRUE;

              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_COMBO_AOV_NAME:
                    switch (HIWORD(wparam))
                    {
                      case CBN_SELCHANGE:
                        {
                            LRESULT selected = SendMessage(GetDlgItem(hwnd, IDC_COMBO_AOV_NAME), CB_GETCURSEL, 0, 0);
                            int aov_index = static_cast<int>(selected);
                            map->GetParamBlock()->SetValue(ParamIdAOVIndex, t, aov_index + 1);
                            auto aov_names = get_aov_names();
                            if (aov_index < aov_names.size())
                            {
                                std::wstring element_name = std::wstring(AppleseedRenderElementFriendlyClassName);
                                element_name += L"_";
                                element_name += utf8_to_wide(aov_names[aov_index]);
                                AppleseedRenderElement* element = static_cast<AppleseedRenderElement*>(map->GetParamBlock()->GetOwner());
                                element->SetElementName(element_name.c_str());
                            }
                        }
                        return TRUE;

                      default:
                        return FALSE;
                    }

                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }
    };

    AppleseedRenderElementParamDlgProc element_dlg_proc;

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdRenderElement,                  // parameter block's ID
        L"appleseedRenderElementParams",            // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_renderelement_classdesc,       // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefRenderElement,                 // parameter block's reference number

        // --- P_AUTO_UI arguments for Parameters rollup ---
        IDD_RENDERELEMENT_PANEL,                    // ID of the dialog template
        IDS_RENDERELEMENT_PARAMS,                   // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        &element_dlg_proc,

        // --- Parameters specifications for Parameters rollup ---
        ParamIdElementName, L"elementName", TYPE_STRING, 0, IDS_ELEMENT_NAME,
            p_default, L"",
            p_end,
        ParamIdEnableOn, L"enabled", TYPE_BOOL, 0, IDS_ENABLED,
            p_default, true,
            p_end,
        ParamIdElementBitmap, L"bitmap", TYPE_BITMAP, 0, IDS_BITMAP,
            p_end,
        ParamIdAOVIndex, L"aov_index", TYPE_INT, 0, IDS_AOV_INDEX,
            p_default, 1,
            p_range, 1, 100,
            p_end,
        p_end
    );
}

Class_ID AppleseedRenderElement::get_class_id()
{
    return Class_ID(0x7f0f540d, 0x62a06ab3);
}

AppleseedRenderElement::AppleseedRenderElement(const bool loading)
  : m_pblock(nullptr)
{
    if (!loading)
    {
        g_appleseed_renderelement_classdesc.MakeAutoParamBlocks(this);
        SetElementName(AppleseedRenderElementFriendlyClassName);
    }
}

void AppleseedRenderElement::DeleteThis()
{
    delete this;
}

void AppleseedRenderElement::GetClassName(TSTR& s)
{
    s = L"appleseedRenderElement";
}

Class_ID AppleseedRenderElement::ClassID()
{
    return get_class_id();
}

SClass_ID AppleseedRenderElement::SuperClassID()
{
    return g_appleseed_renderelement_classdesc.SuperClassID();
}

int AppleseedRenderElement::NumSubs()
{
    return 1;
}

Animatable* AppleseedRenderElement::SubAnim(int i)
{
    switch (i)
    {
      case 0:
        return m_pblock;
      
      default:
        return nullptr;
    }
}

TSTR AppleseedRenderElement::SubAnimName(int i)
{
    switch (i)
    {
      case 0:
        return m_pblock->GetLocalName();

      default:
        return L"";
    }
}

int AppleseedRenderElement::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedRenderElement::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedRenderElement::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* AppleseedRenderElement::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedRenderElement::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedRenderElement::GetReference(int i)
{
    switch (i)
    {
      case ParamBlockIdRenderElement:
        return m_pblock;

      default:
        return nullptr;
    }
}

void AppleseedRenderElement::SetReference(int i, RefTargetHandle rtarg)
{
    switch (i)
    {
      case ParamBlockIdRenderElement:
        m_pblock = static_cast<IParamBlock2*>(rtarg);
        break;
    }
}

IOResult AppleseedRenderElement::Load(ILoad* iload)
{
    return IRenderElement::Load(iload);
}

IOResult AppleseedRenderElement::Save(ISave* isave)
{
    return IRenderElement::Save(isave);
}

RefResult AppleseedRenderElement::NotifyRefChanged(
    const Interval&   changeInt,
    RefTargetHandle   hTarget,
    PartID&           partID,
    RefMessage        message,
    BOOL              propagate)
{
    return REF_SUCCEED;
}

RefTargetHandle AppleseedRenderElement::Clone(RemapDir& remap)
{
    AppleseedRenderElement* clone = new AppleseedRenderElement(false);

    const int num_refs = NumRefs();
    for (int i = 0; i < num_refs; ++i)
    {
        clone->ReplaceReference(i, remap.CloneRef(GetReference(i)));
    }
    BaseClone(this, clone, remap);

    return clone;
}

void* AppleseedRenderElement::GetInterface(ULONG id)
{
    return SpecialFX::GetInterface(id);
}

void AppleseedRenderElement::ReleaseInterface(ULONG id, void* i)
{
    return SpecialFX::ReleaseInterface(id, i);
}

void AppleseedRenderElement::SetEnabled(BOOL on)
{ 
    m_pblock->SetValue(ParamIdEnableOn, 0, on);
}

BOOL AppleseedRenderElement::IsEnabled() const
{
    int is_on;
    m_pblock->GetValue(ParamIdEnableOn, 0, is_on, FOREVER);
    return is_on;
}

void AppleseedRenderElement::SetFilterEnabled(BOOL on)
{
}

BOOL AppleseedRenderElement::IsFilterEnabled() const
{
    return true;
}

BOOL AppleseedRenderElement::BlendOnMultipass() const
{ 
    return true;
}

BOOL AppleseedRenderElement::AtmosphereApplied() const
{ 
    return true;
}

BOOL AppleseedRenderElement::ShadowsApplied() const
{ 
    return true;
}

void AppleseedRenderElement::SetElementName(const MCHAR* element_name)
{
    m_pblock->SetValue(ParamIdElementName, 0, element_name);
}

const TCHAR* AppleseedRenderElement::ElementName() const
{
    const TCHAR* name_str = nullptr;
    m_pblock->GetValue(ParamIdElementName, 0, name_str, FOREVER);
    return name_str;
}

void AppleseedRenderElement::SetPBBitmap(PBBitmap*& bitmap) const
{
    if (DbgVerify(m_pblock != nullptr))
        m_pblock->SetValue(ParamIdElementBitmap, 0, bitmap);
}

void AppleseedRenderElement::GetPBBitmap(PBBitmap*& bitmap) const
{
    if (DbgVerify(m_pblock != nullptr))
    {
        Interval dummy_validity;
        bitmap = m_pblock->GetBitmap(ParamIdElementBitmap, 0, dummy_validity);
    }
    else
    {
        bitmap = nullptr;
    }
}

IRenderElementParamDlg* AppleseedRenderElement::CreateParamDialog(IRendParams* imp)
{
    return g_appleseed_renderelement_classdesc.CreateParamDialogs(imp, this);
}


//
// AppleseedRenderElementClassDesc class implementation.
//

AppleseedRenderElementClassDesc::AppleseedRenderElementClassDesc()
{
}

int AppleseedRenderElementClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedRenderElementClassDesc::Create(BOOL loading)
{
    return new AppleseedRenderElement(loading == TRUE);
}

const MCHAR* AppleseedRenderElementClassDesc::ClassName()
{
    return AppleseedRenderElementFriendlyClassName;
}

SClass_ID AppleseedRenderElementClassDesc::SuperClassID()
{
    return RENDER_ELEMENT_CLASS_ID;
}

Class_ID AppleseedRenderElementClassDesc::ClassID()
{
    return AppleseedRenderElement::get_class_id();
}

const MCHAR* AppleseedRenderElementClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedRenderElementClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedRenderElement";
}

HINSTANCE AppleseedRenderElementClassDesc::HInstance()
{
    return g_module;
}
