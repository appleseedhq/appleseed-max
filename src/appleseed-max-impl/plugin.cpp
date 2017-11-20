
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
#include "appleseedenvmap/appleseedenvmap.h"
#include "appleseedblendmtl/appleseedblendmtl.h"
#include "appleseeddisneymtl/appleseeddisneymtl.h"
#include "appleseedglassmtl/appleseedglassmtl.h"
#include "appleseedlightmtl/appleseedlightmtl.h"
#include "appleseedobjpropsmod/appleseedobjpropsmod.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "appleseedsssmtl/appleseedsssmtl.h"
#include "logtarget.h"
#include "main.h"
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
        return 8;
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

          // Make sure to update LibNumberClasses() if you add classes here.

          default:
            return nullptr;
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

        std::stringstream sstr;
        sstr << "appleseed-max ";
        sstr << wide_to_utf8(PluginVersionString);
        sstr << " plug-in for ";
        sstr << asf::Appleseed::get_synthetic_version_string();
        sstr << " loaded";

        const std::string title = sstr.str();
        const std::string sep(title.size(), '=');

        RENDERER_LOG_INFO("%s", sep.c_str());
        RENDERER_LOG_INFO("%s", title.c_str());
        RENDERER_LOG_INFO("%s", sep.c_str());

        return TRUE;
    }
}
