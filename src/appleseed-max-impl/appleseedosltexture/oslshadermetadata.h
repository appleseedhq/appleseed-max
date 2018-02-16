
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

#pragma once

// appleseed.foundation headers.
#include "foundation/core/concepts/noncopyable.h"
#include "foundation/utility/containers/dictionary.h"

// 3ds Max headers.
#include <maxapi.h>
#include <maxtypes.h>

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
        return  param_type == VectorParam ||
                param_type == NormalParam ||
                param_type == PointParam;
    }

    ParamType       param_type;
    bool            connectable;
    std::string     max_label_str;
    int             max_param_id;
    int             max_ctrl_id;
    std::string     osl_param_name;
    std::string     page_name;
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

    bool getValue(const char* key, std::string& value);

    bool getValue(const char* key, bool& value);

    template <typename T>
    bool getValue(const char* key, T& value) const
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
    std::string paramName;
    std::string paramType;
    bool isOutput;
    bool isClosure;
    bool isStruct;
    std::string structName;
    bool isArray;
    int arrayLen;
    bool lockGeom;

    // Defaults.
    bool validDefault;
    bool hasDefault;
    std::vector<double> defaultValue;
    std::string defaultStringValue;

    // Standard metadata info.
    std::string units;
    std::string page;
    std::string label;
    std::string widget;
    std::string options;
    std::string help;
    bool hasMin;
    double minValue;
    bool hasMax;
    double maxValue;
    bool hasSoftMin;
    double softMinValue;
    bool hasSoftMax;
    double softMaxValue;
    bool divider;

    bool mayaAttributeConnectable;
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

    const OSLParamInfo* findParam(const char* param_name) const;

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
