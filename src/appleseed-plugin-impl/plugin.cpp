
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2016 Francois Beaune, The appleseedhq Organization
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
#include "disneymtl/appleseeddisneymtl.h"
#include "glassmtl/appleseedglassmtl.h"
#include "renderer/appleseedrenderer.h"
#include "sssmtl/appleseedsssmtl.h"
#include "main.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// appleseed.main headers.
#include "main/allocator.h"

// 3ds Max headers.
#include <iparamb2.h>
#include <plugapi.h>

// Windows headers.
#include <tchar.h>

extern "C"
{
    __declspec(dllexport)
    const TCHAR* LibDescription()
    {
        return _T("appleseed Renderer");
    }

    __declspec(dllexport)
    int LibNumberClasses()
    {
        return 4;
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

        return TRUE;
    }
}
