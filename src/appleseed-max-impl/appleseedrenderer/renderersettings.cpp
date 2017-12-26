
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
            m_pixel_samples = 16;
            m_passes = 1;
            m_tile_size = 64;

            m_gi = true;
            m_caustics = false;
            m_bounces = 8;
            m_max_ray_intensity_set = false;
            m_max_ray_intensity = 1.0f;
            m_background_emits_light = true;
            m_background_alpha = 0.0f;

            m_output_mode = OutputMode::RenderOnly;
            m_scale_multiplier = 1.0f;

            m_rendering_threads = 0;    // 0 = as many as there are logical cores
            m_low_priority_mode = true;
            m_use_max_procedural_maps = false;

            const int log_mode = load_system_setting(L"LogOpenMode", static_cast<int>(DialogLogMode::Always));
            m_log_open_mode = static_cast<DialogLogMode>(log_mode);
            m_log_in_material_editor = load_system_setting(L"LogMaterialEditor", false);
        }
    };
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

    if (!m_gi)
        params.insert_path("pt.max_diffuse_bounces", 0);

    params.insert_path("pt.max_bounces", m_bounces);

    params.insert_path("pt.enable_ibl", m_background_emits_light);
    params.insert_path("pt.enable_caustics", m_caustics);

    if (m_max_ray_intensity_set)
        params.insert_path("pt.max_ray_intensity", m_max_ray_intensity);

    if (m_rendering_threads == 0)
        params.insert_path("rendering_threads", "auto");
    else params.insert_path("rendering_threads", m_rendering_threads);

    //params.insert_path("shading_engine.override_shading.mode", "shading_normal");
}

void RendererSettings::apply_settings_to_final_config(asr::Project& project) const
{
    asr::ParamArray& params = project.configurations().get_by_name("final")->get_parameters();

    params.insert_path("generic_frame_renderer.tile_ordering", "spiral");
    params.insert_path("generic_frame_renderer.passes", m_passes);
    params.insert_path("shading_result_framebuffer", m_passes == 1 ? "ephemeral" : "permanent");

    params.insert_path("uniform_pixel_renderer.samples", m_pixel_samples);
    if (m_pixel_samples == 1)
        params.insert_path("uniform_pixel_renderer.force_antialiasing", true);
}

void RendererSettings::apply_settings_to_interactive_config(asr::Project& project) const
{
    asr::ParamArray& params = project.configurations().get_by_name("interactive")->get_parameters();

    params.insert_path("frame_renderer", "progressive");
    params.insert_path("sample_generator", "generic");
    params.insert_path("sample_renderer", "generic");

}

bool RendererSettings::save(ISave* isave) const
{
    bool success = true;

    //
    // Image Sampling settings.
    //

    isave->BeginChunk(ChunkSettingsImageSampling);

        isave->BeginChunk(ChunkSettingsImageSamplingPixelSamples);
        success &= write<int>(isave, m_pixel_samples);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsImageSamplingPasses);
        success &= write<int>(isave, m_passes);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsImageSamplingTileSize);
        success &= write<int>(isave, m_tile_size);
        isave->EndChunk();

    isave->EndChunk();

    //
    // Lighting settings.
    //

    isave->BeginChunk(ChunkSettingsLighting);

        isave->BeginChunk(ChunkSettingsLightingGI);
        success &= write<bool>(isave, m_gi);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightingCaustics);
        success &= write<bool>(isave, m_caustics);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightingBounces);
        success &= write<int>(isave, m_bounces);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightingMaxRayIntensitySet);
        success &= write<bool>(isave, m_max_ray_intensity_set);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightingMaxRayIntensity);
        success &= write<float>(isave, m_max_ray_intensity);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightingBackgroundEmitsLight);
        success &= write<bool>(isave, m_background_emits_light);
        isave->EndChunk();

        isave->BeginChunk(ChunkSettingsLightingBackgroundAlpha);
        success &= write<float>(isave, m_background_alpha);
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

          case ChunkSettingsOutput:
            result = load_output_settings(iload);
            break;

          case ChunkSettingsSystem:
            result = load_system_settings(iload);
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
            result = read<int>(iload, &m_pixel_samples);
            break;

          case ChunkSettingsImageSamplingPasses:
            result = read<int>(iload, &m_passes);
            break;

          case ChunkSettingsImageSamplingTileSize:
            result = read<int>(iload, &m_tile_size);
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
          case ChunkSettingsLightingGI:
            result = read<bool>(iload, &m_gi);
            break;

          case ChunkSettingsLightingCaustics:
            result = read<bool>(iload, &m_caustics);
            break;

          case ChunkSettingsLightingBounces:
            result = read<int>(iload, &m_bounces);
            break;

          case ChunkSettingsLightingMaxRayIntensitySet:
            result = read<bool>(iload, &m_max_ray_intensity_set);
            break;

          case ChunkSettingsLightingMaxRayIntensity:
            result = read<float>(iload, &m_max_ray_intensity);
            break;

          case ChunkSettingsLightingBackgroundEmitsLight:
            result = read<bool>(iload, &m_background_emits_light);
            break;

          case ChunkSettingsLightingBackgroundAlpha:
            result = read<float>(iload, &m_background_alpha);
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
        }

        if (result != IO_OK)
            break;

        result = iload->CloseChunk();
        if (result != IO_OK)
            break;
    }

    return result;
}
