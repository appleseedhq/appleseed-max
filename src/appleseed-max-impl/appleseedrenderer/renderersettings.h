
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
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

    int         m_pixel_samples;
    int         m_passes;
    int         m_tile_size;

    //
    // Lighting.
    //

    bool        m_gi;
    bool        m_caustics;
    int         m_bounces;
    bool        m_max_ray_intensity_set;
    float       m_max_ray_intensity;
    bool        m_background_emits_light;
    float       m_background_alpha;

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

    int         m_rendering_threads;
    bool        m_low_priority_mode;
    bool        m_use_max_procedural_maps;

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
