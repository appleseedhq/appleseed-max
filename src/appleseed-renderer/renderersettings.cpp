
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015 Francois Beaune, The appleseedhq Organization
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

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/utility.h"

namespace asr = renderer;

namespace
{
    struct DefaultRendererSettings
      : public RendererSettings
    {
        DefaultRendererSettings()
        {
            m_pixel_samples = 16;
            m_passes = 4;
            m_output_mode = OutputModeRenderOnly;
            m_rendering_threads = 0;    // 0 = as many as there are logical cores
        }
    };
}

const RendererSettings& RendererSettings::defaults()
{
    static DefaultRendererSettings default_settings;
    return default_settings;
}

void RendererSettings::apply(
    asr::Project&   project,
    const char*     config_name)
{
    asr::ParamArray& params = project.configurations().get_by_name(config_name)->get_parameters();

    params.insert_path("generic_frame_renderer.passes", m_passes);
    params.insert_path("shading_result_framebuffer", m_passes == 1 ? "ephemeral" : "permanent");
    params.insert_path("uniform_pixel_renderer.samples", m_pixel_samples);
    params.insert_path("rendering_threads", m_rendering_threads);

    //params.insert_path("shading_engine.override_shading.mode", "shading_normal");
}
