
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

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <maxtypes.h>
#include <strclass.h>

// Standard headers.
#include <string>

// Forward declarations.
namespace renderer  { class Project; }
class ILoad;
class ISave;

class RendererSettings
{
  public:
    static const RendererSettings& defaults();

    //
    // Image Sampling.
    //

    int         m_uniform_pixel_samples;
    int         m_passes;
    int         m_tile_size;
    int         m_sampler_type;

    //
    // Adaptive Tile Sampler.
    //

    int         m_adaptive_batch_size;
    int         m_adaptive_min_samples;
    int         m_adaptive_max_samples;
    float       m_adaptive_noise_threshold;

    //
    // Pixel Filtering.
    //

    int         m_pixel_filter;
    float       m_pixel_filter_size;

    //
    // Lighting.
    //

    bool        m_enable_gi;
    bool        m_enable_caustics;
    int         m_global_bounces;
    int         m_diffuse_bounces;
    bool        m_diffuse_bounces_enabled;
    int         m_glossy_bounces;
    bool        m_glossy_bounces_enabled;
    int         m_specular_bounces;
    bool        m_specular_bounces_enabled;
    int         m_volume_bounces;
    bool        m_volume_bounces_enabled;
    int         m_volume_distance_samples;
    bool        m_clamp_roughness;
    bool        m_max_ray_intensity_set;
    float       m_max_ray_intensity;
    bool        m_background_emits_light;
    float       m_background_alpha;
    bool        m_force_off_default_lights;
    bool        m_dl_enable_dl;
    int         m_dl_light_samples;
    float       m_dl_low_light_threshold;
    int         m_ibl_env_samples;
    int         m_rr_min_path_length;
    bool        m_optimize_for_lights_outside_volumes;

    //
    // Output.
    //

    enum class OutputMode
    {
        RenderOnly,
        SaveProjectOnly,
        SaveProjectAndRender
    };

    OutputMode  m_output_mode;
    MSTR        m_project_file_path;
    float       m_scale_multiplier;

    //
    // System.
    //

    int                         m_rendering_threads;
    bool                        m_enable_embree;
    bool                        m_low_priority_mode;
    bool                        m_use_max_procedural_maps;
    DialogLogTarget::OpenMode   m_log_open_mode;
    bool                        m_log_material_editor_messages;
    bool                        m_enable_render_stamp;
    MSTR                        m_render_stamp_format;

    // Apply these settings to a given project.
    void apply(renderer::Project& project) const;

    // Save settings to a 3ds Max file.
    bool save(ISave* isave) const;

    // Load settings from a 3ds Max file.
    IOResult load(ILoad* iload);

  private:
    IOResult load_image_sampling_settings(ILoad* iload);
    IOResult load_lighting_settings(ILoad* iload);
    IOResult load_output_settings(ILoad* iload);
    IOResult load_system_settings(ILoad* iload);

    void apply_common_settings(renderer::Project& project, const char* config_name) const;
    void apply_settings_to_final_config(renderer::Project& project) const;
    void apply_settings_to_interactive_config(renderer::Project& project) const;
};
