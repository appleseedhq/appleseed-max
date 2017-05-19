#pragma once

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"
#include "renderer/modeling/environmentedf/environmentedf.h"

// 3ds Max headers.
#include "appleseedenvmap/resource.h"
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <maxtypes.h>
#include <stdmat.h>
#include <imtl.h>
#include <IMaterialBrowserEntryInfo.h>
#undef base_type

//namespace renderer  { class EnvironmentEDF; }

class AppleseedEnvMap : public Texmap {
public:
    static Class_ID get_class_id();

    // Constructor.
    AppleseedEnvMap();

    // Animatable methods.
    virtual void DeleteThis() override;
    virtual void GetClassName(TSTR& s) override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID  ClassID() override;
    virtual int NumSubs() override;
    virtual Animatable* SubAnim(int i) override;
    virtual TSTR SubAnimName(int i) override;
    virtual int SubNumToRefNum(int subNum) override;
    virtual int	NumParamBlocks() override;
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
    virtual RefTargetHandle Clone(RemapDir &remap);
    
    // ISubMap methods.
    virtual int NumSubTexmaps() override;
    virtual Texmap* GetSubTexmap(int i) override;
    virtual void SetSubTexmap(int i, Texmap* texmap) override;
    virtual int MapSlotType(int i) override;
    virtual MSTR GetSubTexmapSlotName(int i) override;

    // MtlBase methods.
    virtual void Update(TimeValue t, Interval& valid);
    virtual void Reset();
    virtual Interval Validity(TimeValue t);
    virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp);

    // Loading/Saving
    virtual IOResult Save(ISave *isave);
    virtual IOResult Load(ILoad *iload);
    
    //From Texmap
    virtual RGBA   EvalColor(ShadeContext& sc);
    virtual Point3 EvalNormalPerturb(ShadeContext& sc);

    virtual foundation::auto_release_ptr<renderer::EnvironmentEDF> create_envmap(const char* name);

protected:
    virtual void SetReference(int i, RefTargetHandle rtarg);

private:
    IParamBlock2*    m_pblock;          // ref 0
    Interval         m_params_validity;
    Interval         m_map_validity;

    float m_sun_theta;
    float m_sun_phi;
    float m_turbidity;
    Texmap* m_turbidity_map;
    BOOL m_turbidity_map_on;
    float m_turb_multiplier;
    float m_lumin_multiplier;
    float m_lumin_gamma;
    float m_sat_multiplier;
    float m_horizon_shift;
    float m_ground_albedo;
};


class EnvMapParamMapDlgProc
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
        LPARAM      lparam) override;
private:
    void enable_disable_controls(HWND hwnd, IParamMap2* map);
};

//
// AppleseedEnvMap material browser info.
//

class AppleseedEnvMapBrowserEntryInfo
    : public IMaterialBrowserEntryInfo
{
public:
    virtual const MCHAR* GetEntryName() const override;
    virtual const MCHAR* GetEntryCategory() const override;
    virtual Bitmap* GetEntryThumbnail() const override;
};

class AppleseedEnvMapClassDesc
    : public ClassDesc2
{
public:
    virtual int IsPublic() override;
    virtual void* Create(BOOL /*loading = FALSE*/) override;
    virtual const TCHAR *	ClassName() override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual const TCHAR* Category() override;
    virtual const TCHAR* InternalName() override;
    virtual FPInterface* GetInterface(Interface_ID id) override;
    virtual HINSTANCE HInstance() override;
private:
    AppleseedEnvMapBrowserEntryInfo m_browser_entry_info;
};

extern AppleseedEnvMapClassDesc g_appleseed_appleseedenvmap_classdesc;



