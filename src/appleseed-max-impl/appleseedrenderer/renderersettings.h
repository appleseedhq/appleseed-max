
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

#pragma once

// appleseed-max headers.
#include "dialoglogtarget.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <maxtypes.h>
#include <strclass.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Standard headers.
#include <string>

// Forward declarations.
namespace renderer  { class Project; }
class ILoad;
class ISave;
class Mtl;

class RendererSettings
{
  public:
    static const RendererSettings& defaults();

    //
    // Image Sampling.
    //

    int                         m_passes;
    int                         m_tile_size;
    int                         m_sampler_type;
    int                         m_noise_seed;
    bool                        m_enable_noise_seed;

    //
    // Uniform Pixel Sampler.
    //

    int                         m_uniform_pixel_samples;

    //
    // Adaptive Tile Sampler.
    //

    int                         m_adaptive_batch_size;
    int                         m_adaptive_min_samples;
    int                         m_adaptive_max_samples;
    float                       m_adaptive_noise_threshold;

    //
    // Pixel Filtering and Background Alpha.
    //

    int                         m_pixel_filter;
    float                       m_pixel_filter_size;
    float                       m_background_alpha;

    //
    // Lighting.
    //

    int                         m_lighting_algorithm;
    bool                        m_force_off_default_lights;
    bool                        m_enable_light_importance_sampling;
    int                         m_light_sampling_algorithm;

    //
    // Path Tracer.
    //

    bool                        m_enable_gi;
    bool                        m_enable_caustics;
    int                         m_global_bounces;
    int                         m_diffuse_bounces;
    bool                        m_diffuse_bounces_enabled;
    int                         m_glossy_bounces;
    bool                        m_glossy_bounces_enabled;
    int                         m_specular_bounces;
    bool                        m_specular_bounces_enabled;
    int                         m_volume_bounces;
    bool                        m_volume_bounces_enabled;
    int                         m_volume_distance_samples;
    bool                        m_clamp_roughness;
    bool                        m_max_ray_intensity_set;
    float                       m_max_ray_intensity;
    bool                        m_dl_enable_dl;
    int                         m_dl_light_samples;
    bool                        m_background_emits_light;
    float                       m_dl_low_light_threshold;
    int                         m_ibl_env_samples;
    int                         m_rr_min_path_length;
    bool                        m_optimize_for_lights_outside_volumes;

    //
    // Stochastic Progressive Photon Mapping.
    //

    int                         m_sppm_photon_type;
    int                         m_sppm_direct_lighting_mode;
    bool                        m_sppm_enable_caustics;
    bool                        m_sppm_enable_ibl;
    int                         m_sppm_photon_tracing_max_bounces;
    bool                        m_sppm_photon_tracing_enable_bounce_limit;
    int                         m_sppm_photon_tracing_rr_min_path_length;
    int                         m_sppm_photon_tracing_light_photons;
    int                         m_sppm_photon_tracing_environment_photons;
    int                         m_sppm_radiance_estimation_max_bounces;
    bool                        m_sppm_radiance_estimation_enable_bounce_limit;
    int                         m_sppm_radiance_estimation_rr_min_path_length;
    float                       m_sppm_radiance_estimation_initial_radius;
    int                         m_sppm_radiance_estimation_max_photons;
    float                       m_sppm_radiance_estimation_alpha;
    bool                        m_sppm_view_photons;
    float                       m_sppm_view_photons_radius;
    bool                        m_sppm_max_ray_intensity_set;
    float                       m_sppm_max_ray_intensity;

    //
    // Output.
    //

    enum class OutputMode
    {
        RenderOnly,
        SaveProjectOnly,
        SaveProjectAndRender
    };

    OutputMode                  m_output_mode;
    MSTR                        m_project_file_path;
    float                       m_scale_multiplier;
    int                         m_shader_override;
    int                         m_material_preview_quality;

    bool                        m_enable_override_material;
    bool                        m_override_exclude_light_materials;
    bool                        m_override_exclude_glass_materials;
    Mtl*                        m_override_material;

    //
    // Post-processing.
    //

    bool                        m_enable_render_stamp;
    MSTR                        m_render_stamp_format;
    int                         m_denoise_mode;
    bool                        m_enable_skip_denoised;
    bool                        m_enable_random_pixel_order;
    bool                        m_enable_prefilter_spikes;
    float                       m_spike_threshold;
    float                       m_patch_distance_threshold;
    int                         m_denoise_scales;

    //
    // System.
    //

    int                         m_rendering_threads;
    bool                        m_enable_embree;
    bool                        m_low_priority_mode;
    bool                        m_use_max_procedural_maps;
    DialogLogTarget::OpenMode   m_log_open_mode;
    bool                        m_log_material_editor_messages;
    std::uint64_t               m_texture_cache_size;

    // Apply these settings to a given project.
    void apply(renderer::Project& project) const;

    // Save settings to a 3ds Max file.
    bool save(ISave* isave) const;

    // Load settings from a 3ds Max file.
    IOResult load(ILoad* iload);

  private:
    IOResult load_image_sampling_settings(ILoad* iload);
    IOResult load_lighting_settings(ILoad* iload);
    IOResult load_pathtracer_settings(ILoad* iload);
    IOResult load_sppm_settings(ILoad* iload);
    IOResult load_output_settings(ILoad* iload);
    IOResult load_system_settings(ILoad* iload);
    IOResult load_postprocessing_settings(ILoad* iload);

    void apply_common_settings(renderer::Project& project, const char* config_name) const;
    void apply_settings_to_final_config(renderer::Project& project) const;
    void apply_settings_to_interactive_config(renderer::Project& project) const;
};
