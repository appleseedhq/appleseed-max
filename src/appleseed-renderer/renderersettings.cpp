
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

// appleseed-max headers.
#include "datachunks.h"
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

bool RendererSettings::save(ISave* isave) const
{
    bool success = true;

    //
    // Image Sampling settings.
    //

    isave->BeginChunk(CHUNK_SETTINGS_IMAGESAMPLING);

        isave->BeginChunk(CHUNK_SETTINGS_IMAGESAMPLING_PIXEL_SAMPLES);
        success &= write<int>(isave, m_pixel_samples);
        isave->EndChunk();

        isave->BeginChunk(CHUNK_SETTINGS_IMAGESAMPLING_PASSES);
        success &= write<int>(isave, m_passes);
        isave->EndChunk();

    isave->EndChunk();

    //
    // Output settings.
    //

    isave->BeginChunk(CHUNK_SETTINGS_OUTPUT);

        isave->BeginChunk(CHUNK_SETTINGS_OUTPUT_MODE);
        switch (m_output_mode)
        {
          case OutputModeRenderOnly:
            success &= write<USHORT>(isave, 0x0000);
            break;
          case OutputModeSaveProjectOnly:
            success &= write<USHORT>(isave, 0x0001);
            break;
          case OutputModeSaveProjectAndRender:
            success &= write<USHORT>(isave, 0x0002);
            break;
        }
        isave->EndChunk();

        isave->BeginChunk(CHUNK_SETTINGS_OUTPUT_PROJECT_FILE_PATH);
        success &= write(isave, m_project_file_path);
        isave->EndChunk();

    isave->EndChunk();

    //
    // System settings.
    //

    isave->BeginChunk(CHUNK_SETTINGS_SYSTEM);

        isave->BeginChunk(CHUNK_SETTINGS_SYSTEM_RENDERING_THREADS);
        success &= write<int>(isave, m_rendering_threads);
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
          case CHUNK_SETTINGS_IMAGESAMPLING:
            result = load_image_sampling_settings(iload);
            break;

          case CHUNK_SETTINGS_OUTPUT:
            result = load_output_settings(iload);
            break;

          case CHUNK_SETTINGS_SYSTEM:
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
          case CHUNK_SETTINGS_IMAGESAMPLING_PIXEL_SAMPLES:
            result = read<int>(iload, &m_pixel_samples);
            break;

          case CHUNK_SETTINGS_IMAGESAMPLING_PASSES:
            result = read<int>(iload, &m_passes);
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
          case CHUNK_SETTINGS_OUTPUT_MODE:
            {
                USHORT mode;
                result = read<USHORT>(iload, &mode);
                if (result == IO_OK)
                {
                    switch (mode)
                    {
                      case 0x0000:
                        m_output_mode = OutputModeRenderOnly;
                        break;
                      case 0x0001:
                        m_output_mode = OutputModeSaveProjectOnly;
                        break;
                      case 0x0002:
                        m_output_mode = OutputModeSaveProjectAndRender;
                        break;
                      default:
                        result = IO_ERROR;
                        break;
                    }
                }
            }
            break;

          case CHUNK_SETTINGS_OUTPUT_PROJECT_FILE_PATH:
            result = read(iload, &m_project_file_path);
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
          case CHUNK_SETTINGS_SYSTEM_RENDERING_THREADS:
            result = read<int>(iload, &m_rendering_threads);
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
