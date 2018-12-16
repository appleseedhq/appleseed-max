
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
#include "renderersettings.h"

// appleseed-max headers.
#include "appleseedrenderer/datachunks.h"
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/utility.h"

// 3ds Max headers.
#include <ioapi.h>

namespace asr = renderer;

namespace
{
    struct DefaultRendererSettings
      : public RendererSettings
    {
        DefaultRendererSettings()
        {
            m_passes = 1;
            m_tile_size = 64;
            m_sampler_type = 0;
            m_noise_seed = 0;
            m_enable_noise_seed = false;

            m_uniform_pixel_samples = 16;

            m_adaptive_batch_size = 16;
            m_adaptive_min_samples = 0;
            m_adaptive_max_samples = 256;
            m_adaptive_noise_threshold = 1.0f;

            m_pixel_filter = 0;
            m_pixel_filter_size = 1.5f;
            m_background_alpha = 1.0f;

            m_lighting_algorithm = 0;
            m_force_off_default_lights = false;
            m_enable_light_importance_sampling = false;
            m_light_sampling_algorithm = 0;

            m_enable_gi = true;
            m_enable_caustics = false;
            m_global_bounces = 8;
            m_diffuse_bounces = 3;
            m_diffuse_bounces_enabled = false;
            m_glossy_bounces = 8;
            m_glossy_bounces_enabled = false;
            m_specular_bounces = 8;
            m_specular_bounces_enabled = false;
            m_volume_bounces = 8;
            m_volume_bounces_enabled = false;
            m_volume_distance_samples = 2;
            m_optimize_for_lights_outside_volumes = false;
            m_dl_enable_dl = true;
            m_background_emits_light = true;
            m_dl_light_samples = 1;
            m_dl_low_light_threshold = 0.0f;
            m_ibl_env_samples = 1;
            m_rr_min_path_length = 6;
            m_max_ray_intensity_set = false;
            m_max_ray_intensity = 1.0f;
            m_clamp_roughness = false;

            m_sppm_photon_type = 1;
            m_sppm_direct_lighting_mode = 0;
            m_sppm_enable_caustics = true;
            m_sppm_enable_ibl = true;
            m_sppm_photon_tracing_max_bounces = 8;
            m_sppm_photon_tracing_enable_bounce_limit = false;
            m_sppm_photon_tracing_rr_min_path_length = 6;
            m_sppm_photon_tracing_light_photons = 1000000;
            m_sppm_photon_tracing_environment_photons = 1000000;
            m_sppm_radiance_estimation_max_bounces = 8;
            m_sppm_radiance_estimation_enable_bounce_limit = false;
            m_sppm_radiance_estimation_rr_min_path_length = 6;
            m_sppm_radiance_estimation_initial_radius = 0.1f;
            m_sppm_radiance_estimation_max_photons = 100;
            m_sppm_radiance_estimation_alpha = 0.7f;
            m_sppm_view_photons = false;
            m_sppm_view_photons_radius = 0.05f;
            m_sppm_max_ray_intensity_set = false;
            m_sppm_max_ray_intensity = 1.0f;

            m_output_mode = OutputMode::RenderOnly;
            m_scale_multiplier = 1.0f;
            m_shader_override = 0;
            m_material_preview_quality = 4; // number of uniform pixel samples

            m_rendering_threads = 0;        // 0 = as many as there are logical cores
            m_enable_embree = false;
            m_low_priority_mode = true;
            m_use_max_procedural_maps = false;
            m_texture_cache_size = 1024;    // value in MB

            const int log_open_mode = load_system_setting(L"LogOpenMode", static_cast<int>(DialogLogTarget::OpenMode::Errors));
            m_log_open_mode = static_cast<DialogLogTarget::OpenMode>(log_open_mode);
            m_log_material_editor_messages = load_system_setting(L"LogMaterialEditorMessages", false);

            m_enable_render_stamp = false;
            m_render_stamp_format = L"appleseed {lib-version} | Time: {render-time}";
            m_denoise_mode = 0;
            m_enable_skip_denoised = true;
            m_enable_random_pixel_order = true;
            m_enable_prefilter_spikes = true;
            m_spike_threshold = 2.0f;
            m_patch_distance_threshold = 1.0f;
            m_denoise_scales = 3;
        }
    };
}

const char* get_shader_override_type(const int shader_override_type)
{
    switch (shader_override_type)
    {
      case 0:  return "no_override";
      case 1:  return "albedo";
      case 2:  return "ambient_occlusion";
      case 3:  return "assembly_instances";
      case 4:  return "barycentric";
      case 5:  return "bitangent";
      case 6:  return "coverage";
      case 7:  return "depth";
      case 8:  return "facing_ratio";
      case 9:  return "geometric_normal";
      case 10: return "materials";
      case 11: return "object_instances";
      case 12: return "original_shading_normal";
      case 13: return "primitives";
      case 14: return "ray_spread";
      case 15: return "regions";
      case 16: return "screen_space_wireframe";
      case 17: return "shading_normal";
      case 18: return "sides";
      case 19: return "tangent";
      case 20: return "uv";
      case 21: return "world_space_position";
      case 22: return "world_space_wireframe";
      default: 
        assert(!"Invalid shader override type."); 
        return "coverage";
    }
}

const char* get_lighting_engine_type(const int lighting_engine_type)
{
    switch (lighting_engine_type)
    {
      case 0:  return "pt";
      case 1:  return "sppm";
      default:
        assert(!"Invalid lighting engine type.");
        return "pt";
    }
}

const char* get_lighting_algorithm_type(const int lighting_algorithm_type)
{
    switch (lighting_algorithm_type)
    {
      case 0:  return "cdf";
      case 1:  return "lighttree";
      default:
        assert(!"Invalid lighting algorithm type.");
        return "cdf";
    }
}

const char* get_sppm_photon_type(const int photon_type)
{
    switch (photon_type)
    {
      case 0:  return "mono";
      case 1:  return "poly";
      default: 
        assert(!"Invalid photon type."); 
        return "poly";
    }
}

const char* get_sppm_direct_lighting_mode(const int lighting_mode)
{
    switch (lighting_mode)
    {
      case 0:  return "rt";
      case 1:  return "sppm";
      case 2:  return "off";
      default: 
        assert(!"Invalid SPPM direct lighting mode."); 
        return "rt";
    }
}

const RendererSettings& RendererSettings::defaults()
{
    static DefaultRendererSettings default_settings;
    return default_settings;
}

void RendererSettings::apply(asr::Project& project) const
{
    apply_common_settings(project, "final");
    apply_common_settings(project, "interactive");

    apply_settings_to_final_config(project);
    apply_settings_to_interactive_config(project);
}

void RendererSettings::apply_common_settings(asr::Project& project, const char* config_name) const
{
    asr::ParamArray& params = project.configurations().get_by_name(config_name)->get_parameters();

    params.insert_path("sampling_mode", "qmc");
    params.insert_path("lighting_engine", get_lighting_engine_type(m_lighting_algorithm));

    if (m_enable_light_importance_sampling)
        params.insert_path("light_sampler.enable_importance_sampling", m_enable_light_importance_sampling);

    params.insert_path("light_sampler.algorithm", get_lighting_algorithm_type(m_light_sampling_algorithm));

    params.insert_path("pt.max_bounces", m_global_bounces);

    if (m_diffuse_bounces_enabled)
        params.insert_path("pt.max_diffuse_bounces", m_diffuse_bounces);

    if (!m_enable_gi)
        params.insert_path("pt.max_diffuse_bounces", 0);

    if (m_glossy_bounces_enabled)
        params.insert_path("pt.max_glossy_bounces", m_glossy_bounces);

    if (m_specular_bounces_enabled)
        params.insert_path("pt.max_specular_bounces", m_specular_bounces);

    if (m_volume_bounces_enabled)
        params.insert_path("pt.max_volume_bounces", m_volume_bounces);

    if (!m_dl_enable_dl)
        params.insert_path("pt.enable_dl", false);

    params.insert_path("pt.dl_light_samples", m_dl_light_samples);
    params.insert_path("pt.dl_low_light_threshold", m_dl_low_light_threshold);
    params.insert_path("pt.ibl_env_samples", m_ibl_env_samples);
    params.insert_path("pt.enable_ibl", m_background_emits_light);
    params.insert_path("pt.enable_caustics", m_enable_caustics);
    params.insert_path("pt.rr_min_path_length", m_rr_min_path_length);
    params.insert_path("pt.volume_distance_samples", m_volume_distance_samples);
    params.insert_path("pt.optimize_for_lights_outside_volumes", m_optimize_for_lights_outside_volumes);
    params.insert_path("pt.clamp_roughness", m_clamp_roughness); 

    if (m_max_ray_intensity_set)
        params.insert_path("pt.max_ray_intensity", m_max_ray_intensity);

    params.insert_path("sppm.photon_type", get_sppm_photon_type(m_sppm_photon_type));
    params.insert_path("sppm.dl_mode", get_sppm_direct_lighting_mode(m_sppm_direct_lighting_mode));
    params.insert_path("sppm.enable_caustics", m_sppm_enable_caustics);
    params.insert_path("sppm.enable_ibl", m_sppm_enable_ibl);

    if (m_sppm_max_ray_intensity_set)
        params.insert_path("sppm.path_tracing_max_ray_intensity", m_sppm_max_ray_intensity);

    if (!m_sppm_photon_tracing_enable_bounce_limit)
        params.insert_path("sppm.path_tracing_max_bounces", -1);

    params.insert_path("sppm.path_tracing_rr_min_path_length", m_sppm_photon_tracing_rr_min_path_length);
    params.insert_path("sppm.light_photons_per_pass", m_sppm_photon_tracing_light_photons);
    params.insert_path("sppm.env_photons_per_pass", m_sppm_photon_tracing_environment_photons);

    if (!m_sppm_radiance_estimation_enable_bounce_limit)
        params.insert_path("sppm.photon_tracing_max_bounces", -1);

    params.insert_path("sppm.photon_tracing_rr_min_path_length", m_sppm_radiance_estimation_rr_min_path_length);
    params.insert_path("sppm.initial_radius", m_sppm_radiance_estimation_initial_radius);
    params.insert_path("sppm.max_photons_per_estimate", m_sppm_radiance_estimation_max_photons);
    params.insert_path("sppm.alpha", m_sppm_radiance_estimation_alpha);
    params.insert_path("sppm.view_photons", m_sppm_view_photons);
    params.insert_path("sppm.view_photons_radius", m_sppm_view_photons_radius);

    params.insert_path("use_embree", m_enable_embree);
    params.insert_path("texture_store.max_size", m_texture_cache_size * 1024 * 1024);

    if (m_rendering_threads == 0)
        params.insert_path("rendering_threads", "auto");
    else params.insert_path("rendering_threads", m_rendering_threads);

    if (m_shader_override > 0)
       params.insert_path("shading_engine.override_shading.mode", get_shader_override_type(m_shader_override));  
}

void RendererSettings::apply_settings_to_final_config(asr::Project& project) const
{
    asr::ParamArray& params = project.configurations().get_by_name("final")->get_parameters();

    params.insert_path("generic_frame_renderer.tile_ordering", "spiral");
    params.insert_path("passes", m_passes);
    params.insert_path("shading_result_framebuffer", m_passes == 1 ? "ephemeral" : "permanent");

    if (m_sampler_type == 0)
    {
        params.insert_path("pixel_renderer", "uniform");
        params.insert_path("uniform_pixel_renderer.samples", m_uniform_pixel_samples);
        if (m_uniform_pixel_samples == 1)
            params.insert_path("uniform_pixel_renderer.force_antialiasing", true);
    }
    else
    {
        params.insert_path("tile_renderer", "adaptive");
        params.insert_path("adaptive_tile_renderer.batch_size", m_adaptive_batch_size);
        params.insert_path("adaptive_tile_renderer.min_samples", m_adaptive_min_samples);
        params.insert_path("adaptive_tile_renderer.max_samples", m_adaptive_max_samples);
        params.insert_path("adaptive_tile_renderer.noise_threshold", m_adaptive_noise_threshold);
    }
}

void RendererSettings::apply_settings_to_interactive_config(asr::Project& project) const
{
    asr::ParamArray& params = project.configurations().get_by_name("interactive")->get_parameters();

    params.insert_path("frame_renderer", "progressive");
    params.insert_path("sample_generator", "generic");
    params.insert_path("sample_renderer", "generic");
    params.insert_path("lighting_engine", "pt");

    if (m_rendering_threads == 0)
        params.insert_path("rendering_threads", "-1");
    else params.insert_path("rendering_threads", m_rendering_threads);
}

bool RendererSettings::save(ISave* isave) const
{
    bool success = true;

    //
    // Image Sampling settings.
    //

    isave->BeginChunk(ChunkSettingsImageSampling);

        isave->BeginChunk(ChunkSettingsImageSamplingPixelSamples);
        success &= write<int>(isave, m_uniform_pixel_samples);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsImageSamplingPasses);
        success &= write<int>(isave, m_passes);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsImageSamplingTileSize);
        success &= write<int>(isave, m_tile_size);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPixelFilter);
        success &= write<int>(isave, m_pixel_filter);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPixelFilterSize);
        success &= write<float>(isave, m_pixel_filter_size);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsImageSamplerType);
        success &= write<int>(isave, m_sampler_type);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsAdaptiveTileBatchSize);
        success &= write<int>(isave, m_adaptive_batch_size);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsAdaptiveTileMinSamples);
        success &= write<int>(isave, m_adaptive_min_samples);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsAdaptiveTileMaxSamples);
        success &= write<int>(isave, m_adaptive_max_samples);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsAdaptiveTileNoiseThreshold);
        success &= write<float>(isave, m_adaptive_noise_threshold);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsNoiseSeed);
        success &= write<int>(isave, m_noise_seed);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsEnableNoiseSeed);
        success &= write<bool>(isave, m_enable_noise_seed);
        isave->EndChunk();

    isave->EndChunk();

    //
    // Lighting settings.
    //

    isave->BeginChunk(ChunkSettingsLighting);

        isave->BeginChunk(ChunkSettingsLightingAlgorithm);
        success &= write<int>(isave, m_lighting_algorithm);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightSamplingAlgorithm);
        success &= write<int>(isave, m_light_sampling_algorithm);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsEnableLightImportanceSampling);
        success &= write<bool>(isave, m_enable_light_importance_sampling);
        isave->EndChunk();

    isave->EndChunk();

    //
    // Path Tracer settings.
    //

    isave->BeginChunk(ChunkSettingsPathtracer);

        isave->BeginChunk(ChunkSettingsPathtracerGI);
        success &= write<bool>(isave, m_enable_gi);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerRRMinPathLength);
        success &= write<int>(isave, m_rr_min_path_length);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerCaustics);
        success &= write<bool>(isave, m_enable_caustics);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerGlobalBounces);
        success &= write<int>(isave, m_global_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerDiffuseBouncesEnabled);
        success &= write<bool>(isave, m_diffuse_bounces_enabled);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerDiffuseBounces);
        success &= write<int>(isave, m_diffuse_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerGlossyBouncesEnabled);
        success &= write<bool>(isave, m_glossy_bounces_enabled);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerGlossyBounces);
        success &= write<int>(isave, m_glossy_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerSpecularBouncesEnabled);
        success &= write<bool>(isave, m_specular_bounces_enabled);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerSpecularBounces);
        success &= write<int>(isave, m_specular_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerVolumeBouncesEnabled);
        success &= write<bool>(isave, m_volume_bounces_enabled);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerVolumeBounces);
        success &= write<int>(isave, m_volume_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerVolumeDistanceSamples);
        success &= write<int>(isave, m_volume_distance_samples);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerMaxRayIntensitySet);
        success &= write<bool>(isave, m_max_ray_intensity_set);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerMaxRayIntensity);
        success &= write<float>(isave, m_max_ray_intensity);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerClampRougness);
        success &= write<bool>(isave, m_clamp_roughness);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerBackgroundEmitsLight);
        success &= write<bool>(isave, m_background_emits_light);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerDirectLightingEnabled);
        success &= write<bool>(isave, m_dl_enable_dl);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerDirectLightingSamples);
        success &= write<int>(isave, m_dl_light_samples);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerDirectLightingLowLightThreshold);
        success &= write<float>(isave, m_dl_low_light_threshold);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerImageBasedLightingSamples);
        success &= write<int>(isave, m_ibl_env_samples);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPixelBackgroundAlpha);
        success &= write<float>(isave, m_background_alpha);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightingForceOffDefaultLights);
        success &= write<bool>(isave, m_force_off_default_lights);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPathtracerOptimizeLightsOutsideVolumes);
        success &= write<bool>(isave, m_optimize_for_lights_outside_volumes);
        isave->EndChunk();

    isave->EndChunk();

    //
    // Stochastic Progressive Photon Mapping settings.
    //

    isave->BeginChunk(ChunkSettingsSPPM);

        isave->BeginChunk(ChunkSettingsSPPMPhotonType);
        success &= write<int>(isave, m_sppm_photon_type);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMDirectLightingMode);
        success &= write<int>(isave, m_sppm_direct_lighting_mode);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMEnableCaustics);
        success &= write<bool>(isave, m_sppm_enable_caustics);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMEnableImageBasedLighting);
        success &= write<bool>(isave, m_sppm_enable_ibl);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMPhotonTracingMaxBounces);
        success &= write<int>(isave, m_sppm_photon_tracing_max_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMPhotonTracingEnableBounceLimit);
        success &= write<bool>(isave, m_sppm_photon_tracing_enable_bounce_limit);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMPhotonTracingRRMinPathLength);
        success &= write<int>(isave, m_sppm_photon_tracing_rr_min_path_length);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMPhotonTracingLightPhotons);
        success &= write<int>(isave, m_sppm_photon_tracing_light_photons);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMPhotonTracingEnvironmentPhotons);
        success &= write<int>(isave, m_sppm_photon_tracing_environment_photons);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMRadianceEstimationMaxBounces);
        success &= write<int>(isave, m_sppm_radiance_estimation_max_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMRadianceEstimationEnableBounceLimit);
        success &= write<bool>(isave, m_sppm_radiance_estimation_enable_bounce_limit);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMRadianceEstimationRRMinPathLength);
        success &= write<int>(isave, m_sppm_radiance_estimation_rr_min_path_length);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMRadianceEstimationInitialRadius);
        success &= write<float>(isave, m_sppm_radiance_estimation_initial_radius);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMRadianceEstimationMaxPhotons);
        success &= write<int>(isave, m_sppm_radiance_estimation_max_photons);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMRadianceEstimationAlpha);
        success &= write<float>(isave, m_sppm_radiance_estimation_alpha);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMViewPhotons);
        success &= write<bool>(isave, m_sppm_view_photons);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMViewPhotonsRadius);
        success &= write<float>(isave, m_sppm_view_photons_radius);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMMaxRayIntensitySet);
        success &= write<bool>(isave, m_sppm_max_ray_intensity_set);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSPPMMaxRayIntensity);
        success &= write<float>(isave, m_sppm_max_ray_intensity);
        isave->EndChunk();

    isave->EndChunk();

    //
    // Output settings.
    //

    isave->BeginChunk(ChunkSettingsOutput);

        isave->BeginChunk(ChunkSettingsOutputMode);
        switch (m_output_mode)
        {
          case OutputMode::RenderOnly:
            success &= write<BYTE>(isave, 0x00);
            break;
          case OutputMode::SaveProjectOnly:
            success &= write<BYTE>(isave, 0x01);
            break;
          case OutputMode::SaveProjectAndRender:
            success &= write<BYTE>(isave, 0x02);
            break;
        }
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsOutputProjectFilePath);
        success &= write(isave, m_project_file_path);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsOutputScaleMultiplier);
        success &= write<float>(isave, m_scale_multiplier);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsOutputShaderOverride);
        success &= write<int>(isave, m_shader_override);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsOutputMaterialPreviewQuality);
        success &= write<int>(isave, m_material_preview_quality);
        isave->EndChunk();

    isave->EndChunk();

    //
    // Post-processing settings.
    //

    isave->BeginChunk(ChunkSettingsPostprocessing);

        isave->BeginChunk(ChunkSettingsPostprocessingDenoiseMode);
        success &= write<int>(isave, m_denoise_mode);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPostprocessingEnableSkipDenoised);
        success &= write<bool>(isave, m_enable_skip_denoised);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPostprocessingEnableRandomPixelOrder);
        success &= write<bool>(isave, m_enable_random_pixel_order);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPostprocessingEnablePrefilterSpikes);
        success &= write<bool>(isave, m_enable_prefilter_spikes);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPostprocessingSpikeThreshold);
        success &= write<float>(isave, m_spike_threshold);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPostprocessingPatchDistanceThreshold);
        success &= write<float>(isave, m_patch_distance_threshold);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsPostprocessingDenoiseScales);
        success &= write<int>(isave, m_denoise_scales);
        isave->EndChunk();

    isave->EndChunk();

    //
    // System settings.
    //

    isave->BeginChunk(ChunkSettingsSystem);

        isave->BeginChunk(ChunkSettingsSystemRenderingThreads);
        success &= write<int>(isave, m_rendering_threads);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSystemLowPriorityMode);
        success &= write<bool>(isave, m_low_priority_mode);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSystemUseMaxProceduralMaps);
        success &= write<bool>(isave, m_use_max_procedural_maps);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSystemEnableRenderStamp);
        success &= write<bool>(isave, m_enable_render_stamp);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSystemRenderStampString);
        success &= write(isave, m_render_stamp_format);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSystemEnableEmbree);
        success &= write<bool>(isave, m_enable_embree);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsSystemTextureCacheSize);
        success &= write<foundation::uint64>(isave, m_texture_cache_size);
        isave->EndChunk();
        
    isave->EndChunk();

    return success;
}

IOResult RendererSettings::load(ILoad* iload)
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
          case ChunkSettingsImageSampling:
            result = load_image_sampling_settings(iload);
            break;

          case ChunkSettingsLighting:
            result = load_lighting_settings(iload);
            break;

          case ChunkSettingsPathtracer:
            result = load_pathtracer_settings(iload);
            break;

          case ChunkSettingsSPPM:
            result = load_sppm_settings(iload);
            break;

          case ChunkSettingsOutput:
            result = load_output_settings(iload);
            break;

          case ChunkSettingsSystem:
            result = load_system_settings(iload);
            break;

          case ChunkSettingsPostprocessing:
            result = load_postprocessing_settings(iload);
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

IOResult RendererSettings::load_image_sampling_settings(ILoad* iload)
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
          case ChunkSettingsImageSamplingPixelSamples:
            result = read<int>(iload, &m_uniform_pixel_samples);
            break;

          case ChunkSettingsImageSamplingPasses:
            result = read<int>(iload, &m_passes);
            break;

          case ChunkSettingsImageSamplingTileSize:
            result = read<int>(iload, &m_tile_size);
            break;

          case ChunkSettingsPixelFilter:
            result = read<int>(iload, &m_pixel_filter);
            break;

          case ChunkSettingsPixelFilterSize:
            result = read<float>(iload, &m_pixel_filter_size);
            break;

          case ChunkSettingsImageSamplerType:
            result = read<int>(iload, &m_sampler_type);
            break;

          case ChunkSettingsAdaptiveTileBatchSize:
            result = read<int>(iload, &m_adaptive_batch_size);
            break;

          case ChunkSettingsAdaptiveTileMinSamples:
            result = read<int>(iload, &m_adaptive_min_samples);
            break;

          case ChunkSettingsAdaptiveTileMaxSamples:
            result = read<int>(iload, &m_adaptive_max_samples);
            break;

          case ChunkSettingsAdaptiveTileNoiseThreshold:
            result = read<float>(iload, &m_adaptive_noise_threshold);
            break;

          case ChunkSettingsNoiseSeed:
            result = read<int>(iload, &m_noise_seed);
            break;

          case ChunkSettingsEnableNoiseSeed:
            result = read<bool>(iload, &m_enable_noise_seed);
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

IOResult RendererSettings::load_pathtracer_settings(ILoad* iload)
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
          case ChunkSettingsPathtracerGI:
            result = read<bool>(iload, &m_enable_gi);
            break;

          case ChunkSettingsPathtracerRRMinPathLength:
            result = read<int>(iload, &m_rr_min_path_length);
            break;

          case ChunkSettingsPathtracerCaustics:
            result = read<bool>(iload, &m_enable_caustics);
            break;

          case ChunkSettingsPathtracerGlobalBounces:
            result = read<int>(iload, &m_global_bounces);
            break;

          case ChunkSettingsPathtracerDiffuseBouncesEnabled:
            result = read<bool>(iload, &m_diffuse_bounces_enabled);
            break;

          case ChunkSettingsPathtracerDiffuseBounces:
            result = read<int>(iload, &m_diffuse_bounces);
            break;

          case ChunkSettingsPathtracerGlossyBouncesEnabled:
            result = read<bool>(iload, &m_glossy_bounces_enabled);
            break;

          case ChunkSettingsPathtracerGlossyBounces:
            result = read<int>(iload, &m_glossy_bounces);
            break;

          case ChunkSettingsPathtracerSpecularBouncesEnabled:
            result = read<bool>(iload, &m_specular_bounces_enabled);
            break;

          case ChunkSettingsPathtracerSpecularBounces:
            result = read<int>(iload, &m_specular_bounces);
            break;

          case ChunkSettingsPathtracerVolumeBouncesEnabled:
            result = read<bool>(iload, &m_volume_bounces_enabled);
            break;

          case ChunkSettingsPathtracerVolumeBounces:
            result = read<int>(iload, &m_volume_bounces);
            break;

          case ChunkSettingsPathtracerVolumeDistanceSamples:
            result = read<int>(iload, &m_volume_distance_samples);
            break;

          case ChunkSettingsPathtracerMaxRayIntensitySet:
            result = read<bool>(iload, &m_max_ray_intensity_set);
            break;

          case ChunkSettingsPathtracerMaxRayIntensity:
            result = read<float>(iload, &m_max_ray_intensity);
            break;

          case ChunkSettingsPathtracerClampRougness:
            result = read<bool>(iload, &m_clamp_roughness);
            break;

          case ChunkSettingsPathtracerBackgroundEmitsLight:
            result = read<bool>(iload, &m_background_emits_light);
            break;

          case ChunkSettingsPathtracerDirectLightingEnabled:
            result = read<bool>(iload, &m_dl_enable_dl);
            break;

          case ChunkSettingsPathtracerDirectLightingSamples:
            result = read<int>(iload, &m_dl_light_samples);
            break;

          case ChunkSettingsPathtracerDirectLightingLowLightThreshold:
            result = read<float>(iload, &m_dl_low_light_threshold);
            break;

          case ChunkSettingsPathtracerImageBasedLightingSamples:
            result = read<int>(iload, &m_ibl_env_samples);
            break;

          case ChunkSettingsPixelBackgroundAlpha:
            result = read<float>(iload, &m_background_alpha);
            break;

          case ChunkSettingsLightingForceOffDefaultLights:
            result = read<bool>(iload, &m_force_off_default_lights);
            break;

          case ChunkSettingsPathtracerOptimizeLightsOutsideVolumes:
            result = read<bool>(iload, &m_optimize_for_lights_outside_volumes);
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

IOResult RendererSettings::load_sppm_settings(ILoad* iload)
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
          case ChunkSettingsSPPMPhotonType:
            result = read<int>(iload, &m_sppm_photon_type);
            break;

          case ChunkSettingsSPPMDirectLightingMode:
            result = read<int>(iload, &m_sppm_direct_lighting_mode);
            break;

          case ChunkSettingsSPPMEnableCaustics:
            result = read<bool>(iload, &m_sppm_enable_caustics);
            break;

          case ChunkSettingsSPPMEnableImageBasedLighting:
            result = read<bool>(iload, &m_sppm_enable_ibl);
            break;

          case ChunkSettingsSPPMPhotonTracingMaxBounces:
            result = read<int>(iload, &m_sppm_photon_tracing_max_bounces);
            break;

          case ChunkSettingsSPPMPhotonTracingEnableBounceLimit:
            result = read<bool>(iload, &m_sppm_photon_tracing_enable_bounce_limit);
            break;

          case ChunkSettingsSPPMPhotonTracingRRMinPathLength:
            result = read<int>(iload, &m_sppm_photon_tracing_rr_min_path_length);
            break;

          case ChunkSettingsSPPMPhotonTracingLightPhotons:
            result = read<int>(iload, &m_sppm_photon_tracing_light_photons);
            break;

          case ChunkSettingsSPPMPhotonTracingEnvironmentPhotons:
            result = read<int>(iload, &m_sppm_photon_tracing_environment_photons);
            break;

          case ChunkSettingsSPPMRadianceEstimationMaxBounces:
            result = read<int>(iload, &m_sppm_radiance_estimation_max_bounces);
            break;

          case ChunkSettingsSPPMRadianceEstimationEnableBounceLimit:
            result = read<bool>(iload, &m_sppm_radiance_estimation_enable_bounce_limit);
            break;

          case ChunkSettingsSPPMRadianceEstimationRRMinPathLength:
            result = read<int>(iload, &m_sppm_radiance_estimation_rr_min_path_length);
            break;

          case ChunkSettingsSPPMRadianceEstimationInitialRadius:
            result = read<float>(iload, &m_sppm_radiance_estimation_initial_radius);
            break;

          case ChunkSettingsSPPMRadianceEstimationMaxPhotons:
            result = read<int>(iload, &m_sppm_radiance_estimation_max_photons);
            break;

          case ChunkSettingsSPPMRadianceEstimationAlpha:
            result = read<float>(iload, &m_sppm_radiance_estimation_alpha);
            break;

          case ChunkSettingsSPPMViewPhotons:
            result = read<bool>(iload, &m_sppm_view_photons);
            break;

          case ChunkSettingsSPPMViewPhotonsRadius:
            result = read<float>(iload, &m_sppm_view_photons_radius);
            break;

          case ChunkSettingsSPPMMaxRayIntensitySet:
            result = read<bool>(iload, &m_sppm_max_ray_intensity_set);
            break;

          case ChunkSettingsSPPMMaxRayIntensity:
            result = read<float>(iload, &m_sppm_max_ray_intensity);
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


IOResult RendererSettings::load_output_settings(ILoad* iload)
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
          case ChunkSettingsOutputMode:
            {
                BYTE mode;
                result = read<BYTE>(iload, &mode);
                if (result == IO_OK)
                {
                    switch (mode)
                    {
                      case 0x00:
                        m_output_mode = OutputMode::RenderOnly;
                        break;
                      case 0x01:
                        m_output_mode = OutputMode::SaveProjectOnly;
                        break;
                      case 0x02:
                        m_output_mode = OutputMode::SaveProjectAndRender;
                        break;
                      default:
                        result = IO_ERROR;
                        break;
                    }
                }
            }
            break;

          case ChunkSettingsOutputProjectFilePath:
            result = read(iload, &m_project_file_path);
            break;

          case ChunkSettingsOutputScaleMultiplier:
            result = read(iload, &m_scale_multiplier);
            break;

          case ChunkSettingsOutputShaderOverride:
            result = read(iload, &m_shader_override);
            break;

          case ChunkSettingsOutputMaterialPreviewQuality:
            result = read(iload, &m_material_preview_quality);
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

IOResult RendererSettings::load_system_settings(ILoad* iload)
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
          case ChunkSettingsSystemRenderingThreads:
            result = read<int>(iload, &m_rendering_threads);
            break;

          case ChunkSettingsSystemLowPriorityMode:
            result = read<bool>(iload, &m_low_priority_mode);
            break;

          case ChunkSettingsSystemUseMaxProceduralMaps:
            result = read<bool>(iload, &m_use_max_procedural_maps);
            break;

          case ChunkSettingsSystemEnableRenderStamp:
            result = read<bool>(iload, &m_enable_render_stamp);
            break;

          case ChunkSettingsSystemRenderStampString:
            result = read(iload, &m_render_stamp_format);
            break;

          case ChunkSettingsSystemEnableEmbree:
            result = read<bool>(iload, &m_enable_embree);
            break;

          case ChunkSettingsSystemTextureCacheSize:
            result = read<foundation::uint64>(iload, &m_texture_cache_size);
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

IOResult RendererSettings::load_postprocessing_settings(ILoad* iload)
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
          case ChunkSettingsPostprocessingDenoiseMode:
            result = read<int>(iload, &m_denoise_mode);
            break;

          case ChunkSettingsPostprocessingEnableSkipDenoised:
            result = read<bool>(iload, &m_enable_skip_denoised);
            break;

          case ChunkSettingsPostprocessingEnableRandomPixelOrder:
            result = read<bool>(iload, &m_enable_random_pixel_order);
            break;

          case ChunkSettingsPostprocessingEnablePrefilterSpikes:
            result = read<bool>(iload, &m_enable_prefilter_spikes);
            break;

          case ChunkSettingsPostprocessingSpikeThreshold:
            result = read<float>(iload, &m_spike_threshold);
            break;

          case ChunkSettingsPostprocessingPatchDistanceThreshold:
            result = read<float>(iload, &m_patch_distance_threshold);
            break;

          case ChunkSettingsPostprocessingDenoiseScales:
            result = read<int>(iload, &m_denoise_scales);
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

IOResult RendererSettings::load_lighting_settings(ILoad* iload)
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
          case ChunkSettingsLightingAlgorithm:
            result = read<int>(iload, &m_lighting_algorithm);
            break;

          case ChunkSettingsLightSamplingAlgorithm:
            result = read<int>(iload, &m_light_sampling_algorithm);
            break;

          case ChunkSettingsEnableLightImportanceSampling:
            result = read<bool>(iload, &m_enable_light_importance_sampling);
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

