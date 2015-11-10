
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

// appleseed-max headers.
#include "appleseedrenderer.h"

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"
#include "foundation/platform/windows.h"

// 3ds Max headers.
#include <plugapi.h>

// Windows headers.
#include <tchar.h>

namespace
{
    //
    // AppleseedRenderer class descriptor.
    //

    struct AppleseedRendererClassDesc
      : public ClassDesc
    {
        virtual int IsPublic() APPLESEED_OVERRIDE
        {
            return TRUE;
        }

        virtual void* Create(BOOL loading) APPLESEED_OVERRIDE
        {
            return new AppleseedRenderer();
        }

        virtual const TCHAR* ClassName() APPLESEED_OVERRIDE
        {
            return _T("appleseed Renderer");
        }

        virtual SClass_ID SuperClassID() APPLESEED_OVERRIDE
        {
            return RENDERER_CLASS_ID;
        }

        virtual Class_ID ClassID() APPLESEED_OVERRIDE
        {
            return Class_ID(0x6170706c, 0x73656564);    // appl seed
        }

        virtual const TCHAR* Category() APPLESEED_OVERRIDE
        {
            return _T("");
        }
    };

    static AppleseedRendererClassDesc g_appleseed_renderer_classdesc;
}

extern "C"
{
    __declspec(dllexport)
    const TCHAR* LibDescription()
    {
        return _T("appleseed Plugin");
    }

    __declspec(dllexport)
    int LibNumberClasses()
    {
        return 1;
    }

    __declspec(dllexport)
    ClassDesc* LibClassDesc(int i)
    {
        switch (i)
        {
          case 0: return &g_appleseed_renderer_classdesc;
          // Make sure to update LibNumberClasses() if you add classes.
          default: return 0;
        }
    }

    __declspec(dllexport)
    ULONG LibVersion()
    {
        return VERSION_3DSMAX; 
    }
}
