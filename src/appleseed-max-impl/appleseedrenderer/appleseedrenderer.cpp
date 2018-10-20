
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
#include "foundation/utility/kvpair.h"

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
        ParamMapIdPathTracer,
        ParamMapIdSPPM,
        ParamMapIdPostProcessing,
        ParamMapIdSystem
    };

    enum ParamId
    {
        ParamIdOuputMode                                = 0,
        ParamIdProjectPath                              = 1,
        ParamIdScaleMultiplier                          = 2,
        ParamIdShaderOverrideType                       = 54,
        ParamIdMaterialPreviewQuality                   = 55,

        ParamIdUniformPixelSamples                      = 3,
        ParamIdTileSize                                 = 4,
        ParamIdPasses                                   = 5,
        ParamIdFilterSize                               = 6,
        ParamIdFilterType                               = 7,
        ParamIdImageSamplerType                         = 40,
        ParamIdAdaptiveTileBatchSize                    = 41,
        ParamIdAdaptiveTileMinSamples                   = 42,
        ParamIdAdaptiveTileMaxSamples                   = 43,
        ParamIdAdaptiveTileNoiseThreshold               = 44,
        ParamIdBackgroundAlphaValue                     = 15,

        ParamIdLightingAlgorithm                        = 52,
        ParamIdForceDefaultLightsOff                    = 13,

        ParamIdEnableGI                                 = 8,
        ParamIdEnableCaustics                           = 10,
        ParamIdEnableMaxRayIntensity                    = 11,
        ParamIdMaxRayIntensity                          = 12,
        ParamIdEnableBackgroundLight                    = 14,
        ParamIdEnableRoughnessClamping                  = 23,
        ParamIdEnableDiffuseBounceLimit                 = 25,
        ParamIdEnableGlossyBounceLimit                  = 27,
        ParamIdEnableSpecularBounceLimit                = 29,
        ParamIdEnableVolumeBounceLimit                  = 31,
        ParamIdGlobalBounceLimit                        = 9,
        ParamIdDiffuseBounceLimit                       = 26,
        ParamIdGlossyBounceLimit                        = 28,
        ParamIdSpecularBounceLimit                      = 30,
        ParamIdVolumeBounceLimit                        = 32,
        ParamIdDirectLightSamples                       = 33,
        ParamIdLowLightThreshold                        = 34,
        ParamIdEnvLightSamples                          = 35,
        ParamIdRussianRouletteMinPathLength             = 36,
        ParamIdEnableDirectLighting                     = 37,
        ParamIdVolumeDistanceSamples                    = 38,
        ParamIdOptLightOutsideVolumes                   = 39, 

        ParamIdSPPMPhotonType                           = 57,
        ParamIdSPPMDirectLightingType                   = 58,
        ParamIdSPPMEnableCaustics                       = 59,
        ParamIdSPPMEnableImageBasedLighting             = 60,
        ParamIdSPPMPhotonTracingMaxBounces              = 61,
        ParamIdSPPMPhotonTracingEnableBounceLimit       = 62,
        ParamIdSPPMPhotonTracingRRMinPathLength         = 63,
        ParamIdSPPMPhotonTracingLightPhotons            = 64,
        ParamIdSPPMPhotonTracingEnvironmentPhotons      = 65,
        ParamIdSPPMRadianceEstimationMaxBounces         = 66,
        ParamIdSPPMRadianceEstimationEnableBounceLimit = 67,
        ParamIdSPPMRadianceEstimationRRMinPathLength    = 68,
        ParamIdSPPMRadianceEstimationInitialRadius      = 69,
        ParamIdSPPMRadianceEstimationMaxPhotons         = 70,
        ParamIdSPPMRadianceEstimationAlpha              = 71,

        ParamIdEnableRenderStamp                        = 21,
        ParamIdRenderStampFormat                        = 22,
        ParamIdDenoiseMode                              = 45,
        ParamIdEnableSkipDenoisedPixel                  = 46,
        ParamIdEnableRandomPixelOrder                   = 47,
        ParamIdEnablePrefilterSpikes                    = 48,
        ParamIdSpikeThreshold                           = 49,
        ParamIdPatchDistance                            = 50,
        ParamIdDenoiseScales                            = 51,

        ParamIdCPUCores                                 = 16,
        ParamIdOpenLogMode                              = 17,
        ParamIdLogMaterialRendering                     = 18,
        ParamIdUseMaxProcedurals                        = 19,
        ParamIdEnableLowPriority                        = 20,
        ParamIdEnableEmbree                             = 24,
        ParamIdTextureCacheSize                         = 53,
    };
    
    const asf::KeyValuePair<int, const wchar_t*> g_dialog_strings[] =
    {
        { IDS_RENDERERPARAMS_FILTER_TYPE_1,         L"Blackman-Harris" },
        { IDS_RENDERERPARAMS_FILTER_TYPE_2,         L"Box" },
        { IDS_RENDERERPARAMS_FILTER_TYPE_3,         L"Catmull-Rom Spline" },
        { IDS_RENDERERPARAMS_FILTER_TYPE_4,         L"Cubic B-spline" },
        { IDS_RENDERERPARAMS_FILTER_TYPE_5,         L"Gaussian" },
        { IDS_RENDERERPARAMS_FILTER_TYPE_6,         L"Lanczos" },
        { IDS_RENDERERPARAMS_FILTER_TYPE_7,         L"Mitchell-Netravali" },
        { IDS_RENDERERPARAMS_FILTER_TYPE_8,         L"Triangle" },
        { IDS_RENDERERPARAMS_LOG_OPEN_MODE_1,       L"Always" },
        { IDS_RENDERERPARAMS_LOG_OPEN_MODE_2,       L"Never" },
        { IDS_RENDERERPARAMS_LOG_OPEN_MODE_3,       L"On Error" },
        { IDS_RENDERERPARAMS_SAMPLER_TYPE_1,        L"Uniform Sampler" },
        { IDS_RENDERERPARAMS_SAMPLER_TYPE_2,        L"Adaptive Tile Sampler" },
        { IDS_RENDERERPARAMS_DENOISE_MODE_1 ,       L"Off" },
        { IDS_RENDERERPARAMS_DENOISE_MODE_2,        L"On" },
        { IDS_RENDERERPARAMS_DENOISE_MODE_3,        L"Write Outputs" },
        { IDS_RENDERERPARAMS_LIGHTING_ALGORITHM_1,  L"Path Tracing" },
        { IDS_RENDERERPARAMS_LIGHTING_ALGORITHM_2,  L"Stochastic Progressive Photon Mapping" },
        { IDS_RENDERERPARAMS_LIGHTING_ALGORITHM_3,  L"Bidirectional Path Tracing" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_1,     L"No Override" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_2,     L"Albedo" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_3,     L"Ambient Occlusion" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_4,     L"Assembly Instances" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_5,     L"Barycentric Coordinates" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_6,     L"Bitangents" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_7,     L"Coverage" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_8,     L"Depth" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_9,     L"Facing Ratio" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_10,    L"Geometric Normals" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_11,    L"Materials" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_12,    L"Object Instances" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_13,    L"Original Shading Normals" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_14,    L"Primitives" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_15,    L"Ray Spread" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_16,    L"Regions" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_17,    L"Screen Space Wireframe" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_18,    L"Shading Normals" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_19,    L"Sides" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_20,    L"Tangents" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_21,    L"UV Coordinates" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_22,    L"World Space Position" },
        { IDS_RENDERERPARAMS_SHADER_OVERRIDE_23,    L"World Space Wireframe" },
        { IDS_RENDERERPARAMS_SPPM_PHOTON_TYPE_1,    L"Monochromatic" },
        { IDS_RENDERERPARAMS_SPPM_PHOTON_TYPE_2,    L"Polychromatic" },
        { IDS_RENDERERPARAMS_SPPM_DL_TYPE_1,        L"Ray Traced" },
        { IDS_RENDERERPARAMS_SPPM_DL_TYPE_2,        L"SPPM" },
        { IDS_RENDERERPARAMS_SPPM_DL_TYPE_3,        L"Off" }
    };

    class AppleseedRenderContext
      : public RenderGlobalContext
    {
      public:
        AppleseedRenderContext(
            Renderer*           renderer,
            Bitmap*             bitmap,
            const RendParams&   rend_params,
            const ViewParams&   view_params,
            const TimeValue     time)
        {
            this->time = time;
            this->inMtlEdit = rend_params.inMtlEdit;
            IRenderElementMgr* current_re_manager = GetCOREInterface()->GetCurRenderElementMgr();
            if (current_re_manager != nullptr)
                this->SetRenderElementMgr(current_re_manager);

            this->renderer = renderer;
            this->projType = view_params.projType;

            if (bitmap != nullptr)
            {
                this->devWidth = bitmap->Width();
                this->devHeight = bitmap->Height();
                this->devAspect = bitmap->Aspect();

                this->xc = this->devWidth * 0.5f;
                this->yc = this->devHeight * 0.5f;

                if (this->projType == PROJ_PERSPECTIVE)
                {
                    const float fac = -(1.0f / tanf(0.5f * view_params.fov));
                    this->xscale = fac * this->xc;
                }
                else
                {
                    this->xscale = this->devWidth / view_params.fov;
                }

                this->yscale = -this->devAspect * this->xscale;
            }

            this->antialias = false;
            this->nearRange = 0.0f;
            this->farRange = 0.0f;
            this->envMap = nullptr;
            this->globalLightLevel = Color(0.0f, 0.0f, 0.0f);
            this->atmos = nullptr;
            this->wireMode = false;
            this->wire_thick = 0;
            this->first_field = true;
        }
    };
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

      case ParamIdScaleMultiplier:
        v.f = settings.m_scale_multiplier;
        break;

      case ParamIdShaderOverrideType:
        v.i = settings.m_shader_override;
        break;

      case ParamIdMaterialPreviewQuality:
        v.i = settings.m_material_preview_quality;
        break;
        
      //
      // Image Sampling.
      //

      case ParamIdUniformPixelSamples:
        v.i = settings.m_uniform_pixel_samples;
        break;

      case ParamIdTileSize:
        v.i = settings.m_tile_size;
        break;

      case ParamIdPasses:
        v.i = settings.m_passes;
        break;

      case ParamIdImageSamplerType:
        v.i = settings.m_sampler_type;
        break;

      //
      // Adaptive Tile Renderer.
      //

      case ParamIdAdaptiveTileBatchSize:
        v.i = settings.m_adaptive_batch_size;
        break;

      case ParamIdAdaptiveTileMinSamples:
        v.i = settings.m_adaptive_min_samples;
        break;

      case ParamIdAdaptiveTileMaxSamples:
        v.i = settings.m_adaptive_max_samples;
        break;

      case ParamIdAdaptiveTileNoiseThreshold:
        v.f = settings.m_adaptive_noise_threshold;
        break;

      //
      // Pixel Filtering and Background Alpha.
      //

      case ParamIdFilterType:
        v.i = settings.m_pixel_filter;
        break;

      case ParamIdFilterSize:
        v.f = settings.m_pixel_filter_size;
        break;

      case ParamIdBackgroundAlphaValue:
        v.f = settings.m_background_alpha;
        break;

      //
      // Lighting.
      //

      case ParamIdLightingAlgorithm:
        v.i = settings.m_lighting_algorithm;
        break;

      case ParamIdForceDefaultLightsOff:
        v.i = static_cast<int>(settings.m_force_off_default_lights);
        break;

      //
      // Path Tracer.
      //

      case ParamIdEnableGI:
        v.i = static_cast<int>(settings.m_enable_gi);
        break;
            
      case ParamIdEnableDirectLighting:
        v.i = static_cast<int>(settings.m_dl_enable_dl);
        break;
     
      case ParamIdEnableCaustics:
        v.i = static_cast<int>(settings.m_enable_caustics);
        break;
            
      case ParamIdEnableMaxRayIntensity:
        v.i = static_cast<int>(settings.m_max_ray_intensity_set);
        break;

      case ParamIdEnableBackgroundLight:
        v.i = static_cast<int>(settings.m_background_emits_light);
        break;

      case ParamIdEnableRoughnessClamping:
        v.i = static_cast<int>(settings.m_clamp_roughness);
        break;

      case ParamIdEnableDiffuseBounceLimit:
        v.i = static_cast<int>(settings.m_diffuse_bounces_enabled);
        break;

      case ParamIdEnableGlossyBounceLimit:
        v.i = static_cast<int>(settings.m_glossy_bounces_enabled);
        break;

      case ParamIdEnableSpecularBounceLimit:
        v.i = static_cast<int>(settings.m_specular_bounces_enabled);
        break;

      case ParamIdEnableVolumeBounceLimit:
        v.i = static_cast<int>(settings.m_volume_bounces_enabled);
        break;

      case ParamIdOptLightOutsideVolumes:
        v.i = static_cast<int>(settings.m_optimize_for_lights_outside_volumes);
        break;

      case ParamIdMaxRayIntensity:
        v.f = settings.m_max_ray_intensity;
        break;

      case ParamIdGlobalBounceLimit:
        v.i = settings.m_global_bounces;
        break;

      case ParamIdDiffuseBounceLimit:
        v.i = settings.m_diffuse_bounces;
        break;

      case ParamIdGlossyBounceLimit:
        v.i = settings.m_glossy_bounces;
        break;

      case ParamIdSpecularBounceLimit:
        v.i = settings.m_specular_bounces;
        break;

      case ParamIdVolumeBounceLimit:
        v.i = settings.m_volume_bounces;
        break;

      case ParamIdVolumeDistanceSamples:
        v.i = settings.m_volume_distance_samples;
        break;

      case ParamIdDirectLightSamples:
        v.i = settings.m_dl_light_samples;
        break;

      case ParamIdLowLightThreshold:
        v.f = settings.m_dl_low_light_threshold;
        break;

      case ParamIdEnvLightSamples:
        v.i = settings.m_ibl_env_samples;
        break;

      case ParamIdRussianRouletteMinPathLength:
        v.i = settings.m_rr_min_path_length;
        break;

      //
      // Stochastic Progressive Photon Mapping.
      //

      case ParamIdSPPMPhotonType:
        v.i = settings.m_sppm_photon_type;
        break;

      case ParamIdSPPMDirectLightingType:
        v.i = settings.m_sppm_direct_lighting_mode;
        break;

      case ParamIdSPPMEnableCaustics:
        v.i = static_cast<int>(settings.m_sppm_enable_caustics);
        break;

      case ParamIdSPPMEnableImageBasedLighting:
        v.i = static_cast<int>(settings.m_sppm_enable_image_based_lighting);
        break;

      case ParamIdSPPMPhotonTracingMaxBounces:
        v.i = settings.m_sppm_photon_tracing_max_bounces;
        break;

      case ParamIdSPPMPhotonTracingEnableBounceLimit:
        v.i = static_cast<int>(settings.m_sppm_photon_tracing_enable_bounce_limit);
        break;

      case ParamIdSPPMPhotonTracingRRMinPathLength:
        v.i = settings.m_sppm_photon_tracing_rr_min_path_length;
        break;

      case ParamIdSPPMPhotonTracingLightPhotons:
        v.i = settings.m_sppm_photon_tracing_light_photons;
        break;

      case ParamIdSPPMPhotonTracingEnvironmentPhotons:
        v.i = settings.m_sppm_photon_tracing_environment_photons;
        break;

      case ParamIdSPPMRadianceEstimationMaxBounces:
        v.i = settings.m_sppm_radiance_estimation_max_bounces;
        break;

      case ParamIdSPPMRadianceEstimationEnableBounceLimit:
        v.i = static_cast<int>(settings.m_sppm_radiance_estimation_enable_bounce_limit);
        break;

      case ParamIdSPPMRadianceEstimationRRMinPathLength:
        v.i = settings.m_sppm_radiance_estimation_rr_min_path_length;
        break;

      case ParamIdSPPMRadianceEstimationInitialRadius:
        v.f = settings.m_sppm_radiance_estimation_initial_radius;
        break;

      case ParamIdSPPMRadianceEstimationMaxPhotons:
        v.i = settings.m_sppm_radiance_estimation_max_photons;
        break;

      case ParamIdSPPMRadianceEstimationAlpha:
        v.f = settings.m_sppm_radiance_estimation_alpha;
        break;

      //
      // Post-Processing.
      //

      case ParamIdEnableRenderStamp:
        v.i = static_cast<int>(settings.m_enable_render_stamp);
        break;

      case ParamIdDenoiseMode:
        v.i = settings.m_denoise_mode;
        break;

      case ParamIdEnableSkipDenoisedPixel:
        v.i = static_cast<int>(settings.m_enable_skip_denoised);
        break;

      case ParamIdEnableRandomPixelOrder:
        v.i = static_cast<int>(settings.m_enable_random_pixel_order);
        break;

      case ParamIdEnablePrefilterSpikes:
        v.i = static_cast<int>(settings.m_enable_prefilter_spikes);
        break;

      case ParamIdSpikeThreshold:
        v.f = settings.m_spike_threshold;
        break;

      case ParamIdPatchDistance:
        v.f = settings.m_patch_distance_threshold;
        break;

      case ParamIdDenoiseScales:
        v.i = settings.m_denoise_scales;
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

      case ParamIdLogMaterialRendering:
        v.i = static_cast<int>(settings.m_log_material_editor_messages);
        break;

      case ParamIdOpenLogMode:
        v.i = static_cast<int>(settings.m_log_open_mode);
        break;

      case ParamIdEnableEmbree:
        v.i = static_cast<int>(settings.m_enable_embree);
        break;

      case ParamIdTextureCacheSize:
        v.i = static_cast<int>(settings.m_texture_cache_size);
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

      case ParamIdShaderOverrideType:
        settings.m_shader_override = v.i;
        break;

      case ParamIdMaterialPreviewQuality:
        settings.m_material_preview_quality = v.i;
        break;
        
     //
     // Image Sampling.
     //

      case ParamIdUniformPixelSamples:
        settings.m_uniform_pixel_samples = v.i;
        break;

      case ParamIdTileSize:
        settings.m_tile_size = v.i;
        break;

      case ParamIdPasses:
        settings.m_passes = v.i;
        break;

      case ParamIdImageSamplerType:
        settings.m_sampler_type = v.i;
        break;

     //
     // Adaptive Tile Renderer.
     //

      case ParamIdAdaptiveTileBatchSize:
        settings.m_adaptive_batch_size = v.i;
        break;

      case ParamIdAdaptiveTileMinSamples:
        settings.m_adaptive_min_samples = v.i;
        break;

      case ParamIdAdaptiveTileMaxSamples:
        settings.m_adaptive_max_samples = v.i;
        break;

      case ParamIdAdaptiveTileNoiseThreshold:
        settings.m_adaptive_noise_threshold = v.f;
        break;

    //
    // Pixel Filtering and Background Alpha.
    //

      case ParamIdFilterType:
        settings.m_pixel_filter = v.i;
        break;

      case ParamIdFilterSize:
        settings.m_pixel_filter_size = v.f;
        break;

      case ParamIdBackgroundAlphaValue:
        settings.m_background_alpha = v.f;
        break;

    //
    // Lighting.
    //

      case ParamIdLightingAlgorithm:
        settings.m_lighting_algorithm = v.i;
        break;

      case ParamIdForceDefaultLightsOff:
        settings.m_force_off_default_lights = v.i > 0;
        break;

    //
    // Pathtracer.
    //

      case ParamIdEnableGI:
        settings.m_enable_gi = v.i > 0;
        break;
            
      case ParamIdEnableDirectLighting:
        settings.m_dl_enable_dl = v.i > 0;
        break;

      case ParamIdEnableCaustics:
        settings.m_enable_caustics = v.i > 0;
        break;
            
      case ParamIdEnableMaxRayIntensity:
        settings.m_max_ray_intensity_set = v.i > 0;
        break;

      case ParamIdEnableBackgroundLight:
        settings.m_background_emits_light = v.i > 0;
        break;

      case ParamIdEnableDiffuseBounceLimit:
        settings.m_diffuse_bounces_enabled = v.i > 0;
        break;

      case ParamIdEnableGlossyBounceLimit:
        settings.m_glossy_bounces_enabled = v.i > 0;
        break;

      case ParamIdEnableSpecularBounceLimit:
        settings.m_specular_bounces_enabled = v.i > 0;
        break;

      case ParamIdEnableVolumeBounceLimit:
        settings.m_volume_bounces_enabled = v.i > 0;
        break;

      case ParamIdGlobalBounceLimit:
        settings.m_global_bounces = v.i;
        break;

      case ParamIdEnableRoughnessClamping:
        settings.m_clamp_roughness = v.i > 0;
        break;

      case ParamIdMaxRayIntensity:
        settings.m_max_ray_intensity = v.f;
        break;

      case ParamIdDiffuseBounceLimit:
        settings.m_diffuse_bounces = v.i;
        break;

      case ParamIdGlossyBounceLimit:
        settings.m_glossy_bounces = v.i;
        break;

      case ParamIdSpecularBounceLimit:
        settings.m_specular_bounces = v.i;
        break;

      case ParamIdVolumeBounceLimit:
        settings.m_volume_bounces = v.i;
        break;

      case ParamIdVolumeDistanceSamples:
        settings.m_volume_distance_samples = v.i;
        break;

      case ParamIdOptLightOutsideVolumes:
        settings.m_optimize_for_lights_outside_volumes = v.i > 0;
        break;

      case ParamIdDirectLightSamples:
        settings.m_dl_light_samples = v.i;
        break;

      case ParamIdLowLightThreshold:
        settings.m_dl_low_light_threshold = v.f;
        break;

      case ParamIdEnvLightSamples:
        settings.m_ibl_env_samples = v.i;
        break;

      case ParamIdRussianRouletteMinPathLength:
        settings.m_rr_min_path_length = v.i;
        break;

      //
      // Stochastic Progressive Photon Mapping.
      //

      case ParamIdSPPMPhotonType:
        settings.m_sppm_photon_type = v.i;
        break;

      case ParamIdSPPMDirectLightingType:
        settings.m_sppm_direct_lighting_mode = v.i;
        break;

      case ParamIdSPPMEnableCaustics:
        settings.m_sppm_enable_caustics = v.i > 0;
        break;

      case ParamIdSPPMEnableImageBasedLighting:
        settings.m_sppm_enable_image_based_lighting = v.i > 0;
        break;

      case ParamIdSPPMPhotonTracingMaxBounces:
        settings.m_sppm_photon_tracing_max_bounces = v.i;
        break;

      case ParamIdSPPMPhotonTracingEnableBounceLimit:
        settings.m_sppm_photon_tracing_enable_bounce_limit = v.i > 0;
        break;

      case ParamIdSPPMPhotonTracingRRMinPathLength:
        settings.m_sppm_photon_tracing_rr_min_path_length = v.i;
        break;

      case ParamIdSPPMPhotonTracingLightPhotons:
        settings.m_sppm_photon_tracing_light_photons = v.i;
        break;

      case ParamIdSPPMPhotonTracingEnvironmentPhotons:
        settings.m_sppm_photon_tracing_environment_photons = v.i;
        break;

      case ParamIdSPPMRadianceEstimationMaxBounces:
        settings.m_sppm_radiance_estimation_max_bounces = v.i;
        break;

      case ParamIdSPPMRadianceEstimationEnableBounceLimit:
        settings.m_sppm_radiance_estimation_enable_bounce_limit = v.i > 0;
        break;

      case ParamIdSPPMRadianceEstimationRRMinPathLength:
        settings.m_sppm_radiance_estimation_rr_min_path_length = v.i;
        break;

      case ParamIdSPPMRadianceEstimationInitialRadius:
        settings.m_sppm_radiance_estimation_initial_radius = v.f;
        break;

      case ParamIdSPPMRadianceEstimationMaxPhotons:
        settings.m_sppm_radiance_estimation_max_photons = v.i;
        break;

      case ParamIdSPPMRadianceEstimationAlpha:
        settings.m_sppm_radiance_estimation_alpha = v.f;
        break;

    //
    // Postprocessing.
    //

      case ParamIdEnableRenderStamp:
        settings.m_enable_render_stamp = v.i > 0;
        break;

      case ParamIdRenderStampFormat:
        settings.m_render_stamp_format = v.s;
        break;

      case ParamIdDenoiseMode:
        settings.m_denoise_mode = v.i;
        break;

      case ParamIdEnableSkipDenoisedPixel:
        settings.m_enable_skip_denoised = v.i > 0;
        break;

      case ParamIdEnableRandomPixelOrder:
        settings.m_enable_random_pixel_order = v.i > 0;
        break;

      case ParamIdEnablePrefilterSpikes:
        settings.m_enable_prefilter_spikes= v.i > 0;
        break;

      case ParamIdSpikeThreshold:
        settings.m_spike_threshold = v.f;
        break;

      case ParamIdPatchDistance:
        settings.m_patch_distance_threshold = v.f;
        break;

      case ParamIdDenoiseScales:
        settings.m_denoise_scales = v.i;
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
            
      case ParamIdLogMaterialRendering:
        settings.m_log_material_editor_messages = v.i > 0;
        break;

      case ParamIdOpenLogMode:
        settings.m_log_open_mode = static_cast<DialogLogTarget::OpenMode>(v.i);
        break;

      case ParamIdEnableEmbree:
        settings.m_enable_embree = v.i > 0;
        break;

      case ParamIdTextureCacheSize:
        settings.m_texture_cache_size = v.i;
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
    P_AUTO_CONSTRUCT + 
    P_AUTO_UI + 
    P_MULTIMAP + 
    P_VERSION + 
    P_CALLSETS_ON_LOAD,                         // block flags

    1,                                          // --- P_VERSION arguments ---

                                                // --- P_AUTO_CONSTRUCT arguments ---
    0,                                          // parameter block's reference number

                                                // --- P_MULTIMAP arguments ---
    7,

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

    ParamMapIdPathTracer,
    IDD_FORMVIEW_RENDERERPARAMS_PATH_TRACING,   // ID of the dialog template
    0,                                          // ID of the dialog's title string
    0,                                          // IParamMap2 creation/deletion flag mask
    0,                                          // rollup creation flag
    nullptr,

    ParamMapIdSPPM,
    IDD_FORMVIEW_RENDERERPARAMS_SPPM,           // ID of the dialog template
    0,                                          // ID of the dialog's title string
    0,                                          // IParamMap2 creation/deletion flag mask
    0,                                          // rollup creation flag
    nullptr,

    ParamMapIdPostProcessing,
    IDD_FORMVIEW_RENDERERPARAMS_POSTPROCESSING, // ID of the dialog template
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

    ParamIdOuputMode, L"output_mode", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdOutput, TYPE_RADIO, 3, IDC_RADIO_RENDER, IDC_RADIO_SAVEPROJECT, IDC_RADIO_SAVEPROJECT_AND_RENDER,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdProjectPath, L"project_path", TYPE_STRING, P_TRANSIENT, 0,
        p_ui, ParamMapIdOutput, TYPE_EDITBOX, IDC_TEXT_PROJECT_FILEPATH,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdScaleMultiplier, L"scale_multiplier", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdOutput, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_SCALE_MULTIPLIER, IDC_SPINNER_SCALE_MULTIPLIER, SPIN_AUTOSCALE,
        p_default, 1.0f,
        p_range, 1.0e-6f, 1.0e6f,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdShaderOverrideType, L"shader_override_type", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdOutput, TYPE_INT_COMBOBOX, IDC_COMBO_DIAGNOSTIC_SHADER_MODE,
        23, IDS_RENDERERPARAMS_SHADER_OVERRIDE_1, IDS_RENDERERPARAMS_SHADER_OVERRIDE_2, IDS_RENDERERPARAMS_SHADER_OVERRIDE_3,
        IDS_RENDERERPARAMS_SHADER_OVERRIDE_4, IDS_RENDERERPARAMS_SHADER_OVERRIDE_5, IDS_RENDERERPARAMS_SHADER_OVERRIDE_6,
        IDS_RENDERERPARAMS_SHADER_OVERRIDE_7, IDS_RENDERERPARAMS_SHADER_OVERRIDE_8, IDS_RENDERERPARAMS_SHADER_OVERRIDE_9,
        IDS_RENDERERPARAMS_SHADER_OVERRIDE_10, IDS_RENDERERPARAMS_SHADER_OVERRIDE_11, IDS_RENDERERPARAMS_SHADER_OVERRIDE_12,
        IDS_RENDERERPARAMS_SHADER_OVERRIDE_13, IDS_RENDERERPARAMS_SHADER_OVERRIDE_14, IDS_RENDERERPARAMS_SHADER_OVERRIDE_15,
        IDS_RENDERERPARAMS_SHADER_OVERRIDE_16, IDS_RENDERERPARAMS_SHADER_OVERRIDE_17, IDS_RENDERERPARAMS_SHADER_OVERRIDE_18,
        IDS_RENDERERPARAMS_SHADER_OVERRIDE_19, IDS_RENDERERPARAMS_SHADER_OVERRIDE_20, IDS_RENDERERPARAMS_SHADER_OVERRIDE_21, 
        IDS_RENDERERPARAMS_SHADER_OVERRIDE_22, IDS_RENDERERPARAMS_SHADER_OVERRIDE_23,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdMaterialPreviewQuality, L"material_preview_samples", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdOutput, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_MATERIAL_PREVIEW_QUALITY, IDC_SPINNER_MATERIAL_PREVIEW_QUALITY, SPIN_AUTOSCALE,
        p_default, 4,
        p_range, 1, 64,
        p_accessor, &g_pblock_accessor,
    p_end,

    // --- Parameters specifications for Image Sampling rollup ---

    ParamIdUniformPixelSamples, L"pixel_samples", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_PIXEL_SAMPLES, IDC_SPINNER_PIXEL_SAMPLES, SPIN_AUTOSCALE,
        p_default, 16,
        p_range, 1, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdTileSize, L"tile_size", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_TILE_SIZE, IDC_SPINNER_TILE_SIZE, SPIN_AUTOSCALE,
        p_default, 64,
        p_range, 1, 4096,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdPasses, L"passes", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_PASSES, IDC_SPINNER_PASSES, SPIN_AUTOSCALE,
        p_default, 1,
        p_range, 1, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdFilterSize, L"filter_size", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_FILTER_SIZE, IDC_SPINNER_FILTER_SIZE, SPIN_AUTOSCALE,
        p_default, 1.5f,
        p_range, 0.5f, 20.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdFilterType, L"filter_type", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_INT_COMBOBOX, IDC_COMBO_FILTER,
        8, IDS_RENDERERPARAMS_FILTER_TYPE_1, IDS_RENDERERPARAMS_FILTER_TYPE_2,
        IDS_RENDERERPARAMS_FILTER_TYPE_3, IDS_RENDERERPARAMS_FILTER_TYPE_4,
        IDS_RENDERERPARAMS_FILTER_TYPE_5, IDS_RENDERERPARAMS_FILTER_TYPE_6,
        IDS_RENDERERPARAMS_FILTER_TYPE_7, IDS_RENDERERPARAMS_FILTER_TYPE_8,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdImageSamplerType, L"image_sampler_type", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_INT_COMBOBOX, IDC_COMBO_SAMPLER_TYPE,
        2, IDS_RENDERERPARAMS_SAMPLER_TYPE_1, IDS_RENDERERPARAMS_SAMPLER_TYPE_2,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdAdaptiveTileBatchSize, L"batch_size", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_ADAPTIVE_BATCH_SIZE, IDC_SPINNER_ADAPTIVE_BATCH_SIZE, SPIN_AUTOSCALE,
        p_default, 16,
        p_range, 1, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdAdaptiveTileMinSamples, L"minimum_samples", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_MIN_ADAPTIVE_SAMPLES, IDC_SPINNER_MIN_ADAPTIVE_SAMPLES, SPIN_AUTOSCALE,
        p_default, 0,
        p_range, 0, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdAdaptiveTileMaxSamples, L"maximum_samples", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_MAX_ADAPTIVE_SAMPLES, IDC_SPINNER_MAX_ADAPTIVE_SAMPLES, SPIN_AUTOSCALE,
        p_default, 256,
        p_range, 0, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdAdaptiveTileNoiseThreshold, L"noise_threshold", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_ADAPTIVE_NOISE_THRESHOLD, IDC_SPINNER_ADAPTIVE_NOISE_THRESHOLD, SPIN_AUTOSCALE,
        p_default, 1.0f,
        p_range, 0.0f, 25.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdBackgroundAlphaValue, L"background_alpha", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdImageSampling, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_BACKGROUND_ALPHA, IDC_SPINNER_BACKGROUND_ALPHA, SPIN_AUTOSCALE,
        p_default, 0.0f,
        p_range, 0.0f, 1.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    // --- Parameters specifications for Lighting rollup ---

    ParamIdLightingAlgorithm, L"lighting_algorithm", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdLighting, TYPE_INT_COMBOBOX, IDC_COMBO_LIGHTING_ALGORITHM,
        2, IDS_RENDERERPARAMS_LIGHTING_ALGORITHM_1, IDS_RENDERERPARAMS_LIGHTING_ALGORITHM_2,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdForceDefaultLightsOff, L"force_default_lights_off", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdLighting, TYPE_SINGLECHEKBOX, IDC_CHECK_FORCE_OFF_DEFAULT_LIGHT,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,

    // --- Parameters specifications for Path Tracer rollup ---

    ParamIdEnableGI, L"enable_global_illumination", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_GI,
        p_default, TRUE,
        p_enable_ctrls, 1, ParamIdGlobalBounceLimit,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdGlobalBounceLimit, L"global_illumination_bounces", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_BOUNCES, IDC_SPINNER_BOUNCES, SPIN_AUTOSCALE,
        p_default, 8,
        p_range, 0, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableDirectLighting, L"enable_direct_lighting", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_DL,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableCaustics, L"enable_caustics", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_CAUSTICS,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableRoughnessClamping, L"roughness_clamping", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_ROUGHNESS,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableMaxRayIntensity, L"enable_max_ray", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_MAX_RAY_INTENSITY,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdMaxRayIntensity,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdMaxRayIntensity, L"max_ray_value", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_MAX_RAY_INTENSITY, IDC_SPINNER_MAX_RAY_INTENSITY, SPIN_AUTOSCALE,
        p_default, 1.0f,
        p_range, 0.0f, 1000.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableBackgroundLight, L"enable_background_light", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_BACKGROUND_EMITS_LIGHT,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableDiffuseBounceLimit, L"enable_diffuse_bounce_limit", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_ENABLE_DBOUNCE,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdDiffuseBounceLimit,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdDiffuseBounceLimit, L"diffuse_bounce_limit", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_DBOUNCES, IDC_SPINNER_DBOUNCES, SPIN_AUTOSCALE,
        p_default, 3,
        p_range, 0, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableGlossyBounceLimit, L"enable_glossy_bounce_limit", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_ENABLE_GBOUNCE,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdGlossyBounceLimit,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdGlossyBounceLimit, L"glossy_bounce_limit", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_GBOUNCES, IDC_SPINNER_GBOUNCES, SPIN_AUTOSCALE,
        p_default, 8,
        p_range, 0, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableSpecularBounceLimit, L"enable_specular_bounce_limit", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_ENABLE_SBOUNCE,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdSpecularBounceLimit,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSpecularBounceLimit, L"specular_bounce_limit", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_SBOUNCES, IDC_SPINNER_SBOUNCES, SPIN_AUTOSCALE,
        p_default, 8,
        p_range, 0, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableVolumeBounceLimit, L"enable_volume_bounce_limit", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_ENABLE_VBOUNCE,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdVolumeBounceLimit,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdVolumeBounceLimit, L"volume_bounce_limit", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_VBOUNCES, IDC_SPINNER_VBOUNCES, SPIN_AUTOSCALE,
        p_default, 8,
        p_range, 0, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdVolumeDistanceSamples, L"volume_distance_samples", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_VOL_DISTANCE_SAMPLES, IDC_SPINNER_VOL_DISTANCE_SAMPLES, SPIN_AUTOSCALE,
        p_default, 2,
        p_range, 1, 1000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdOptLightOutsideVolumes, L"optimize_for_lights_outside_volumes", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SINGLECHEKBOX, IDC_CHECK_LIGHT_OUTSIDE_VOLUMES,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdDirectLightSamples, L"direct_light_samples", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_DL_SAMPLES, IDC_SPINNER_DL_SAMPLES, SPIN_AUTOSCALE,
        p_default, 1,
        p_range, 0, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdLowLightThreshold, L"low_light_threshold", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_DL_LOW_LIGHT_THRESHOLD, IDC_SPINNER_DL_LOW_LIGHT_THRESHOLD, SPIN_AUTOSCALE,
        p_default, 0.0f,
        p_range, 0.0f, 1000.0f,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnvLightSamples, L"environment_light_samples", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_ENV_SAMPLES, IDC_SPINNER_ENV_SAMPLES, SPIN_AUTOSCALE,
        p_default, 1,
        p_range, 0, 1000000,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdRussianRouletteMinPathLength, L"russian_roulette_min_path_length", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPathTracer, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_RR_MIN_PATH_LENGTH, IDC_SPINNER_RR_MIN_PATH_LENGTH, SPIN_AUTOSCALE,
        p_default, 6,
        p_range, 1, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

     // --- Parameters specifications for SPPM rollup ---

    ParamIdSPPMPhotonType, L"photon_type", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_INT_COMBOBOX, IDC_COMBO_SPPM_PHOTON_TYPE,
        2, IDS_RENDERERPARAMS_SPPM_PHOTON_TYPE_1, IDS_RENDERERPARAMS_SPPM_PHOTON_TYPE_2,
        p_default, 1,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSPPMDirectLightingType, L"direct_lighting_type", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_INT_COMBOBOX, IDC_COMBO_SPPM_DL_TYPE,
        3, IDS_RENDERERPARAMS_SPPM_DL_TYPE_1, IDS_RENDERERPARAMS_SPPM_DL_TYPE_2, IDS_RENDERERPARAMS_SPPM_DL_TYPE_3,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSPPMEnableCaustics, L"enable_caustics", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SINGLECHEKBOX, IDC_CHECK_SPPM_CAUSTICS,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSPPMEnableImageBasedLighting, L"enable_image_based_lighting", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SINGLECHEKBOX, IDC_CHECK_SPPM_IMAGE_BASED_LIGHTING,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSPPMPhotonTracingMaxBounces, L"photon_tracing_max_bounces", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_SPPM_PT_BOUNCES, IDC_SPINNER_SPPM_PT_BOUNCES, SPIN_AUTOSCALE,
        p_default, -1,
        p_range, -1, 100,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSPPMPhotonTracingEnableBounceLimit, L"enable_photon_trace_bounce_limit", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SINGLECHEKBOX, IDC_CHECK_SPPM_PT_BOUNCES,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdSPPMPhotonTracingMaxBounces,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSPPMPhotonTracingRRMinPathLength, L"russian_roulette_start_bounce", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_TEXT_SPPM_PT_RR_MIN_PATH_LENGTH, IDC_SPINNER_SPPM_PT_RR_MIN_PATH_LENGTH, SPIN_AUTOSCALE,
        p_default, 6,
        p_range, 1, 100,
        p_accessor, &g_pblock_accessor,
     p_end,

     ParamIdSPPMPhotonTracingLightPhotons, L"light_photons", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_TEXT_SPPM_PT_LIGHT_PHOTONS, IDC_SPINNER_SPPM_PT_LIGHT_PHOTONS, SPIN_AUTOSCALE,
        p_default, 1000000,
        p_range, 0, 1000000000,
        p_accessor, &g_pblock_accessor,
     p_end,

     ParamIdSPPMPhotonTracingEnvironmentPhotons, L"environment_photons", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_TEXT_SPPM_PT_ENVIRONMENT_PHOTONS, IDC_SPINNER_SPPM_PT_ENVIRONMENT_PHOTONS, SPIN_AUTOSCALE,
        p_default, 1000000,
        p_range, 0, 1000000000,
        p_accessor, &g_pblock_accessor,
     p_end,

     ParamIdSPPMRadianceEstimationMaxBounces, L"radiance_estimation_max_bounces", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_SPPM_RADIANCE_BOUNCES, IDC_SPINNER_SPPM_RADIANCE_BOUNCES, SPIN_AUTOSCALE,
        p_default, -1,
        p_range, -1, 100,
        p_accessor, &g_pblock_accessor,
     p_end,

     ParamIdSPPMRadianceEstimationEnableBounceLimit, L"enable_radiance_estimation_bounce_limit", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SINGLECHEKBOX, IDC_CHECK_SPPM_RADIANCE_BOUNCES,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdSPPMRadianceEstimationMaxBounces,
        p_accessor, &g_pblock_accessor,
     p_end,

     ParamIdSPPMRadianceEstimationRRMinPathLength, L"russian_roulette_start_bounce", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_TEXT_SPPM_RADIANCE_RR_MIN_PATH_LENGTH, IDC_SPINNER_SPPM_RADIANCE_RR_MIN_PATH_LENGTH, SPIN_AUTOSCALE,
        p_default, 6,
        p_range, 1, 100,
        p_accessor, &g_pblock_accessor,
      p_end,

     ParamIdSPPMRadianceEstimationInitialRadius, L"initial_radius", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_POS_FLOAT, IDC_TEXT_SPPM_RADIANCE_INITIAL_RADIUS, IDC_SPINNER_SPPM_RADIANCE_INITIAL_RADIUS, SPIN_AUTOSCALE,
        p_default, 0.1f,
        p_range, 0.001f, 100.0f,
        p_accessor, &g_pblock_accessor,
     p_end,

     ParamIdSPPMRadianceEstimationMaxPhotons, L"max_photons_per_estimate", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_TEXT_SPPM_RADIANCE_MAX_PHOTONS, IDC_SPINNER_SPPM_RADIANCE_MAX_PHOTONS, SPIN_AUTOSCALE,
        p_default, 100,
        p_range, 8, 1000000000,
        p_accessor, &g_pblock_accessor,
     p_end,

     ParamIdSPPMRadianceEstimationAlpha, L"alpha", TYPE_FLOAT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSPPM, TYPE_SPINNER, EDITTYPE_POS_FLOAT, IDC_TEXT_SPPM_RADIANCE_ALPHA, IDC_SPINNER_SPPM_RADIANCE_ALPHA, SPIN_AUTOSCALE,
        p_default, 0.7f,
        p_range, 0.0f, 1.0f,
        p_accessor, &g_pblock_accessor,
     p_end,

     // --- Parameters specifications for Post-Processing rollup ---

    ParamIdDenoiseMode, L"denoiser_mode", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdPostProcessing, TYPE_INT_COMBOBOX, IDC_COMBO_DENOISE_MODE,
        3, IDS_RENDERERPARAMS_DENOISE_MODE_1, IDS_RENDERERPARAMS_DENOISE_MODE_2, IDS_RENDERERPARAMS_DENOISE_MODE_3,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableRenderStamp, L"enable_render_stamp", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPostProcessing, TYPE_SINGLECHEKBOX, IDC_CHECK_RENDER_STAMP,
        p_default, FALSE,
        p_enable_ctrls, 1, ParamIdRenderStampFormat,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdRenderStampFormat, L"render_stamp_format", TYPE_STRING, P_TRANSIENT, 0,
        p_ui, ParamMapIdPostProcessing, TYPE_EDITBOX, IDC_TEXT_RENDER_STAMP,
        p_default, L"appleseed {lib-version} | Time: {render-time}",
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableSkipDenoisedPixel, L"skip_denoised_pixels", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPostProcessing, TYPE_SINGLECHEKBOX, IDC_CHECK_SKIP_DENOISED_PIXELS,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableRandomPixelOrder, L"enable_random_pixel_order", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPostProcessing, TYPE_SINGLECHEKBOX, IDC_CHECK_RANDOM_PIXEL_ORDER,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnablePrefilterSpikes, L"enable_spike_prefiltering", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdPostProcessing, TYPE_SINGLECHEKBOX, IDC_CHECK_PREFILTER_SPIKES,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdSpikeThreshold, L"spike_threshold", TYPE_FLOAT, P_TRANSIENT, 0,
       p_ui, ParamMapIdPostProcessing, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_SPIKE_THRESHOLD, IDC_SPINNER_SPIKE_THRESHOLD, SPIN_AUTOSCALE,
       p_default, 2.0f,
       p_range, 0.1f, 4.0f,
       p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdPatchDistance, L"patch_distance_threshold", TYPE_FLOAT, P_TRANSIENT, 0,
       p_ui, ParamMapIdPostProcessing, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_PATCH_DISTANCE, IDC_SPINNER_PATCH_DISTANCE, SPIN_AUTOSCALE,
       p_default, 1.0f,
       p_range, 0.5f, 3.0f,
       p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdDenoiseScales, L"denoise_scales", TYPE_INT, P_TRANSIENT, 0,
       p_ui, ParamMapIdPostProcessing, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_DENOISE_SCALES, IDC_SPINNER_DENOISE_SCALES, SPIN_AUTOSCALE,
       p_default, 3,
       p_range, 1, 10,
       p_accessor, &g_pblock_accessor,
    p_end,

    // --- Parameters specifications for System rollup ---

    ParamIdCPUCores, L"cpu_cores", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSystem, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_RENDERINGTHREADS, IDC_SPINNER_RENDERINGTHREADS, SPIN_AUTOSCALE,
        p_default, 0,
        p_range, -255, 256,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdOpenLogMode, L"log_open_mode", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSystem, TYPE_INT_COMBOBOX, IDC_COMBO_LOG,
        3, IDS_RENDERERPARAMS_LOG_OPEN_MODE_1, IDS_RENDERERPARAMS_LOG_OPEN_MODE_2,
        IDS_RENDERERPARAMS_LOG_OPEN_MODE_3,
        p_default, 0,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdLogMaterialRendering, L"log_material_editor_rendering", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_LOG_MATERIAL_EDITOR,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdUseMaxProcedurals, L"use_max_procedural_maps", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_USE_MAX_PROCEDURAL_MAPS,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,
    
    ParamIdEnableLowPriority, L"low_priority_mode", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_LOW_PRIORITY_MODE,
        p_default, TRUE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdEnableEmbree, L"enable_embree", TYPE_BOOL, P_TRANSIENT, 0,
        p_ui, ParamMapIdSystem, TYPE_SINGLECHEKBOX, IDC_CHECK_ENABLE_EMBREE,
        p_default, FALSE,
        p_accessor, &g_pblock_accessor,
    p_end,

    ParamIdTextureCacheSize, L"texture_cache_size", TYPE_INT, P_TRANSIENT, 0,
        p_ui, ParamMapIdSystem, TYPE_SPINNER, EDITTYPE_INT, IDC_TEXT_TEXTURE_CACHE_SIZE, IDC_SPINNER_TEXTURE_CACHE_SIZE, SPIN_AUTOSCALE,
        p_default, 1024,
        p_range, 1, 1024*1024,
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
        m_param_block = static_cast<IParamBlock2*>(rtarg);
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
#if MAX_RELEASE < 19000
    if (id == IRENDERERREQUIREMENTS_INTERFACE_ID)
        return static_cast<IRendererRequirements*>(this);
#endif
    if (id == TAB_DIALOG_OBJECT_INTERFACE_ID)
        return static_cast<ITabDialogObject*>(this);
    else return Renderer::GetInterface(id);
}

bool AppleseedRenderer::HasRequirement(Requirement requirement)
{
    switch (requirement)
    {
      case kRequirement_NoVFB:
      case kRequirement_DontSaveRenderOutput:
        return m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectOnly;
#if MAX_RELEASE < 19000
      case kRequirement8_Wants32bitFPOutput: return true;
#else
      case kRequirement_Wants32bitFPOutput: return true;
      case kRequirement_SupportsConcurrentRendering: return true;
#endif
    }

    return false;
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
        renderer_settings.m_lighting_algorithm = 0;
        renderer_settings.m_sampler_type = 0;
        renderer_settings.m_shader_override = 0;
        renderer_settings.m_denoise_mode = 0;
        renderer_settings.m_uniform_pixel_samples = renderer_settings.m_material_preview_quality;
        // renderer_settings.m_uniform_pixel_samples = m_rend_params.mtlEditAA ? 32 : 4;
        renderer_settings.m_passes = 1;
        renderer_settings.m_enable_gi = true;
        renderer_settings.m_dl_enable_dl = true;
        renderer_settings.m_background_emits_light = false;
        renderer_settings.m_dl_light_samples = 1;
        renderer_settings.m_dl_low_light_threshold = 0.0f;
        renderer_settings.m_ibl_env_samples = 1;
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

            AppleseedRenderContext render_context(
                static_cast<Renderer*>(this),
                bitmap,
                m_rend_params,
                m_view_params,
                time);

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
    const auto* dialog_string_pair = LOOKUP_KVPAIR_ARRAY(g_dialog_strings, id);

    if (dialog_string_pair != nullptr)
        return dialog_string_pair->m_value;
    else
        return nullptr;
}
