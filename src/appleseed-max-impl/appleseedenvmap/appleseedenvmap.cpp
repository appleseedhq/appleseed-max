
#include "renderer/modeling/environmentedf/hosekenvironmentedf.h"

#include "appleseedenvmap.h"
#include "appleseedenvmap/resource.h"
#include "main.h"
#include "utilities.h"

namespace
{
    const TCHAR* AppleseedEnvMapFriendlyClassName = _T("appleseed Environment Map");
}

AppleseedEnvMapClassDesc g_appleseed_appleseedenvmap_classdesc;

//
// AppleseedEnvMap class implementation.
//

namespace
{
    enum { ParamBlockIdEnvMap };
    enum { ParamBlockRefEnvMap };

    enum ParamId 
    {
        ParamIdSunTheta             = 0,
        ParamIdSunPhi               = 1,
        ParamIdTurbidity            = 2,
        ParamIdTurbidityMap         = 3,
        ParamIdTurbidityMapOn       = 4,
        ParamIdTurbMultiplier       = 5,
        ParamIdLuminMultiplier      = 6,
        ParamIdLuminGamma           = 7,
        ParamIdSatMultiplier        = 8,
        ParamIdHorizonShift         = 10,
        ParamIdGroundAlbedo         = 11
    };

    enum TexmapId
    {
        TexmapIdTurbidity           = 0,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        _T("Turbidity"),
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdTurbidityMap
    };

    ParamBlockDesc2 g_block_desc(
        ParamBlockIdEnvMap,
        _T("appleseedEnvironmentMapParams"), 
        0,
        &g_appleseed_appleseedenvmap_classdesc,
        P_AUTO_CONSTRUCT + P_AUTO_UI,

         // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefEnvMap,                     // parameter block's reference number

        // --- P_AUTO_UI arguments for Disney rollup ---
        IDD_ENVMAP_PANEL,                        // ID of the dialog template
        IDS_ENVMAP_PARAMS,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,   

        // params
        ParamIdSunTheta, _T("sun_theta"), TYPE_FLOAT, P_ANIMATABLE, IDS_THETA,
            p_default, 45.0f,
            p_range, 0.0f, 180.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_THETA, IDC_SPIN_THETA, 0.01f,
        p_end,

        ParamIdSunPhi, _T("sun_phi"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHI,
            p_default, 0.0f,
            p_range, 0.0f, 360.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_PHI, IDC_SPIN_PHI, 0.01f,
        p_end,

        ParamIdTurbidity, _T("turbitidy"), TYPE_FLOAT, P_ANIMATABLE, IDS_TURBIDITY,
            p_default, 1.0f,
            p_range, 0.0f, 1.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TURBIDITY, IDC_SPIN_TURBIDITY, 0.01f,
        p_end,

        ParamIdTurbidityMap, _T("turbitidy_map"), TYPE_TEXMAP, 0, IDS_TURB_MAP,
            p_subtexno, TexmapIdTurbidity,
            p_ui, TYPE_TEXMAPBUTTON, IDC_PICK_TURB_TEXTURE,
        p_end,

        ParamIdTurbidityMapOn, _T("turbitidy_map_on"), TYPE_BOOL, 0, IDS_TURB_MAP_ON,
            p_default, TRUE,
            p_ui, TYPE_SINGLECHEKBOX, IDC_TURB_TEX_ON,
        p_end,

        ParamIdTurbMultiplier, _T("turbitidy_multiplier"), TYPE_FLOAT, P_ANIMATABLE, IDS_TURB_MULTIPLIER,
            p_default, 2.0f,
            p_range, 0.0f, 8.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TURB_MULTIPLIER, IDC_SPIN_TURB_MULTIPLIER, 0.01f,
        p_end,

        ParamIdLuminMultiplier, _T("luminance_multiplier"), TYPE_FLOAT, P_ANIMATABLE, IDS_LUMINANCE_MULTIPLIER,
            p_default, 1.0f,
            p_range, 0.0f, 10.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_LUMIN_MULTIPLIER, IDC_SPIN_LUMIN_MULTIPLIER, 0.01f,
        p_end,

        ParamIdLuminGamma, _T("luminance_gamma"), TYPE_FLOAT, P_ANIMATABLE, IDS_LUMINANCE_GAMMA,
            p_default, 1.0f,
            p_range, 0.0f, 3.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_LUMIN_GAMMA, IDC_SPIN_LUMIN_GAMMA, 0.01f,
        p_end,

        ParamIdSatMultiplier, _T("saturation_multiplier"), TYPE_FLOAT, P_ANIMATABLE, IDS_SAT_MULITPLIER,
            p_default, 1.0f,
            p_range, 0.0f, 10.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_SATUR_MULTIPLIER, IDC_SPIN_SATUR_MULTIPLIER, 0.01f,
        p_end,

        ParamIdHorizonShift, _T("horizon_shift"), TYPE_FLOAT, P_ANIMATABLE, IDS_HORIZON_SHIFT,
            p_default, 0.0f,
            p_range, -10.0f, 10.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_HORIZON_SHIFT, IDC_SPIN_HORIZON_SHIFT, 0.01f,
        p_end,

        ParamIdGroundAlbedo, _T("ground_albedo"), TYPE_FLOAT, P_ANIMATABLE, IDS_GROUND_ALBEDO,
            p_default, 0.3f,
            p_range, 0.0f, 1.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_GROUND_ALBEDO, IDC_SPIN_GROUND_ALBEDO, 0.01f,
        p_end,

        p_end
    );
}

Class_ID AppleseedEnvMap::get_class_id()
{
    return Class_ID(0x52848b4a, 0x5e6cb361);
}

AppleseedEnvMap::AppleseedEnvMap()
    : m_pblock(nullptr)
    , m_sun_theta(45.0f)
    , m_sun_phi(90.0f)
    , m_turbidity(1.0f)
    , m_turbidity_map(nullptr)
    , m_turbidity_map_on(true)
    , m_turb_multiplier(2.0f)
    , m_lumin_multiplier(1.0f)
    , m_lumin_gamma(1.0f)
    , m_sat_multiplier(1.0f)
    , m_horizon_shift(0.0f)
    , m_ground_albedo(0.3f)
{
    g_appleseed_appleseedenvmap_classdesc.MakeAutoParamBlocks(this);
    Reset();
}

void AppleseedEnvMap::DeleteThis()
{
    delete this;
}

void AppleseedEnvMap::GetClassName(TSTR& s)
{
    s = _T("appleseedEnvMap");
}

SClass_ID AppleseedEnvMap::SuperClassID()
{
    return TEXMAP_CLASS_ID;
}

Class_ID AppleseedEnvMap::ClassID()
{
    return get_class_id();
}

int AppleseedEnvMap::NumSubs()
{
    return 2; //pblock + subtexture
}

Animatable* AppleseedEnvMap::SubAnim(int i)
{
    switch (i) {
    case 0: return m_pblock;
    case 1: return m_turbidity_map;
    default: return nullptr;
    }
}

TSTR AppleseedEnvMap::SubAnimName(int i)
{
    switch (i) {
    case 0: return _T("Parameters");
    case 1: return GetSubTexmapTVName(i - 1);
    default: return nullptr;
    }
}

int AppleseedEnvMap::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedEnvMap::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedEnvMap::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* AppleseedEnvMap::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedEnvMap::NumRefs()
{
    return 2; //pblock + subtexture
}

RefTargetHandle AppleseedEnvMap::GetReference(int i)
{
    switch (i) {
        case ParamBlockIdEnvMap: return m_pblock;
        case 1: return m_turbidity_map;
        default: return nullptr;
    }
}

void AppleseedEnvMap::SetReference(int i, RefTargetHandle rtarg)
{
    switch (i) {
        case ParamBlockIdEnvMap: m_pblock = (IParamBlock2 *)rtarg; break;
        case 1: m_turbidity_map = (Texmap *)rtarg; break;
    }
}

RefResult AppleseedEnvMap::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, PartID& /*partID*/, RefMessage message, BOOL /*propagate*/)
{
    switch (message)
    {
        case REFMSG_TARGET_DELETED:
            if (hTarget == m_pblock) 
            { 
                m_pblock = nullptr; 
            }
            else
            {
                if (m_turbidity_map == hTarget)
                {
                    m_turbidity_map = nullptr;
                    break;
                }
            }
            break;
        case REFMSG_CHANGE:
            m_params_validity.SetEmpty();
            m_map_validity.SetEmpty();
            if (hTarget == m_pblock)
                g_block_desc.InvalidateUI(m_pblock->LastNotifyParamID());
            break;
    }
    return(REF_SUCCEED);
}

RefTargetHandle AppleseedEnvMap::Clone(RemapDir &remap)
{
    AppleseedEnvMap *mnew = new AppleseedEnvMap();
    *static_cast<MtlBase*>(mnew) = *static_cast<MtlBase*>(this);

    mnew->ReplaceReference(0, remap.CloneRef(m_pblock));
    mnew->ReplaceReference(1, remap.CloneRef(m_turbidity_map));

    BaseClone(this, mnew, remap);
    return (RefTargetHandle)mnew;
}

int AppleseedEnvMap::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedEnvMap::GetSubTexmap(int i)
{
    return (i == 0) ? m_turbidity_map : nullptr;
}

void AppleseedEnvMap::SetSubTexmap(int i, Texmap* texmap)
{
    if (i > 0)
      return;
    ReplaceReference(i + 1, texmap);

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);
    g_block_desc.InvalidateUI( param_id );
    m_map_validity.SetEmpty();
}

int AppleseedEnvMap::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

TSTR AppleseedEnvMap::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedEnvMap::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdSunTheta, t, m_sun_theta, m_params_validity);
        m_pblock->GetValue(ParamIdSunPhi, t, m_sun_phi, m_params_validity);

        m_pblock->GetValue(ParamIdTurbidity, t, m_turbidity, m_params_validity);
        m_pblock->GetValue(ParamIdTurbidityMap, t, m_turbidity_map, m_params_validity);
        m_pblock->GetValue(ParamIdTurbidityMapOn, t, m_turbidity_map_on, m_params_validity);

        m_pblock->GetValue(ParamIdTurbMultiplier, t, m_turb_multiplier, m_params_validity);
        m_pblock->GetValue(ParamIdLuminMultiplier, t, m_lumin_multiplier, m_params_validity);
        m_pblock->GetValue(ParamIdLuminGamma, t, m_lumin_gamma, m_params_validity);
        m_pblock->GetValue(ParamIdSatMultiplier, t, m_sat_multiplier, m_params_validity);
        m_pblock->GetValue(ParamIdHorizonShift, t, m_horizon_shift, m_params_validity);
        m_pblock->GetValue(ParamIdGroundAlbedo, t, m_ground_albedo, m_params_validity);

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }

    if (!m_map_validity.InInterval(t))
	{
		m_map_validity.SetInfinite();
	    if (m_turbidity_map)
	        m_turbidity_map->Update(t,m_map_validity);
	}

    valid &= m_map_validity;
    valid &= m_params_validity;
}

void AppleseedEnvMap::Reset()
{
    m_params_validity.SetEmpty();
    m_map_validity.SetEmpty();
}

Interval AppleseedEnvMap::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedEnvMap::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    IAutoMParamDlg* masterDlg = g_appleseed_appleseedenvmap_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    g_block_desc.SetUserDlgProc(new EnvMapParamMapDlgProc());

    return masterDlg;
}

const USHORT ChunkFileFormatVersion                 = 0x0001;
const USHORT ChunkMtlBase                           = 0x1000;
const USHORT FileFormatVersion = 0x0001;

IOResult AppleseedEnvMap::Save(ISave* isave)
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

IOResult AppleseedEnvMap::Load(ILoad* iload)
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

AColor AppleseedEnvMap::EvalColor(ShadeContext& sc)
{
    Color basecolor (0.13f, 0.58f, 1.0f);
    if (!sc.InMtlEditor())
        return basecolor;

    //render gradient for the thumbnail
    Point3 p = sc.UVW(0);
    Color white (1.0f, 1.0f, 1.0f);

	return AColor((basecolor * (float)p.y) + (white * (float)(1.0-p.y)));
}

Point3 AppleseedEnvMap::EvalNormalPerturb(ShadeContext& /*sc*/)
{
    return Point3(0, 0, 0);
}

foundation::auto_release_ptr<renderer::EnvironmentEDF> AppleseedEnvMap::create_envmap(const char* name)
{
    renderer::ParamArray map_params;

    map_params.insert("sun_theta", m_sun_theta);
    map_params.insert("sun_phi", m_sun_phi);
    map_params.insert("turbidity", m_turbidity);
    map_params.insert("turbidity_multiplier", m_turb_multiplier);
    map_params.insert("ground_albedo", m_ground_albedo);
    map_params.insert("luminance_multiplier", m_lumin_multiplier);
    map_params.insert("luminance_gamma", m_lumin_gamma);
    map_params.insert("saturation_multiplier", m_sat_multiplier);
    map_params.insert("horizon_shift", m_horizon_shift);

    auto envmap = renderer::HosekEnvironmentEDFFactory::static_create(name, map_params);

    return envmap;
}
//
// AppleseedEnvMapBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedEnvMapBrowserEntryInfo::GetEntryName() const
{
    return AppleseedEnvMapFriendlyClassName;
}

const MCHAR* AppleseedEnvMapBrowserEntryInfo::GetEntryCategory() const
{
    return _T("Maps\\appleseed");
}

Bitmap* AppleseedEnvMapBrowserEntryInfo::GetEntryThumbnail() const
{
    return nullptr;
}

//
// EnvMapParamMapDlgProc class implementation.
//

INT_PTR EnvMapParamMapDlgProc::DlgProc(
        TimeValue   t,
        IParamMap2* map,
        HWND        hwnd,
        UINT        umsg,
        WPARAM      wparam,
        LPARAM      lparam)
{
    switch (umsg)
    {
    case WM_INITDIALOG:
        enable_disable_controls(hwnd, map);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_COMBO_SKY_TYPE:
            switch (HIWORD(wparam))
            {
            case CBN_SELCHANGE:
                enable_disable_controls(hwnd, map);
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

void EnvMapParamMapDlgProc::enable_disable_controls(HWND hwnd, IParamMap2* map)
{
    auto selected = SendMessage(GetDlgItem(hwnd, IDC_COMBO_SKY_TYPE), CB_GETCURSEL, 0, 0);
	map->Enable(ParamIdGroundAlbedo, selected == 0 ? TRUE : FALSE);
}

//
// AppleseedEnvMapClassDesc class implementation.
//

int AppleseedEnvMapClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedEnvMapClassDesc::Create(BOOL loading)
{
    return new AppleseedEnvMap();
}

const MCHAR* AppleseedEnvMapClassDesc::ClassName()
{
    return AppleseedEnvMapFriendlyClassName;
}

SClass_ID AppleseedEnvMapClassDesc::SuperClassID()
{
    return TEXMAP_CLASS_ID;
}

Class_ID AppleseedEnvMapClassDesc::ClassID()
{
    return AppleseedEnvMap::get_class_id();
}

const MCHAR* AppleseedEnvMapClassDesc::Category()
{
    return _T("");
}

const MCHAR* AppleseedEnvMapClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return _T("appleseedEnvMap");
}

FPInterface* AppleseedEnvMapClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedEnvMapClassDesc::HInstance()
{
    return g_module;
}