
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
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

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/iostreamop.h"
#include "foundation/utility/searchpaths.h"
#include "foundation/utility/string.h"

// appleseed-max headers.
#include "utilities.h"

// 3ds Max headers.
#include <iparamb2.h>
#include <maxtypes.h>

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
    void get_float3_default(const asf::Dictionary& paramInfo, std::vector<double>& defaultValue)
    {
        const asf::Vector3f v = paramInfo.get<asf::Vector3f>("default");
        defaultValue.push_back(v[0]);
        defaultValue.push_back(v[1]);
        defaultValue.push_back(v[2]);
    }
}

OSLParamInfo::OSLParamInfo(const asf::Dictionary& paramInfo)
  : hasDefault(false)
  , divider(false)
{
    paramName = paramInfo.get("name");
    paramType = paramInfo.get("type");
    validDefault = paramInfo.get<bool>("validdefault");
    connectable = true;

    // todo: lots of refactoring possibilities here...
    if (validDefault)
    {
        if (paramInfo.strings().exist("default"))
        {
            if (paramType == "color")
            {
                get_float3_default(paramInfo, defaultValue);
                hasDefault = true;
            }
            else if (paramType == "float")
            {
                defaultValue.push_back(paramInfo.get<float>("default"));
                hasDefault = true;
            }
            else if (paramType == "int")
            {
                defaultValue.push_back(paramInfo.get<int>("default"));
                hasDefault = true;
            }
            if (paramType == "normal")
            {
                get_float3_default(paramInfo, defaultValue);
                hasDefault = true;
            }
            if (paramType == "point")
            {
                get_float3_default(paramInfo, defaultValue);
                hasDefault = true;
            }
            else if (paramType == "string")
            {
                defaultStringValue = paramInfo.get("default");
                hasDefault = true;
            }
            else if (paramType == "vector")
            {
                get_float3_default(paramInfo, defaultValue);
                hasDefault = true;
            }
        }
    }

    isOutput = paramInfo.get<bool>("isoutput");
    isClosure = paramInfo.get<bool>("isclosure");

    if (paramInfo.dictionaries().exist("metadata"))
    {
        OSLMetadataExtractor metadata(paramInfo.dictionary("metadata"));

        metadata.get_value("units", units);
        metadata.get_value("page", page);
        metadata.get_value("label", label);
        metadata.get_value("widget", widget);
        metadata.get_value("options", options);
        metadata.get_value("help", help);
        hasMin = metadata.get_value("min", minValue);
        hasMax = metadata.get_value("max", maxValue);
        hasSoftMin = metadata.get_value("softmin", softMinValue);
        hasSoftMax = metadata.get_value("softmax", softMaxValue);
        metadata.get_value("divider", divider);

        metadata.get_value("as_maya_attribute_connectable", connectable);
    }
}

std::ostream& operator<<(std::ostream& os, const OSLParamInfo& paramInfo)
{
    os << "Param : " << paramInfo.paramName << "\n";
    os << "  label name     : " << paramInfo.label << "\n";
    os << "  type           : " << paramInfo.paramType << "\n";
    os << "  output         : " << paramInfo.isOutput << "\n";
    os << "  valid default  : " << paramInfo.validDefault << "\n";
    os << "  closure        : " << paramInfo.isClosure << "\n";
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
        metadata.get_value("as_maya_node_name", max_shader_name);
        m_max_shader_name = utf8_to_wide(max_shader_name);
        
        metadata.get_value("as_max_class_id", m_class_id);
        
        std::string plugin_type;
        metadata.get_value("as_max_plugin_type", plugin_type);
        m_is_texture = plugin_type == "texture";

        m_params.reserve(q.get_param_count());
        for (size_t i = 0, e = q.get_param_count(); i < e; ++i)
        {
            OSLParamInfo osl_param(q.get_param_info(i));
            
            MaxParam& max_param = osl_param.m_max_param;

            max_param.m_osl_param_name = osl_param.paramName;
            max_param.m_max_label_str = osl_param.label;
            max_param.m_connectable = osl_param.connectable;
            max_param.m_param_type = MaxParam::Unsupported;
            max_param.m_page_name = osl_param.page;

            if (osl_param.paramType == "color")
                max_param.m_param_type = MaxParam::Color;
            else if (osl_param.paramType == "float")
                max_param.m_param_type = MaxParam::Float;
            else if (osl_param.paramType == "float[2]")
                max_param.m_param_type = MaxParam::Float2f;
            else if (osl_param.paramType == "matrix")
                max_param.m_param_type = MaxParam::Matrix;
            else if (osl_param.paramType == "pointer")
                max_param.m_param_type = MaxParam::Closure;
            else if (osl_param.paramType == "int")
            {
                if (osl_param.widget == "mapper")
                    max_param.m_param_type = MaxParam::IntMapper;
                else if (osl_param.widget == "checkBox")
                    max_param.m_param_type = MaxParam::IntCheckbox;
                else
                    max_param.m_param_type = MaxParam::IntNumber;
            }
            else if (osl_param.paramType == "point")
                max_param.m_param_type = MaxParam::PointParam;
            else if (osl_param.paramType == "vector")
                max_param.m_param_type = MaxParam::VectorParam;
            else if (osl_param.paramType == "normal")
                max_param.m_param_type = MaxParam::NormalParam;
            else if (osl_param.paramType == "string")
            {
                if (osl_param.widget == "popup")
                    max_param.m_param_type = MaxParam::StringPopup;
                else
                    max_param.m_param_type = MaxParam::String;
            }
            
            if (max_param.m_param_type != MaxParam::Unsupported)
            {
                if (osl_param.isOutput)
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
        if (m_params[i].paramName == param_name)
            return &m_params[i];
    }

    return nullptr;
}
