
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
#include "main.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <iparamb2.h>
#include <plugapi.h>

// Windows headers.
#include <Pathcch.h>
#include <tchar.h>

namespace
{
    HINSTANCE g_plugin_lib;

    HINSTANCE load_relative_library(const wchar_t* filename)
    {
        wchar_t path[MAX_PATH];
        const DWORD path_length = sizeof(path) / sizeof(wchar_t);

        if (!GetModuleFileName(g_module, path, path_length))
            return false;

        PathCchRemoveFileSpec(path, path_length);
        PathCchAppend(path, path_length, filename);

        return LoadLibrary(path);
    }
}

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
        typedef int (*LibNumberClassesFunc)();
        auto RealLibNumberClasses =
            reinterpret_cast<LibNumberClassesFunc>(GetProcAddress(g_plugin_lib, "LibNumberClasses"));
        return RealLibNumberClasses();
    }

    __declspec(dllexport)
    ClassDesc2* LibClassDesc(int i)
    {
        typedef ClassDesc2* (*LibClassDescFunc)(int i);
        auto RealLibClassDesc =
            reinterpret_cast<LibClassDescFunc>(GetProcAddress(g_plugin_lib, "LibClassDesc"));
        return RealLibClassDesc(i);
    }

    __declspec(dllexport)
    ULONG LibVersion()
    {
        return VERSION_3DSMAX;
    }

    __declspec(dllexport)
    int LibInitialize()
    {
        if (load_relative_library(_T("appleseed.dll")) == nullptr)
            return FALSE;

        if ((g_plugin_lib = load_relative_library(_T("appleseed-plugin-impl.dll"))) == nullptr)
            return FALSE;

        return TRUE;
    }
}
