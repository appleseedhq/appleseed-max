
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
#include "foundation/core/buildoptions.h"

// appleseed.foundation headers.
#include "foundation/containers/dictionary.h"
#include "foundation/core/concepts/noncopyable.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <maxapi.h>
#include <maxtypes.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Standard headers.
#include <vector>
#include <map>
#include <string>

// Forward declarations.
namespace renderer { class ShaderQuery; }
class OSLParamInfo;

typedef std::map<int, const std::wstring> IdNameMap;
typedef std::vector<std::pair<int, std::wstring>> IdNameVector;

class MaxParam
{
  public:
    enum ParamType
    {
        Float,
        Float2f,
        VectorParam,
        NormalParam,
        PointParam,
        Matrix,
        IntNumber,
        IntCheckbox,
        IntMapper,
        StringPopup,
        Color,
        Closure,
        String,
        Unsupported
    };

    bool is_vector() const
    {
        return  m_param_type == VectorParam ||
                m_param_type == NormalParam ||
                m_param_type == PointParam;
    }

    ParamType       m_param_type;
    bool            m_is_connectable;
    bool            m_is_constant;
    std::string     m_max_label_str;
    int             m_max_param_id;
    int             m_max_map_param_id;
    int             m_max_ctrl_id;
    std::string     m_osl_param_name;
    std::string     m_page_name;
};


//
// The OSLMetadataExtractor class extracts OSL metadata entries from appleseed dictionaries.
//

class OSLMetadataExtractor
  : public foundation::NonCopyable
{
  public:
    explicit OSLMetadataExtractor(const foundation::Dictionary& metadata);

    bool exists(const char* key) const;

    bool get_value(const char* key, std::string& value) const;

    bool get_value(const char* key, bool& value) const;

    template <typename T>
    bool get_value(const char* key, T& value) const
    {
        if (exists(key))
        {
            const foundation::Dictionary& dict = m_metadata.dictionary(key);
            value = dict.get<T>("value");
            return true;
        }

        return false;
    }

  private:
    const foundation::Dictionary& m_metadata;
};


//
// The OSLParamInfo class holds information about a parameter of an OSL shader.
//

class OSLParamInfo
{
  public:
    explicit OSLParamInfo(const foundation::Dictionary& paramInfo);

    // Query info.
    std::string m_param_name;
    std::string m_param_type;
    bool m_is_output;
    bool m_is_closure;
    bool m_is_struct;
    bool m_lock_geom;

    // Defaults.
    bool m_valid_default;
    bool m_has_default;
    std::vector<double> m_default_value;
    std::string m_default_string_value;

    // Standard metadata info.
    std::string m_units;
    std::string m_page;
    std::string m_label;
    std::string m_widget;
    std::string m_options;
    std::string m_help;
    bool m_has_min;
    double m_min_value;
    bool m_has_max;
    double m_max_value;
    bool m_has_soft_min;
    double m_soft_min_value;
    bool m_has_soft_max;
    double m_soft_max_value;
    bool m_divider;

    std::string m_maya_attribute_name;
    bool m_is_connectable;
    bool m_max_hidden_attr;
    bool m_is_deprecated;
    int m_max_param_id;
    MaxParam m_max_param;
};

std::ostream& operator<<(std::ostream& os, const OSLParamInfo& paramInfo);


//
// The OSLShaderInfo class holds information about an OSL shader.
//

class OSLShaderInfo
{
  public:
    OSLShaderInfo();

    OSLShaderInfo(
        const renderer::ShaderQuery&    q,
        const std::string               filename);

    const OSLParamInfo* find_param(const char* param_name) const;
    const OSLParamInfo* find_maya_attribute(const char* param_name) const;

    Class_ID                    m_class_id;
    bool                        m_is_texture;
    IdNameVector                m_material_id_map;
    std::wstring                m_max_shader_name;
    std::vector<OSLParamInfo>   m_output_params;
    MaxParam                    m_output_param;
    std::vector<OSLParamInfo>   m_params;
    std::string                 m_shader_name;
    IdNameMap                   m_string_map;
    IdNameVector                m_texture_id_map;
};
