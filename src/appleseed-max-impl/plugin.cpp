
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

// appleseed-max headers.
#include "appleseedblendmtl/appleseedblendmtl.h"
#include "appleseedenvmap/appleseedenvmap.h"
#include "appleseeddisneymtl/appleseeddisneymtl.h"
#include "appleseedglassmtl/appleseedglassmtl.h"
#include "appleseedlightmtl/appleseedlightmtl.h"
#include "appleseedmetalmtl/appleseedmetalmtl.h"
#include "appleseedobjpropsmod/appleseedobjpropsmod.h"
#include "appleseedoslplugin/oslshaderregistry.h"
#include "appleseedplasticmtl/appleseedplasticmtl.h"
#include "appleseedrenderelement/appleseedrenderelement.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "appleseedsssmtl/appleseedsssmtl.h"
#include "logtarget.h"
#include "main.h"
#include "osloutputselectormap/osloutputselector.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/log.h"

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// appleseed.main headers.
#include "main/allocator.h"

// 3ds Max headers.
#include <iparamb2.h>
#include <plugapi.h>

// Windows headers.
#include <tchar.h>

// Standard headers.
#include <sstream>
#include <string>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    LogTarget g_log_target;
    OSLShaderRegistry g_shader_registry;
}

extern "C"
{
    __declspec(dllexport)
    const wchar_t* LibDescription()
    {
        return L"appleseed Renderer";
    }

    __declspec(dllexport)
    int LibNumberClasses()
    {
        return 12 + g_shader_registry.get_size();
    }

    __declspec(dllexport)
    ClassDesc2* LibClassDesc(int i)
    {
        switch (i)
        {
          case 0: return &g_appleseed_renderer_classdesc;
          case 1: return &g_appleseed_disneymtl_classdesc;
          case 2: return &g_appleseed_sssmtl_classdesc;
          case 3: return &g_appleseed_glassmtl_classdesc;
          case 4: return &g_appleseed_lightmtl_classdesc;
          case 5: return &g_appleseed_objpropsmod_classdesc;
          case 6: return &g_appleseed_envmap_classdesc;
          case 7: return &g_appleseed_blendmtl_classdesc;
          case 8: return &g_appleseed_metalmtl_classdesc;
          case 9: return &g_appleseed_plasticmtl_classdesc;
          case 10: return &g_appleseed_outputselector_classdesc;
          case 11: return &g_appleseed_renderelement_classdesc;

          // Make sure to update LibNumberClasses() if you add classes here.

          default:
            return g_shader_registry.get_class_descriptor(i - 11);
        }
    }

    __declspec(dllexport)
    ULONG LibVersion()
    {
        return VERSION_3DSMAX;
    }

    __declspec(dllexport)
    int LibInitialize()
    {
        start_memory_tracking();

        asr::global_logger().add_target(&g_log_target);

        std::wstringstream sstr;
        sstr << "appleseed-max ";
        sstr << PluginVersionString;
        sstr << " plug-in using ";
        sstr << utf8_to_wide(asf::Appleseed::get_synthetic_version_string());
        sstr << " loaded";

        const auto title = sstr.str();
        const std::wstring sep(title.size(), L'=');

        // We use GetCOREInterface()->Log()->LogEntry() directly rather than RENDERER_LOG_INFO()
        // because it seems that we are not running on the main thread (hence log messages are
        // enqueued to later be emitted from the main thread rather than being emitted directly)
        // but the Win32 message that should trigger message emission seems to get lost.

        GetCOREInterface()->Log()->LogEntry(
            SYSLOG_INFO,
            FALSE,
            L"appleseed",
            L"[appleseed] %s",
            sep.c_str());

        GetCOREInterface()->Log()->LogEntry(
            SYSLOG_INFO,
            FALSE,
            L"appleseed",
            L"[appleseed] %s",
            title.c_str());

        GetCOREInterface()->Log()->LogEntry(
            SYSLOG_INFO,
            FALSE,
            L"appleseed",
            L"[appleseed] %s",
            sep.c_str());

        g_shader_registry.create_class_descriptors();

        return TRUE;
    }
}
