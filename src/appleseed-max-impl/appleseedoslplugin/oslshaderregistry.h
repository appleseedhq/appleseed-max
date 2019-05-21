
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

// appleseed-max headers.
#include "appleseedoslplugin/oslshadermetadata.h"

// 3ds Max headers.
#include "_beginmaxheaders.h"
#include <iparamb2.h>
#include <maxtypes.h>
#include "_endmaxheaders.h"

// Standard headers.
#include <string>
#include <vector>

class OSLShaderRegistry
{
  public:
    OSLShaderRegistry();
    
    ClassDesc2* get_class_descriptor(int index) const;
    int get_size() const;
    void create_class_descriptors();
    void add_const_parameter(
        ParamBlockDesc2*        pb_desc,
        const OSLParamInfo&     osl_param,
        MaxParam&               param_info,
        IdNameMap&              string_map,
        const int               param_id,
        int&                    ctrl_id,
        int&                    string_id);
    void add_input_parameter(
        ParamBlockDesc2*        pb_desc,
        const OSLParamInfo&     osl_param,
        MaxParam&               param_info,
        IdNameVector&           texture_map,
        IdNameVector&           material_map,
        const int               param_id,
        const int               ctrl_id,
        const int               string_id);

  private:
    std::map<std::wstring, OSLShaderInfo>           m_shader_map;
    std::vector<MaxSDK::AutoPtr<ClassDesc2>>        m_class_descriptors;
    std::vector<MaxSDK::AutoPtr<ParamBlockDesc2>>   m_paramblock_descriptors;
};
