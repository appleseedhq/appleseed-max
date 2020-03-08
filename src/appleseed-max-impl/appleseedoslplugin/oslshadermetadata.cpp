
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
#include "oslshadermetadata.h"

// appleseed-max headers.
#include "utilities.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"

// appleseed.foundation headers.
#include "foundation/containers/dictionary.h"
#include "foundation/memory/autoreleaseptr.h"
#include "foundation/utility/iostreamop.h"
#include "foundation/utility/searchpaths.h"
#include "foundation/string/string.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <iparamb2.h>
#include <maxtypes.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Standard headers.
#include <iostream>
#include <memory>
#include <vector>

namespace asf = foundation;
namespace asr = renderer;

OSLMetadataExtractor::OSLMetadataExtractor(const foundation::Dictionary& metadata)
  : m_metadata(metadata)
{
}

bool OSLMetadataExtractor::exists(const char* key) const
{
    return m_metadata.dictionaries().exist(key);
}

bool OSLMetadataExtractor::get_value(const char* key, std::string& value) const
{
    if (exists(key))
    {
        const foundation::Dictionary& dict = m_metadata.dictionary(key);
        value = dict.get("value");
        return true;
    }

    return false;
}

bool OSLMetadataExtractor::get_value(const char* key, bool& value) const
{
    int tmp;
    if (get_value(key, tmp))
    {
        value = (tmp != 0);
        return true;
    }

    return false;
}

namespace
{
    void get_float3_default(const asf::Dictionary& param_info, std::vector<double>& default_value)
    {
        const asf::Vector3f v = param_info.get<asf::Vector3f>("default");
        default_value.push_back(v[0]);
        default_value.push_back(v[1]);
        default_value.push_back(v[2]);
    }
}

OSLParamInfo::OSLParamInfo(const asf::Dictionary& param_info)
  : m_has_default(false)
  , m_divider(false)
  , m_lock_geom(true)
  , m_is_deprecated(false)
{
    m_param_name = param_info.get("name");
    m_param_type = param_info.get("type");
    m_valid_default = param_info.get<bool>("validdefault");
    m_is_connectable = true;

    // todo: lots of refactoring possibilities here...
    if (m_valid_default)
    {
        if (param_info.strings().exist("default"))
        {
            if (m_param_type == "color")
            {
                get_float3_default(param_info, m_default_value);
                m_has_default = true;
            }
            else if (m_param_type == "float")
            {
                m_default_value.push_back(param_info.get<float>("default"));
                m_has_default = true;
            }
            else if (m_param_type == "int")
            {
                m_default_value.push_back(param_info.get<int>("default"));
                m_has_default = true;
            }
            if (m_param_type == "normal")
            {
                get_float3_default(param_info, m_default_value);
                m_has_default = true;
            }
            if (m_param_type == "point")
            {
                get_float3_default(param_info, m_default_value);
                m_has_default = true;
            }
            else if (m_param_type == "string")
            {
                m_default_string_value = param_info.get("default");
                m_has_default = true;
            }
            else if (m_param_type == "vector")
            {
                get_float3_default(param_info, m_default_value);
                m_has_default = true;
            }
        }
    }

    m_is_output = param_info.get<bool>("isoutput");
    m_is_closure = param_info.get<bool>("isclosure");

    if (param_info.dictionaries().exist("metadata"))
    {
        OSLMetadataExtractor metadata(param_info.dictionary("metadata"));

        metadata.get_value("lockgeom", m_lock_geom);
        metadata.get_value("units", m_units);
        metadata.get_value("page", m_page);
        metadata.get_value("label", m_label);
        metadata.get_value("widget", m_widget);
        metadata.get_value("options", m_options);
        metadata.get_value("help", m_help);
        m_has_min = metadata.get_value("min", m_min_value);
        m_has_max = metadata.get_value("max", m_max_value);
        m_has_soft_min = metadata.get_value("softmin", m_soft_min_value);
        m_has_soft_max = metadata.get_value("softmax", m_soft_max_value);
        metadata.get_value("divider", m_divider);

        if (!metadata.get_value("as_max_param_id", m_max_param_id))
            m_max_param_id = -1;

        metadata.get_value("as_maya_attribute_name", m_maya_attribute_name);
        metadata.get_value("as_maya_attribute_connectable", m_is_connectable);
        m_max_hidden_attr = false;
        metadata.get_value("as_max_attribute_hidden", m_max_hidden_attr);

        metadata.get_value("as_deprecated", m_is_deprecated);
    }
}

std::ostream& operator<<(std::ostream& os, const OSLParamInfo& param_info)
{
    os << "Param : " << param_info.m_param_name << "\n";
    os << "  label name     : " << param_info.m_label << "\n";
    os << "  type           : " << param_info.m_param_type << "\n";
    os << "  output         : " << param_info.m_is_output << "\n";
    os << "  valid default  : " << param_info.m_valid_default << "\n";
    os << "  closure        : " << param_info.m_is_closure << "\n";
    os << std::endl;
    return os;
}

std::istream& operator>>(std::istream& s, Class_ID& class_id)
{
    int a;
    int b;
    s >> a;
    s >> b;
    class_id = Class_ID(a, b);
    return s;
}

OSLShaderInfo::OSLShaderInfo()
{
}

OSLShaderInfo::OSLShaderInfo(
    const asr::ShaderQuery&         q,
    const std::string               filename)
{
    m_is_texture = true;
    m_string_map.clear();
    m_texture_id_map.clear();

    m_shader_name = q.get_shader_name();
    OSLMetadataExtractor metadata(q.get_metadata());

    if (metadata.exists("as_max_class_id"))
    {
        std::string max_shader_name;
        metadata.get_value("as_node_name", max_shader_name);
        m_max_shader_name = utf8_to_wide(max_shader_name);

        metadata.get_value("as_max_class_id", m_class_id);

        std::string plugin_type;
        metadata.get_value("as_max_plugin_type", plugin_type);
        m_is_texture = plugin_type == "texture";

        m_params.reserve(q.get_param_count());
        for (size_t i = 0, e = q.get_param_count(); i < e; ++i)
        {
            OSLParamInfo osl_param(q.get_param_info(i));

            if (osl_param.m_widget == "null" && 
                !osl_param.m_is_connectable && 
                !osl_param.m_is_output)
                continue;

            MaxParam& max_param = osl_param.m_max_param;

            max_param.m_osl_param_name = osl_param.m_param_name;
            max_param.m_max_label_str = osl_param.m_label;
            max_param.m_is_connectable = osl_param.m_is_connectable;
            max_param.m_param_type = MaxParam::Unsupported;
            max_param.m_page_name = osl_param.m_page;

            max_param.m_is_constant =
                osl_param.m_valid_default &&
                osl_param.m_lock_geom &&
                osl_param.m_widget != "null";

            if (osl_param.m_param_type == "color")
                max_param.m_param_type = MaxParam::Color;
            else if (osl_param.m_param_type == "float")
                max_param.m_param_type = MaxParam::Float;
            else if (osl_param.m_param_type == "float[2]")
                max_param.m_param_type = MaxParam::Float2f;
            else if (osl_param.m_param_type == "matrix")
                max_param.m_param_type = MaxParam::Matrix;
            else if (osl_param.m_param_type == "pointer")
                max_param.m_param_type = MaxParam::Closure;
            else if (osl_param.m_param_type == "int")
            {
                if (osl_param.m_widget == "mapper")
                    max_param.m_param_type = MaxParam::IntMapper;
                else if (osl_param.m_widget == "checkBox")
                    max_param.m_param_type = MaxParam::IntCheckbox;
                else
                    max_param.m_param_type = MaxParam::IntNumber;
            }
            else if (osl_param.m_param_type == "point")
                max_param.m_param_type = MaxParam::PointParam;
            else if (osl_param.m_param_type == "vector")
                max_param.m_param_type = MaxParam::VectorParam;
            else if (osl_param.m_param_type == "normal")
                max_param.m_param_type = MaxParam::NormalParam;
            else if (osl_param.m_param_type == "string")
            {
                if (osl_param.m_widget == "popup")
                    max_param.m_param_type = MaxParam::StringPopup;
                else
                    max_param.m_param_type = MaxParam::String;
            }

            if (max_param.m_param_type != MaxParam::Unsupported)
            {
                if (osl_param.m_is_output)
                    m_output_params.push_back(osl_param);
                else
                    m_params.push_back(osl_param);
            }
        }
    }
}

const OSLParamInfo* OSLShaderInfo::find_param(const char* param_name) const
{
    for (size_t i = 0, e = m_params.size(); i < e; ++i)
    {
        if (m_params[i].m_param_name == param_name)
            return &m_params[i];
    }

    return nullptr;
}

const OSLParamInfo* OSLShaderInfo::find_maya_attribute(const char* attr_name) const
{
    for (size_t i = 0, e = m_params.size(); i < e; ++i)
    {
        if (m_params[i].m_maya_attribute_name == attr_name)
            return &m_params[i];
    }

    return nullptr;
}
