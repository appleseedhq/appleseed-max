
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2018 Sergo Pogosyan, The appleseedhq Organization
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

// Build options header.
#include "renderer/api/buildoptions.h"

// appleseed-max headers.
#include "oslshaderregistry.h"

// 3ds Max headers.
#include "_beginmaxheaders.h"
#include <IMaterialBrowserEntryInfo.h>
#include <IMtlRender_Compatibility.h>
#include "_endmaxheaders.h"

//
// OSLPlugin material browser info.
//

class OSLPluginBrowserEntryInfo
    : public IMaterialBrowserEntryInfo
{
  public:
    explicit OSLPluginBrowserEntryInfo(bool is_texture);
    const MCHAR* GetEntryName() const override;
    const MCHAR* GetEntryCategory() const override;
    Bitmap* GetEntryThumbnail() const override;

  private:
    bool m_is_texture;
};


//
// OSLPlugin class descriptor.
//

class OSLPluginClassDesc
  : public ClassDesc2
  , public IMtlRender_Compatibility_MtlBase
{
  public:
    explicit OSLPluginClassDesc(OSLShaderInfo* shader_info);
    int IsPublic() override;
    void* Create(BOOL loading) override;
    const wchar_t* ClassName() override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    const wchar_t* Category() override;
    const wchar_t* InternalName() override;
    FPInterface* GetInterface(Interface_ID id) override;
    HINSTANCE HInstance() override;

    const MCHAR* GetRsrcString(INT_PTR id) override;

    // IMtlRender_Compatibility_MtlBase methods.
    bool IsCompatibleWithRenderer(ClassDesc& renderer_class_desc) override;

    OSLShaderInfo*                      m_shader_info;

  private:
    Class_ID                            m_class_id;
    OSLPluginBrowserEntryInfo   m_browser_entry_info;
};
