
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

// Interface header.
#include "oslclassdesc.h"

// appleseed-max headers.
#include "appleseedoslplugin/oslmaterial.h"
#include "appleseedoslplugin/osltexture.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "main.h"

//
// OSLPluginBrowserEntryInfo class implementation.
//

OSLPluginBrowserEntryInfo::OSLPluginBrowserEntryInfo(bool is_texture)
  : m_is_texture(is_texture)
{
}

const MCHAR* OSLPluginBrowserEntryInfo::GetEntryName() const
{
    return L"";
}

const MCHAR* OSLPluginBrowserEntryInfo::GetEntryCategory() const
{
    return m_is_texture ? L"Maps\\appleseed OSL" : L"Materials\\appleseed OSL";
}

Bitmap* OSLPluginBrowserEntryInfo::GetEntryThumbnail() const
{
    return nullptr;
}

//
// OSLPluginClassDesc class implementation.
//

OSLPluginClassDesc::OSLPluginClassDesc(OSLShaderInfo* shader_info)
  : m_class_id(shader_info->m_class_id)
  , m_shader_info(shader_info)
  , m_browser_entry_info(shader_info->m_is_texture)
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int OSLPluginClassDesc::IsPublic()
{
    return TRUE;
}

void* OSLPluginClassDesc::Create(BOOL loading)
{
    if (m_shader_info->m_is_texture)
        return new OSLTexture(m_shader_info->m_class_id, this);
    else
        return new OSLMaterial(m_shader_info->m_class_id, this);
}

const MCHAR* OSLPluginClassDesc::ClassName()
{
    return m_shader_info->m_max_shader_name.c_str();;
}

SClass_ID OSLPluginClassDesc::SuperClassID()
{
    return m_shader_info->m_is_texture ? TEXMAP_CLASS_ID : MATERIAL_CLASS_ID;
}

Class_ID OSLPluginClassDesc::ClassID()
{
    return m_class_id;
}

const MCHAR* OSLPluginClassDesc::Category()
{
    return L"";
}

const MCHAR* OSLPluginClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return m_shader_info->m_max_shader_name.c_str();
}

FPInterface* OSLPluginClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE OSLPluginClassDesc::HInstance()
{
    return g_module;
}

const MCHAR* OSLPluginClassDesc::GetRsrcString(INT_PTR id)
{
    const auto it = m_shader_info->m_string_map.find(static_cast<int>(id));

    if (it != m_shader_info->m_string_map.end())
    {
        return it->second.c_str();
    }
    else
        return ClassDesc2::GetRsrcString(id);
}

bool OSLPluginClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
