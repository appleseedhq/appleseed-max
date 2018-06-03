
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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
#include "appleseedrenderer.h"

// appleseed-max headers.
#include "appleseedinteractive/appleseedinteractive.h"
#include "appleseedrenderelement/appleseedrenderelement.h"
#include "appleseedrenderer/appleseedrendererparamdlg.h"
#include "appleseedrenderer/datachunks.h"
#include "appleseedrenderer/dialoglogtarget.h"
#include "appleseedrenderer/projectbuilder.h"
#include "appleseedrenderer/renderercontroller.h"
#include "appleseedrenderer/tilecallback.h"
#include "main.h"
#include "resource.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/frame.h"
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"

// appleseed.foundation headers.
#include "foundation/image/canvasproperties.h"
#include "foundation/image/image.h"
#include "foundation/platform/thread.h"
#include "foundation/platform/types.h"
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include <assert1.h>
#include <bitmap.h>
#include <interactiverender.h>
#include <iparamm2.h>
#include <notify.h>
#include <pbbitmap.h>
#include <renderelements.h>

// Standard headers.
#include <clocale>
#include <cstddef>
#include <string>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const Class_ID AppleseedRendererClassId(0x72651b24, 0x5da32e1d);
    const Class_ID AppleseedRendererTabClassId(0x6155c0c, 0xed6475b);
    const wchar_t* AppleseedRendererClassName = L"appleseed Renderer";

    enum ParamMapId
    {
        ParamMapIdOutput,
        ParamMapIdImageSampling,
        ParamMapIdLighting,
        ParamMapIdSystem
    };

    enum ParamId
    {
        ParamIdOuputMode                = 0,
        ParamIdProjectPath              = 1,
        ParamIdScaleMultiplier          = 2,
        
        ParamIdPixelSamples             = 3,
        ParamIdTileSize                 = 4,
        ParamIdPasses                   = 5,
        ParamIdFilterSize               = 6,
        ParamIdFilterType               = 7,
        
        ParamIdEnableGI                 = 8,
        ParamIdGIBounces                = 9,
        ParamIdEnableCaustics           = 10,
        ParamIdEnableMaxRayIntensity    = 11,
        ParamIdMaxRayIntensity          = 12,
        ParamIdForceDefaultLightsOff    = 13,
        ParamIdEnableBackgroundLight    = 14,
        ParamIdBackgroundAlphaValue     = 15,

        ParamIdCPUCores                 = 16,
        ParamIdOpenLogMode              = 17,
        ParamIdLogMaterialRendering     = 18,
        ParamIdUseMaxProcedurals        = 19,
        ParamIdEnableLowPriority        = 20,
        ParamIdEnableRenderStamp        = 21,
        ParamIdRenderStampFormat        = 22,
    };

    MCHAR* GetString(int id)
    {
        static wchar_t buf[256];

        if (g_module)
            return LoadString(g_module, id, buf, _countof(buf)) ? buf : nullptr;
        return nullptr;
    }
}

//
// AppleseedRendererPBlockAccessor class implementation.
//

void AppleseedRendererPBlockAccessor::Get(
    PB2Value&       v,
    ReferenceMaker* owner,
    ParamID         id,
    int             tab_index,
    TimeValue       t,
    Interval        &valid)
{
    AppleseedRenderer* const renderer = static_cast<AppleseedRenderer*>(owner);
    RendererSettings& settings = renderer->m_settings;

    switch (id)
    {
      case ParamIdOuputMode:
        v.i = static_cast<int>(settings.m_output_mode);
        break;

      case ParamIdProjectPath:
        v.s = settings.m_project_file_path;
        break;

      case ParamIdScaleMultiplier:
        v.f = settings.m_scale_multiplier;
        break;
        
    //
    // Image Sampling.
    //

      case ParamIdPixelSamples:
        v.i = settings.m_pixel_samples;
        break;

      case ParamIdTileSize:
        v.i = settings.m_tile_size;
        break;

      case ParamIdPasses:
        v.i = settings.m_passes;
        break;

    //
    // Pixel Filtering.
    //

      case ParamIdFilterType:
        v.i = settings.m_pixel_filter;
        break;

      case ParamIdFilterSize:
        v.f = settings.m_pixel_filter_size;
        break;

    //
    // Lighting.
    //

      case ParamIdEnableGI:
        v.i = static_cast<int>(settings.m_gi);
        break;
            
      case ParamIdEnableCaustics:
        v.i = static_cast<int>(settings.m_caustics);
        break;
            
      case ParamIdEnableMaxRayIntensity:
        v.i = static_cast<int>(settings.m_max_ray_intensity_set);
        break;

      case ParamIdEnableBackgroundLight:
        v.i = static_cast<int>(settings.m_background_emits_light);
        break;

      case ParamIdForceDefaultLightsOff:
          v.i = static_cast<int>(settings.m_force_off_default_lights);
        break;

      case ParamIdGIBounces:
        v.i = settings.m_bounces;
        break;

      case ParamIdMaxRayIntensity:
        v.f = settings.m_max_ray_intensity;
        break;

      case ParamIdBackgroundAlphaValue:
        v.f = settings.m_background_alpha;
        break;

    //
    // System.
    //

      case ParamIdCPUCores:
        v.i = settings.m_rendering_threads;
        break;

      case ParamIdUseMaxProcedurals:
        v.i = static_cast<int>(settings.m_use_max_procedural_maps);
        break;

      case ParamIdEnableLowPriority:
        v.i = static_cast<int>(settings.m_low_priority_mode);
        break;
            
      case ParamIdEnableRenderStamp:
        v.i = static_cast<int>(settings.m_enable_render_stamp);
        break;

      case ParamIdRenderStampFormat:
        v.s = settings.m_render_stamp_format;
        break;

      case ParamIdLogMaterialRendering:
        v.i = static_cast<int>(settings.m_log_material_editor_messages);
        break;

      case ParamIdOpenLogMode:
        v.i = static_cast<int>(settings.m_log_open_mode);
        break;

      default:
        break;
    }
}

void AppleseedRendererPBlockAccessor::Set(
    PB2Value&       v,
    ReferenceMaker* owner,
    ParamID         id,
    int             tab_index,
    TimeValue       t)
{
    AppleseedRenderer* const renderer = dynamic_cast<AppleseedRenderer*>(owner);
    RendererSettings& settings = renderer->m_settings;

    switch (id)
    {
      case ParamIdOuputMode:
        settings.m_output_mode = static_cast<RendererSettings::OutputMode>(v.i);
        break;

      case ParamIdProjectPath:
        settings.m_project_file_path = v.s;
        break;

      case ParamIdScaleMultiplier:
        settings.m_scale_multiplier = v.f;
        break;
        
    //
    // Image Sampling.
    //

      case ParamIdPixelSamples:
        settings.m_pixel_samples = v.i;
        break;

      case ParamIdTileSize:
        settings.m_tile_size = v.i;
        break;

      case ParamIdPasses:
        settings.m_passes = v.i;
        break;

    //
    // Pixel Filtering.
    //

      case ParamIdFilterType:
        settings.m_pixel_filter = v.i;
        break;

      case ParamIdFilterSize:
        settings.m_pixel_filter_size = v.f;
        break;

    //
    // Lighting.
    //

      case ParamIdEnableGI:
        settings.m_gi = v.i > 0;
        break;
            
      case ParamIdEnableCaustics:
        settings.m_caustics = v.i > 0;
        break;
            
      case ParamIdEnableMaxRayIntensity:
        settings.m_max_ray_intensity_set = v.i > 0;
        break;

      case ParamIdEnableBackgroundLight:
        settings.m_background_emits_light = v.i > 0;
        break;

      case ParamIdForceDefaultLightsOff:
        settings.m_force_off_default_lights = v.i > 0;
        break;

      case ParamIdGIBounces:
        settings.m_bounces = v.i;
        break;

      case ParamIdMaxRayIntensity:
        settings.m_max_ray_intensity = v.f;
        break;

      case ParamIdBackgroundAlphaValue:
        settings.m_background_alpha = v.f;
        break;

    //
    // System.
    //

      case ParamIdCPUCores:
        settings.m_rendering_threads = v.i;
        break;

      case ParamIdUseMaxProcedurals:
        settings.m_use_max_procedural_maps = v.i > 0;
        break;

      case ParamIdEnableLowPriority:
        settings.m_low_priority_mode = v.i > 0;
        break;
            
      case ParamIdEnableRenderStamp:
        settings.m_enable_render_stamp = v.i > 0;
        break;

      case ParamIdRenderStampFormat:
        settings.m_render_stamp_format = v.s;
        break;

      case ParamIdLogMaterialRendering:
        settings.m_log_material_editor_messages = v.i > 0;
        break;

      case ParamIdOpenLogMode:
        settings.m_log_open_mode = static_cast<DialogLogTarget::OpenMode>(v.i);
        break;

      default:
        break;
    }
}

AppleseedRendererClassDesc g_appleseed_renderer_classdesc;
asf::auto_release_ptr<DialogLogTarget> g_dialog_log_target;
#if MAX_RELEASE < 19000
AppleseedRECompatible g_appleseed_renderelement_compatible;
#endif
AppleseedRendererPBlockAccessor g_pblock_accessor;

ParamBlockDesc2 g_param_block_desc(
    // --- Required arguments ---
    0,                                          // parameter block's ID
    L"appleseed render parameters",             // internal parameter block's name
    0,                                          // ID of the localized name string
    &g_appleseed_renderer_classdesc,            // class descriptor
    P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP + P_VERSION,  // block flags

    1,                                          // --- P_VERSION arguments ---

                                                // --- P_AUTO_CONSTRUCT arguments ---
    0,                                          // parameter block's reference number

                                                // --- P_MULTIMAP arguments ---
    4,

    // --- P_AUTO_UI arguments for Parameters rollup ---

    ParamMapIdOutput,
    IDD_FORMVIEW_RENDERERPARAMS_OUTPUT,         // ID of the dialog template
    0,                                          // ID of the dialog's title string
    0,                                          // IParamMap2 creation/deletion flag mask
    0,                                          // rollup creation flag
    nullptr,

    ParamMapIdImageSampling,
    IDD_FORMVIEW_RENDERERPARAMS_IMAGESAMPLING,  // ID of the dialog template
    0,                                          // ID of the dialog's title string
    0,                                          // IParamMap2 creation/deletion flag mask
    0,                                          // rollup creation flag
    nullptr,

    ParamMapIdLighting,
    IDD_FORMVIEW_RENDERERPARAMS_LIGHTING,       // ID of the dialog template
    0,                                          // ID of the dialog's title string
    0,                                          // IParamMap2 creation/deletion flag mask
    0,                                          // rollup creation flag
    nullptr,

    ParamMapIdSystem,
    IDD_FORMVIEW_RENDERERPARAMS_SYSTEM,         // ID of the dialog template
    0,                                          // ID of the dialog's title string
    0,                                          // IParamMap2 creation/deletion flag mask
    0,                                          // rollup creation flag
    nullptr,

    // --- Parameters specifications for Output rollup ---

    ParamIdOuputMode, L"output_mode", TYPE_INT, 0, 0,
        p_ui, ParamMapIdOutput, TYPE_RADIO, 3, IDC_RADIO_RENDER, IDC_RADIO_SAVEPROJECT, IDC_RADIO_SAVEPROJECT_AND_RENDER,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdProjectPath, L"project_path", TYPE_STRING, 0, 0,
        p_ui, ParamMapIdOutput, TYPE_EDITBOX, IDC_TEXT_PROJECT_FILEPATH,
    p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdScaleMultiplier, L"scale_multiplier", TYPE_FLOAT, 0, 0,
        p_ui, ParamMapIdOutput, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_SCALE_MULTIPLIER, IDC_SPINNER_SCALE_MULTIPLIER, SPIN_AUTOSCALE,
        p_default, 1.0f,
        p_range, 1.0e-6f, 1.0e6f,
        p_accessor, &g_pblock_accessor,
    p_end,

    // --- Parameters specifications for Image Sampling rollup ---

    ParamIdPixelSamples, L"pixel_samples", TYPE_INT, 0, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_PIXEL_SAMPLES, IDC_SPINNER_PIXEL_SAMPLES, SPIN_AUTOSCALE,
        p_default, 16,
        p_range, 1, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdTileSize, L"tile_size", TYPE_INT, 0, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_TILE_SIZE, IDC_SPINNER_TILE_SIZE, SPIN_AUTOSCALE,
        p_default, 64,
        p_range, 1, 4096,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdPasses, L"passes", TYPE_INT, 0, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_PASSES, IDC_SPINNER_PASSES, SPIN_AUTOSCALE,
        p_default, 1,
        p_range, 1, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdFilterSize, L"filter_size", TYPE_FLOAT, 0, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_FILTER_SIZE, IDC_SPINNER_FILTER_SIZE, SPIN_AUTOSCALE,
        p_default, 1.5f,
        p_range, 1.0f, 20.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdFilterType, L"filter_type", TYPE_INT, 0, 0,
        p_ui, ParamMapIdImageSampling, TYPE_INT_COMBOBOX, IDC_COMBO_FILTER,
        8, IDS_RENDERERPARAMS_FILTER_TYPE_1, IDS_RENDERERPARAMS_FILTER_TYPE_2,
        IDS_RENDERERPARAMS_FILTER_TYPE_3, IDS_RENDERERPARAMS_FILTER_TYPE_4,
        IDS_RENDERERPARAMS_FILTER_TYPE_5, IDS_RENDERERPARAMS_FILTER_TYPE_6,
        IDS_RENDERERPARAMS_FILTER_TYPE_7, IDS_RENDERERPARAMS_FILTER_TYPE_8,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    // --- Parameters specifications for Lighting rollup ---

    ParamIdEnableGI, L"enable_global_illumination", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SINGLECHEKBOX, IDC_CHECK_GI,
        p_default, TRUE,
        p_enable_ctrls, 1, ParamIdGIBounces,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdGIBounces, L"global_illumination_bounces", TYPE_INT, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_BOUNCES, IDC_SPINNER_BOUNCES, SPIN_AUTOSCALE,
        p_default, 3,
        p_range, 0, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableCaustics, L"enable_caustics", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SINGLECHEKBOX, IDC_CHECK_CAUSTICS,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableMaxRayIntensity, L"enable_max_ray", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SINGLECHEKBOX, IDC_CHECK_MAX_RAY_INTENSITY,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdMaxRayIntensity,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdMaxRayIntensity, L"max_ray_value", TYPE_FLOAT, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_MAX_RAY_INTENSITY, IDC_SPINNER_MAX_RAY_INTENSITY, SPIN_AUTOSCALE,
        p_default, 1.0f,
        p_range, 0.0f, 1000.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdForceDefaultLightsOff, L"force_default_lights_off", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SINGLECHEKBOX, IDC_CHECK_FORCE_OFF_DEFAULT_LIGHT,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableBackgroundLight, L"enable_background_light", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SINGLECHEKBOX, IDC_CHECK_BACKGROUND_EMITS_LIGHT,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdBackgroundAlphaValue, L"background_alpha", TYPE_FLOAT, 0, 0,
        p_ui, ParamMapIdLighting, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_BACKGROUND_ALPHA, IDC_SPINNER_BACKGROUND_ALPHA, SPIN_AUTOSCALE,
        p_default, 0.0f,
        p_range, 0.0f, 1.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    // --- Parameters specifications for System rollup ---

    ParamIdCPUCores, L"cpu_cores", TYPE_INT, 0, 0,
        p_ui, ParamMapIdSystem, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_RENDERINGTHREADS, IDC_SPINNER_RENDERINGTHREADS, SPIN_AUTOSCALE,
        p_default, 0,
        p_range, -255, 256,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdOpenLogMode, L"log_open_mode", TYPE_INT, 0, 0,
        p_ui, ParamMapIdSystem, TYPE_INT_COMBOBOX, IDC_COMBO_LOG,
        3, IDS_RENDERERPARAMS_LOG_OPEN_MODE_1, IDS_RENDERERPARAMS_LOG_OPEN_MODE_2,
        IDS_RENDERERPARAMS_LOG_OPEN_MODE_3,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdLogMaterialRendering, L"log_material_editor_rendering", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_LOG_MATERIAL_EDITOR,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdUseMaxProcedurals, L"use_max_procedural_maps", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_USE_MAX_PROCEDURAL_MAPS,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,
    
    ParamIdEnableLowPriority, L"low_priority_mode", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_LOW_PRIORITY_MODE,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableRenderStamp, L"enable_render_stamp", TYPE_BOOL, 0, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_RENDER_STAMP,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdRenderStampFormat,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdRenderStampFormat, L"render_stamp_format", TYPE_STRING, 0, 0,
        p_ui, ParamMapIdSystem, TYPE_EDITBOX, IDC_TEXT_RENDER_STAMP,
        p_default, L"appleseed {lib-version} | Time: {render-time}",
        p_accessor, &g_pblock_accessor,
    p_end,
    
    p_end
);

//
// AppleseedRenderer class implementation.
//

Class_ID AppleseedRenderer::get_class_id()
{
    return AppleseedRendererClassId;
}

AppleseedRenderer::AppleseedRenderer()
  : m_settings(RendererSettings::defaults())
  , m_interactive_renderer(nullptr)
  , m_param_block(nullptr)
{
    g_appleseed_renderer_classdesc.MakeAutoParamBlocks(this);
    clear();
}

const RendererSettings& AppleseedRenderer::get_renderer_settings()
{
    return m_settings;
}

int AppleseedRenderer::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedRenderer::GetReference(int i)
{
    switch (i)
    {
      case 0:
        return m_param_block;
      default:
        DbgAssert(false);
        return nullptr;
    }
}


void AppleseedRenderer::SetReference(int i, RefTargetHandle rtarg)
{
    switch (i)
    {
      case 0:
        m_param_block = dynamic_cast<IParamBlock2*>(rtarg);
        break;
      default:
        DbgAssert(false);
        break;
    }
}
Class_ID AppleseedRenderer::ClassID()
{
    return AppleseedRendererClassId;
}

void AppleseedRenderer::GetClassName(MSTR& s)
{
    s = AppleseedRendererClassName;
}

void AppleseedRenderer::DeleteThis()
{
    delete m_interactive_renderer;
    delete this;
}

int AppleseedRenderer::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedRenderer::GetParamBlock(int i)
{
    switch (i)
    {
      case 0:
        return m_param_block;
      default:
        DbgAssert(false);
        return nullptr;
    }
}

IParamBlock2* AppleseedRenderer::GetParamBlockByID(BlockID id)
{
    switch (id)
    {
      case 0:
        return m_param_block;
      default:
        return nullptr;
    }
}
void* AppleseedRenderer::GetInterface(ULONG id)
{
#if MAX_RELEASE < 19000
    // This code is specific to 3ds Max 2016 3ds Max 2017 has a new API for that.
    if (id == I_RENDER_ID)
    {
        if (m_interactive_renderer == nullptr)
            m_interactive_renderer = new AppleseedInteractiveRender();

        return static_cast<IInteractiveRender*>(m_interactive_renderer);
    }
    if (id == IRenderElementCompatible::IID)
    {
        return &g_appleseed_renderelement_compatible;
    }
    else
#endif
    {
        return Renderer::GetInterface(id);
    }
}

BaseInterface* AppleseedRenderer::GetInterface(Interface_ID id)
{
    if (id == TAB_DIALOG_OBJECT_INTERFACE_ID)
        return static_cast<ITabDialogObject*>(this);
    else return Renderer::GetInterface(id);
}

#if MAX_RELEASE > 18000

bool AppleseedRenderer::IsStopSupported() const
{
    return false;
}

void AppleseedRenderer::StopRendering()
{
}

Renderer::PauseSupport AppleseedRenderer::IsPauseSupported() const
{
    return PauseSupport::None;
}

void AppleseedRenderer::PauseRendering()
{
}

void AppleseedRenderer::ResumeRendering()
{
}

bool AppleseedRenderer::HasRequirement(Requirement requirement)
{
    // todo: specify kRequirement_NoVFB and kRequirement_DontSaveRenderOutput when saving project instead of rendering.

    switch (requirement)
    {
      case kRequirement_Wants32bitFPOutput: return true;
      case kRequirement_SupportsConcurrentRendering: return true;
    }

    return false;
}

bool AppleseedRenderer::CompatibleWithAnyRenderElement() const
{
    return true;
}

bool AppleseedRenderer::CompatibleWithRenderElement(IRenderElement& render_element) const
{
    return render_element.ClassID() == AppleseedRenderElement::get_class_id();
}

IInteractiveRender* AppleseedRenderer::GetIInteractiveRender()
{
    if (m_interactive_renderer == nullptr)
        m_interactive_renderer = new AppleseedInteractiveRender();

    return static_cast<IInteractiveRender*>(m_interactive_renderer);
}

void AppleseedRenderer::GetVendorInformation(MSTR& info) const
{
    info = L"appleseed-max ";
    info += PluginVersionString;
}

void AppleseedRenderer::GetPlatformInformation(MSTR& info) const
{
}

#endif

RefTargetHandle AppleseedRenderer::Clone(RemapDir& remap)
{
    AppleseedRenderer* new_rend = static_cast<AppleseedRenderer*>(g_appleseed_renderer_classdesc.Create(false));
    
    DbgAssert(new_rend != nullptr);
    
    for (int i = 0, e = NumRefs(); i < e; ++i)
        new_rend->ReplaceReference(i, remap.CloneRef(GetReference(i)));
        
    BaseClone(this, new_rend, remap);

    return new_rend;
}

RefResult AppleseedRenderer::NotifyRefChanged(
    const Interval&         changeInt,
    RefTargetHandle         hTarget,
    PartID&                 partID,
    RefMessage              message,
    BOOL                    propagate)
{
    return REF_SUCCEED;
}

int AppleseedRenderer::Open(
    INode*                  scene,
    INode*                  view_node,
    ViewParams*             view_params,
    RendParams&             rend_params,
    HWND                    hwnd,
    DefaultLight*           default_lights,
    int                     default_light_count,
    RendProgressCallback*   progress_cb)
{
    BroadcastNotification(NOTIFY_PRE_RENDER, &rend_params);

    SuspendAll suspend(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

    m_scene = scene;
    m_view_node = view_node;
    if (view_params)
        m_view_params = *view_params;
    m_rend_params = rend_params;

    // Copy the default lights as the 'default_lights' pointer is no longer valid in Render().
    m_default_lights.clear();
    m_default_lights.reserve(default_light_count);
    for (int i = 0; i < default_light_count; ++i)
        m_default_lights.push_back(default_lights[i]);

    return 1;   // success
}

namespace
{
    void get_view_params_from_view_node(
        ViewParams&             view_params,
        INode*                  view_node,
        const TimeValue         time)
    {
        const ObjectState& os = view_node->EvalWorldState(time);
        switch (os.obj->SuperClassID())
        {
          case CAMERA_CLASS_ID:
            {
                CameraObject* cam = static_cast<CameraObject*>(os.obj);

                Interval validity_interval;
                validity_interval.SetInfinite();

                Matrix3 cam_to_world = view_node->GetObjTMAfterWSM(time, &validity_interval);
                cam_to_world.NoScale();

                view_params.affineTM = Inverse(cam_to_world);

                CameraState cam_state;
                cam->EvalCameraState(time, validity_interval, &cam_state);

                view_params.projType = PROJ_PERSPECTIVE;
                view_params.fov = cam_state.fov;

                if (cam_state.manualClip)
                {
                    view_params.hither = cam_state.hither;
                    view_params.yon = cam_state.yon;
                }
                else
                {
                    view_params.hither = 0.001f;
                    view_params.yon = 1.0e38f;
                }
            }
            break;

          case LIGHT_CLASS_ID:
            {
                DbgAssert(!"Not implemented yet.");
            }
            break;

          default:
            DbgAssert(!"Unexpected super class ID for camera.");
        }
    }

    class RenderBeginProc
      : public RefEnumProc
    {
      public:
        explicit RenderBeginProc(const TimeValue time)
          : m_time(time)
        {
        }

        int proc(ReferenceMaker* rm) override
        {
            rm->RenderBegin(m_time);
            return REF_ENUM_CONTINUE;
        }

      private:
        const TimeValue m_time;
    };

    class RenderEndProc
      : public RefEnumProc
    {
      public:
        explicit RenderEndProc(const TimeValue time)
          : m_time(time)
        {
        }

        int proc(ReferenceMaker* rm) override
        {
            rm->RenderEnd(m_time);
            return REF_ENUM_CONTINUE;
        }

      private:
        const TimeValue m_time;
    };

    void render_begin(
        std::vector<INode*>&    nodes,
        const TimeValue         time)
    {
        RenderBeginProc proc(time);
        proc.BeginEnumeration();

        for (auto node : nodes)
            node->EnumRefHierarchy(proc);

        proc.EndEnumeration();
    }

    void render_end(
        std::vector<INode*>&    nodes,
        const TimeValue         time)
    {
        RenderEndProc proc(time);
        proc.BeginEnumeration();

        for (auto node : nodes)
            node->EnumRefHierarchy(proc);

        proc.EndEnumeration();
    }

    asr::IRendererController::Status render(
        asr::Project&           project,
        const RendererSettings& settings,
        Bitmap*                 bitmap,
        RendProgressCallback*   progress_cb)
    {
        // Number of rendered tiles, shared counter accessed atomically.
        volatile asf::uint32 rendered_tile_count = 0;

        // Create the renderer controller.
        const size_t total_tile_count =
              static_cast<size_t>(settings.m_passes)
            * project.get_frame()->image().properties().m_tile_count;
        RendererController renderer_controller(
            progress_cb,
            &rendered_tile_count,
            total_tile_count);

        // Create the tile callback.
        TileCallback tile_callback(bitmap, &rendered_tile_count);

        // Create the master renderer.
        std::auto_ptr<asr::MasterRenderer> renderer(
            new asr::MasterRenderer(
                project,
                project.configurations().get_by_name("final")->get_inherited_parameters(),
                &renderer_controller,
                &tile_callback));

        // Render the frame.
        renderer->render();

        return renderer_controller.get_status();

        // Make sure the master renderer is deleted before the project.
    }
}

int AppleseedRenderer::Render(
    TimeValue               time,
    Bitmap*                 bitmap,
    FrameRendParams&        frame_rend_params,
    HWND                    hwnd,
    RendProgressCallback*   progress_cb,
    ViewParams*             view_params)
{
    SuspendAll suspend(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

    std::string previous_locale(std::setlocale(LC_ALL, "C"));

    m_time = time;

    if (view_params)
        m_view_params = *view_params;

    if (m_view_node)
        get_view_params_from_view_node(m_view_params, m_view_node, time);

    // Retrieve and tweak renderer settings.
    RendererSettings renderer_settings = m_settings;
    if (m_rend_params.inMtlEdit)
    {
        renderer_settings.m_pixel_samples = m_rend_params.mtlEditAA ? 32 : 4;
        renderer_settings.m_passes = 1;
        renderer_settings.m_gi = true;
        renderer_settings.m_background_emits_light = false;
    }

    if (!m_rend_params.inMtlEdit || m_settings.m_log_material_editor_messages)
        create_log_window();

    TimeValue eval_time = time;
    BroadcastNotification(NOTIFY_RENDER_PREEVAL, &eval_time);

    // Collect the entities we're interested in.
    if (progress_cb)
        progress_cb->SetTitle(L"Collecting Entities...");
    m_entities.clear();
    MaxSceneEntityCollector collector(m_entities);
    collector.collect(m_scene);

    // Call RenderBegin() on all object instances.
    render_begin(m_entities.m_objects, m_time);

    // Build the project.
    if (progress_cb)
        progress_cb->SetTitle(L"Building Project...");
    asf::auto_release_ptr<asr::Project> project(
        build_project(
            m_entities,
            m_default_lights,
            m_view_node,
            m_view_params,
            m_rend_params,
            frame_rend_params,
            renderer_settings,
            bitmap,
            time,
            progress_cb));

    if (m_rend_params.inMtlEdit)
    {
        // Write the project to disk, useful to debug material previews.
        // asr::ProjectFileWriter::write(project.ref(), "appleseed-max-material-editor.appleseed");

        // Render the project.
        if (progress_cb)
            progress_cb->SetTitle(L"Rendering...");
        render(project.ref(), m_settings, bitmap, progress_cb);
    }
    else
    {
        // Write the project to disk.
        if (!m_settings.m_use_max_procedural_maps)
        {
            if (m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectOnly ||
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectAndRender)
            {
                if (progress_cb)
                    progress_cb->SetTitle(L"Writing Project To Disk...");
                asr::ProjectFileWriter::write(
                    project.ref(),
                    wide_to_utf8(m_settings.m_project_file_path).c_str());
            }
        }

        // Render the project.
        if (m_settings.m_output_mode == RendererSettings::OutputMode::RenderOnly ||
            m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectAndRender)
        {

            RenderGlobalContext render_context;
            render_context.time = eval_time;
            render_context.inMtlEdit = m_rend_params.inMtlEdit;
            render_context.SetRenderElementMgr(m_rend_params.GetRenderElementMgr());
            BroadcastNotification(NOTIFY_PRE_RENDERFRAME, &render_context);

            auto render_status = asr::IRendererController::Status::ContinueRendering;

            if (progress_cb)
                progress_cb->SetTitle(L"Rendering...");
            if (m_settings.m_low_priority_mode)
            {
                asf::ProcessPriorityContext background_context(
                    asf::ProcessPriority::ProcessPriorityLow,
                    &asr::global_logger());
                render_status = render(project.ref(), m_settings, bitmap, progress_cb);
            }
            else
            {
                render_status = render(project.ref(), m_settings, bitmap, progress_cb);
            }

            if (render_status != asr::IRendererController::Status::AbortRendering &&
                !GetCOREInterface14()->GetRendUseIterative())
                project->get_frame()->write_main_and_aov_images();

            BroadcastNotification(NOTIFY_POST_RENDERFRAME, &render_context);
        }
    }

    if (progress_cb)
        progress_cb->SetTitle(L"Done.");

    std::setlocale(LC_ALL, previous_locale.c_str());

    // Success.
    return 1;
}

void AppleseedRenderer::Close(
    HWND                    hwnd,
    RendProgressCallback*   progress_cb)
{
    // Call RenderEnd() on all object instances.
    render_end(m_entities.m_objects, m_time);

    clear();

    BroadcastNotification(NOTIFY_POST_RENDER);
}

RendParamDlg* AppleseedRenderer::CreateParamDialog(
    IRendParams*            rend_params,
    BOOL                    in_progress)
{
    return
        new AppleseedRendererParamDlg(
            rend_params,
            in_progress,
            m_settings,
            this);
}

void AppleseedRenderer::ResetParams()
{
    clear();
}

IOResult AppleseedRenderer::Save(ISave* isave)
{
    bool success = true;

    isave->BeginChunk(ChunkFileFormatVersion);
    success &= write(isave, FileFormatVersion);
    isave->EndChunk();

    isave->BeginChunk(ChunkSettings);
    success &= m_settings.save(isave);
    isave->EndChunk();

    return success ? IO_OK : IO_ERROR;
}

IOResult AppleseedRenderer::Load(ILoad* iload)
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

          case ChunkSettings:
            result = m_settings.load(iload);
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

void AppleseedRenderer::AddTabToDialog(
    ITabbedDialog*          dialog,
    ITabDialogPluginTab*    tab) 
{
    const int RenderRollupWidth = 222;  // the width of the render rollout in dialog units
    dialog->AddRollout(
        L"Renderer",
        nullptr,
        AppleseedRendererTabClassId,
        tab,
        -1,
        RenderRollupWidth,
        0,
        0,
        ITabbedDialog::kSystemPage);
}

int AppleseedRenderer::AcceptTab(
    ITabDialogPluginTab*    tab)
{
    const Class_ID RayTraceTabClassId(0x4fa95e9b, 0x9a26e66);

    if (tab->GetSuperClassID() == RADIOSITY_CLASS_ID)
        return 0;

    if (tab->GetClassID() == RayTraceTabClassId)
        return 0;

    return TAB_DIALOG_ADD_TAB;
}

void AppleseedRenderer::show_last_session_log()
{
    if (g_dialog_log_target.get() == nullptr)
        create_log_window();

    g_dialog_log_target->show_last_session_messages();
}

void AppleseedRenderer::create_log_window()
{
    g_dialog_log_target.reset(new DialogLogTarget(m_settings.m_log_open_mode));
}

void AppleseedRenderer::clear()
{
    m_scene = nullptr;
    m_view_node = nullptr;
    m_default_lights.clear();
    m_time = 0;
    m_entities.clear();
}


//
// AppleseedRendererClassDesc class implementation.
//

int AppleseedRendererClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedRendererClassDesc::Create(BOOL loading)
{
    return new AppleseedRenderer();
}

const MCHAR* AppleseedRendererClassDesc::ClassName()
{
    return AppleseedRendererClassName;
}

SClass_ID AppleseedRendererClassDesc::SuperClassID()
{
    return RENDERER_CLASS_ID;
}

Class_ID AppleseedRendererClassDesc::ClassID()
{
    return AppleseedRendererClassId;
}

const MCHAR* AppleseedRendererClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedRendererClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedRenderer";
}

const MCHAR* AppleseedRendererClassDesc::GetRsrcString(INT_PTR id)
{
    return GetString(static_cast<int>(id));
}
