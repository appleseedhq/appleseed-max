
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
#include "osloutputselector.h"

// appleseed-max headers.
#include "appleseedoslplugin/osltexture.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "main.h"
#include "osloutputselectormap/datachunks.h"
#include "osloutputselectormap/resource.h"
#include "utilities.h"
#include "version.h"

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const wchar_t* OSLOutputSelectorFriendlyClassName = L"OSL Output Selector";

    enum { ParamBlockIdOutputSelector };
    enum { ParamBlockRefOutputSelector };

    enum ParamId
    {
        ParamIdSourceMap = 0,
        ParamIdOutputIndex = 1,
    };

    enum TexmapId
    {
        TexmapIdSourceMap = 0,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        L"SourceMap",
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdSourceMap
    };

    class OutputIndexAccessor
      : public PBAccessor
    {
      public:
        void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
        {
            IParamBlock2* pblock = static_cast<OSLOutputSelector*>(owner)->GetParamBlock(0);
            if (pblock == nullptr)
                return;

            IParamMap2* map = pblock->GetMap();
            if (map == nullptr)
                return;

            switch (id)
            {
                case ParamIdOutputIndex:
                {
                    Texmap* texmap = nullptr;
                    pblock->GetValue(ParamIdSourceMap, t, texmap, FOREVER);

                    if (texmap != nullptr && is_osl_texture(texmap))
                    {
                        auto output_names = static_cast<OSLTexture*>(texmap)->get_output_names();

                        int output_index = v.i;
                        if (!output_names.empty())
                        {
                            HWND dlg_hwnd = 0;
                            dlg_hwnd = map->GetHWnd();
                            if (dlg_hwnd != 0)
                            {
                                SendMessage(
                                    GetDlgItem(dlg_hwnd, IDC_COMBO_OUTPUT_NAME),
                                    CB_SETCURSEL,
                                    (output_index - 1) % output_names.size(),
                                    0);
                            }
                        }
                    }
                }
            }
        }
    };
}

OSLOutputSelectorClassDesc g_appleseed_outputselector_classdesc;
OutputIndexAccessor g_output_index_accessor;

//
// OSLOutputSelector class implementation.
//

namespace
{
    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdOutputSelector,                 // parameter block's ID
        L"appleseedOuputSelectorParams",            // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_outputselector_classdesc,      // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

         // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefOutputSelector,                // parameter block's reference number

        // --- P_AUTO_UI arguments for Parameters rollup ---
        IDD_OUTPUTSELECTOR_PANEL,                   // ID of the dialog template
        IDS_OUTPUTSELECTOR_PARAMS,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,

        // --- Parameters specifications for Parameters rollup ---
        ParamIdOutputIndex, L"output_index", TYPE_INT, P_ANIMATABLE, IDS_OUTPUT_INDEX,
            p_default, 1,
            p_range, 1, 100,
            p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_OUTPUT_INDEX, IDC_SPIN_OUTPUT_INDEX, 1,
            p_accessor, &g_output_index_accessor,
        p_end,

        ParamIdSourceMap, L"source_map", TYPE_TEXMAP, 0, IDS_SOURCE_MAP,
            p_subtexno, TexmapIdSourceMap,
            p_ui, TYPE_TEXMAPBUTTON, IDC_SOURCE_MAP,
        p_end,
        
        p_end
    );

    void init_combo_box(IParamMap2* param_map)
    {
        HWND dlg_hwnd = 0;
        if (param_map != nullptr)
            dlg_hwnd = param_map->GetHWnd();
        if (dlg_hwnd == 0)
            return;

        const auto time = GetCOREInterface()->GetTime();
        Texmap* texmap = nullptr;
        param_map->GetParamBlock()->GetValue(ParamIdSourceMap, time, texmap, FOREVER);

        if (texmap != nullptr && is_osl_texture(texmap))
        {
            auto output_names = static_cast<OSLTexture*>(texmap)->get_output_names();
            for (const auto& name : output_names)
                SendMessage(GetDlgItem(dlg_hwnd, IDC_COMBO_OUTPUT_NAME), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(utf8_to_wide(name).c_str()));

            int output_index = param_map->GetParamBlock()->GetInt(ParamIdOutputIndex, time, FOREVER);
            SendMessage(GetDlgItem(dlg_hwnd, IDC_COMBO_OUTPUT_NAME), CB_SETCURSEL, (output_index - 1) % output_names.size(), 0);
        }
        else
        {
            SendMessage(GetDlgItem(dlg_hwnd, IDC_COMBO_OUTPUT_NAME), CB_RESETCONTENT, 0, 0);
        }
    }

    class OutputSelectorDlgProc
        : public ParamMap2UserDlgProc
    {
    public:
        virtual void DeleteThis() override
        {
            delete this;
        }

        virtual INT_PTR DlgProc(
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
                  case IDC_COMBO_OUTPUT_NAME:
                    switch (HIWORD(wparam))
                    {
                      case CBN_SELCHANGE:
                        {
                            LRESULT selected = SendMessage(GetDlgItem(hwnd, IDC_COMBO_OUTPUT_NAME), CB_GETCURSEL, 0, 0);
                            map->GetParamBlock()->SetValue(ParamIdOutputIndex, t, static_cast<int>(selected + 1));
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
}

Class_ID OSLOutputSelector::get_class_id()
{
    return Class_ID(0x53035b67, 0x7b906906);
}

OSLOutputSelector::OSLOutputSelector()
  : m_pblock(nullptr)
{
    g_appleseed_outputselector_classdesc.MakeAutoParamBlocks(this);
    Reset();
}

void OSLOutputSelector::DeleteThis()
{
    delete this;
}

void OSLOutputSelector::GetClassName(TSTR& s)
{
    s = L"appleseedOSLOutputSelector";
}

SClass_ID OSLOutputSelector::SuperClassID()
{
    return TEXMAP_CLASS_ID;
}

Class_ID OSLOutputSelector::ClassID()
{
    return get_class_id();
}

int OSLOutputSelector::NumSubs()
{
    return 1;   // pblock
}

Animatable* OSLOutputSelector::SubAnim(int i)
{
    switch (i)
    {
      case 0:
        return m_pblock;
      
      default:
        return nullptr;
    }
}

TSTR OSLOutputSelector::SubAnimName(int i)
{
    switch (i)
    {
      case 0:
        return L"Parameters";

      default:
        return nullptr;
    }
}

int OSLOutputSelector::SubNumToRefNum(int subNum)
{
    return subNum;
}

int OSLOutputSelector::NumParamBlocks()
{
    return 1;
}

IParamBlock2* OSLOutputSelector::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* OSLOutputSelector::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int OSLOutputSelector::NumRefs()
{
    return 1;   // pblock
}

RefTargetHandle OSLOutputSelector::GetReference(int i)
{
    switch (i)
    {
      case ParamBlockIdOutputSelector:
        return m_pblock;

      default:
        return nullptr;
    }
}

void OSLOutputSelector::SetReference(int i, RefTargetHandle rtarg)
{
    switch (i)
    {
      case ParamBlockIdOutputSelector:
        m_pblock = (IParamBlock2 *)rtarg;
        break;
    }
}

RefResult OSLOutputSelector::NotifyRefChanged(
    const Interval&   /*changeInt*/,
    RefTargetHandle   hTarget,
    PartID&           partID,
    RefMessage        message,
    BOOL              /*propagate*/)
{
    switch (message)
    {
      case REFMSG_TARGET_DELETED:
        if (hTarget == m_pblock)
            m_pblock = nullptr;
        break;

      case REFMSG_CHANGE:
        if (hTarget == m_pblock)
            g_block_desc.InvalidateUI(m_pblock->LastNotifyParamID());
        break;
    }

    return REF_SUCCEED;
}

RefTargetHandle OSLOutputSelector::Clone(RemapDir& remap)
{
    OSLOutputSelector* mnew = new OSLOutputSelector();
    *static_cast<MtlBase*>(mnew) = *static_cast<MtlBase*>(this);
    BaseClone(this, mnew, remap);

    mnew->ReplaceReference(0, remap.CloneRef(m_pblock));

    return (RefTargetHandle)mnew;
}

int OSLOutputSelector::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* OSLOutputSelector::GetSubTexmap(int i)
{
    Texmap* texmap = nullptr;
    Interval iv;
    if (i == 0)
    {
        m_pblock->GetValue(ParamIdSourceMap, 0, texmap, iv);
    }
    return texmap;
}

void OSLOutputSelector::SetSubTexmap(int i, Texmap* texmap)
{
    if (i == 0)
    {
        const auto texmap_id = static_cast<TexmapId>(i);
        const auto param_id = g_texmap_id_to_param_id[texmap_id];
        m_pblock->SetValue(param_id, 0, texmap);

        init_combo_box(m_pblock->GetMap(ParamBlockIdOutputSelector));

        g_block_desc.InvalidateUI(param_id);
    }
}

int OSLOutputSelector::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

TSTR OSLOutputSelector::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void OSLOutputSelector::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }
    m_params_validity.SetInfinite();

    Texmap* source_map;
    m_pblock->GetValue(ParamIdSourceMap, t, source_map, m_params_validity);

    if (source_map)
        source_map->Update(t, m_params_validity);

    valid = m_params_validity;
}

void OSLOutputSelector::Reset()
{
    m_params_validity.SetEmpty();
}

Interval OSLOutputSelector::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* OSLOutputSelector::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    IAutoMParamDlg* master_dlg = g_appleseed_outputselector_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    g_block_desc.SetUserDlgProc(ParamBlockIdOutputSelector, new OutputSelectorDlgProc());
    return master_dlg;
}

IOResult OSLOutputSelector::Save(ISave* isave)
{
    bool success = true;

    isave->BeginChunk(ChunkFileFormatVersion);
    success &= write(isave, &FileFormatVersion, sizeof(FileFormatVersion));
    isave->EndChunk();

    isave->BeginChunk(ChunkMtlBase);
    success &= MtlBase::Save(isave) == IO_OK;
    isave->EndChunk();

    return success ? IO_OK : IO_ERROR;
}

IOResult OSLOutputSelector::Load(ILoad* iload)
{
    IOResult result = IO_OK;

    while (true)
    {
        result = iload->OpenChunk();
        if (result == IO_END)
            return IO_OK;
        if (result != IO_OK)
            break;

        switch (iload->CurChunkID())
        {
          case ChunkFileFormatVersion:
            {
                USHORT version;
                result = read<USHORT>(iload, &version);
            }
            break;

          case ChunkMtlBase:
            result = MtlBase::Load(iload);
            break;
        }

        if (result != IO_OK)
            break;

        result = iload->CloseChunk();
        if (result != IO_OK)
            break;
    }

    return result;
}

AColor OSLOutputSelector::EvalColor(ShadeContext& sc)
{
    return Color(0.13f, 0.58f, 1.0f);
}

Point3 OSLOutputSelector::EvalNormalPerturb(ShadeContext& /*sc*/)
{
    return Point3(0.0f, 0.0f, 0.0f);
}


//
// OSLOutputSelectorBrowserEntryInfo class implementation.
//

const MCHAR* OSLOutputSelectorBrowserEntryInfo::GetEntryName() const
{
    return OSLOutputSelectorFriendlyClassName;
}

const MCHAR* OSLOutputSelectorBrowserEntryInfo::GetEntryCategory() const
{
    return L"Maps\\appleseed OSL";
}

Bitmap* OSLOutputSelectorBrowserEntryInfo::GetEntryThumbnail() const
{
    return nullptr;
}


//
// OSLOutputSelectorClassDesc class implementation.
//

OSLOutputSelectorClassDesc::OSLOutputSelectorClassDesc()
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int OSLOutputSelectorClassDesc::IsPublic()
{
    return TRUE;
}

void* OSLOutputSelectorClassDesc::Create(BOOL loading)
{
    return new OSLOutputSelector();
}

const MCHAR* OSLOutputSelectorClassDesc::ClassName()
{
    return OSLOutputSelectorFriendlyClassName;
}

SClass_ID OSLOutputSelectorClassDesc::SuperClassID()
{
    return TEXMAP_CLASS_ID;
}

Class_ID OSLOutputSelectorClassDesc::ClassID()
{
    return OSLOutputSelector::get_class_id();
}

const MCHAR* OSLOutputSelectorClassDesc::Category()
{
    return L"";
}

const MCHAR* OSLOutputSelectorClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"OSLOutputSelector";
}

FPInterface* OSLOutputSelectorClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE OSLOutputSelectorClassDesc::HInstance()
{
    return g_module;
}

bool OSLOutputSelectorClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
