
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
#include "oslshaderregistry.h"

// appleseed-max headers.
#include "appleseedoslplugin/oslclassdesc.h"
#include "appleseedoslplugin/oslmaterial.h"
#include "appleseedoslplugin/oslshadermetadata.h"
#include "appleseedoslplugin/osltexture.h"
#include "bump/resource.h"
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/searchpaths.h"
#include "foundation/utility/string.h"

// 3ds Max headers.
#include <iparamb2.h>
#include <iparamm2.h>
#include <maxtypes.h>

// Boost headers.
#include "boost/filesystem.hpp"

// Standard headers.
#include <memory>

namespace bfs = boost::filesystem;
namespace asf = foundation;
namespace asr = renderer;

typedef std::map<std::wstring, OSLShaderInfo> OSLShaderInfoMap;

namespace
{
    void set_button_text(
        IParamBlock2*   pblock,
        const ParamID   id,
        const bool      is_bump = false)
    {
        IParamMap2* map = pblock->GetMap();
        if (map == nullptr)
            return;

        Texmap* tex_map = nullptr;
        Interval iv;

        pblock->GetValue(id, 0, tex_map, iv);

        map->SetText(id, tex_map != nullptr ? tex_map->GetFullName() : L"None");
    }

    void set_short_button_text(
        IParamBlock2*   pblock,
        const ParamID   id,
        const bool      is_bump = false)
    {
        IParamMap2* map = pblock->GetMap();
        if (map == nullptr)
            return;

        Texmap* tex_map = nullptr;
        Interval iv;

        pblock->GetValue(id, 0, tex_map, iv);

        map->SetText(id, tex_map != nullptr ? L"M" : L"");
    }

    class TextureAccessorShort
        : public PBAccessor
    {
    public:
        void Set(
            PB2Value&         v,
            ReferenceMaker*   owner,
            ParamID           id,
            int               tabIndex,
            TimeValue         t) override
        {
            IParamBlock2* pblock = owner->GetParamBlock(0);
            
            if (pblock != nullptr)
                set_short_button_text(pblock, id);
        }
    };

    class TextureAccessor
      : public PBAccessor
    {
      public:
        void Set(
            PB2Value&         v,
            ReferenceMaker*   owner,
            ParamID           id,
            int               tabIndex,
            TimeValue         t) override
        {
            IParamBlock2* pblock = owner->GetParamBlock(0);
            
            if (pblock != nullptr)
                set_button_text(pblock, id);
        }
    };
    
    class BumpTextureAccessor
      : public PBAccessor
    {
      public:
        void Set(
            PB2Value&         v,
            ReferenceMaker*   owner,
            ParamID           id,
            int               tabIndex,
            TimeValue         t) override
        {
            IParamBlock2* pblock = owner->GetParamBlock(1);

            set_button_text(pblock, id, true);
        }
    };

    class MaterialAccessor
      : public PBAccessor
    {
      public:
        void Set(
            PB2Value&         v,
            ReferenceMaker*   owner,
            ParamID           id,
            int               tabIndex,
            TimeValue         t) override
        {
            IParamBlock2* pblock = owner->GetParamBlock(0);
            if (pblock == nullptr)
                return;

            IParamMap2* p_map = pblock->GetMap();
            if (p_map == nullptr)
                return;

            Mtl* mtl = nullptr;
            Interval iv;

            pblock->GetValue(id, 0, mtl, iv);

            p_map->SetText(id, mtl != nullptr ? mtl->GetFullName() : L"None");
        }
    };

    static TextureAccessor g_texture_accessor;
    static TextureAccessorShort g_texture_accessor_short;
    static BumpTextureAccessor g_bump_accessor;
    static MaterialAccessor g_material_accessor;

    bool do_register_shader(
        OSLShaderInfoMap&               shader_map,
        const bfs::path&                shaderPath,
        asr::ShaderQuery&               query)
    {
        if (query.open(shaderPath.string().c_str()))
        {
            // Get the shader filename without the .oso extension.
            OSLShaderInfo shaderInfo(query, shaderPath.filename().replace_extension().string());

            if (shaderInfo.m_max_shader_name.empty())
            {
                RENDERER_LOG_DEBUG(
                    "Skipping registration for OSL shader %s. No 3ds Max metadata found.",
                    shaderInfo.m_shader_name);
                return false;
            }

            if (shader_map.count(shaderInfo.m_max_shader_name) != 0)
            {
                RENDERER_LOG_DEBUG(
                    "Skipping registration for OSL shader %s. Already registered.",
                    shaderInfo.m_shader_name);
                return false;
            }

            RENDERER_LOG_DEBUG(
                "Registered OSL shader %s",
                shaderInfo.m_shader_name);

            shader_map[shaderInfo.m_max_shader_name] = shaderInfo;

            return true;
        }
        return false;
    }

    bool register_shader(
        OSLShaderInfoMap&       shader_map,
        const bfs::path&        shaderPath,
        asr::ShaderQuery&       query)
    {
        try
        {
            return do_register_shader(shader_map, shaderPath, query);
        }
        catch (const asf::StringException& e)
        {
            RENDERER_LOG_ERROR(
                "OSL shader query for shader %s failed, error = %s.",
                shaderPath.string().c_str(),
                e.string());
        }
        catch (const std::exception& e)
        {
            RENDERER_LOG_ERROR(
                "OSL shader query for shader %s failed, error = %s.",
                shaderPath.string().c_str(),
                e.what());
        }
        catch (...)
        {
            RENDERER_LOG_ERROR(
                "OSL shader query for shader %s failed.",
                shaderPath.string().c_str());
        }

        return false;
    }

    void register_shaders_in_directory(
        OSLShaderInfoMap&       shader_map,
        const bfs::path&        shaderDir,
        asr::ShaderQuery&       query)
    {
        try
        {
            if (bfs::exists(shaderDir) && bfs::is_directory(shaderDir))
            {
                for (bfs::directory_iterator it(shaderDir), e; it != e; ++it)
                {
                    const bfs::file_status shaderStatus = it->status();
                    if (shaderStatus.type() == bfs::regular_file)
                    {
                        const bfs::path& shaderPath = it->path();
                        if (shaderPath.extension() == ".oso")
                        {
                            RENDERER_LOG_DEBUG(
                                "Found OSL shader %s.",
                                shaderPath.string().c_str());

                            register_shader(shader_map, shaderPath, query);
                        }
                    }

                    // TODO: should we handle symlinks here?
                }
            }
        }
        catch (const bfs::filesystem_error& e)
        {
            RENDERER_LOG_ERROR(
                "Filesystem error, path = %s, error = %s.",
                shaderDir.string().c_str(),
                e.what());
        }
    }

    void register_shading_nodes(OSLShaderInfoMap& shader_map)
    {
        // Build list of dirs to look for shaders
        std::vector<bfs::path> shaderPaths;

        bfs::path p(get_root_path());
        p = p / "shaders" / "appleseed";
        shaderPaths.push_back(p);

        // Paths from the environment.
        if (const char* envSearchPath = getenv("APPLESEED_SEARCHPATH"))
        {
            const char pathSep = asf::SearchPaths::environment_path_separator();
            std::vector<std::string> paths;
            asf::split(
                envSearchPath,
                std::string(&pathSep, 1),
                paths);

            for (size_t i = 0, e = paths.size(); i < e; ++i)
                shaderPaths.push_back(bfs::path(paths[i]));
        }

        asf::auto_release_ptr<asr::ShaderQuery> query =
            asr::ShaderQueryFactory::create();

        // Iterate in reverse order to allow overriding of shaders.
        for (int i = static_cast<int>(shaderPaths.size()) - 1; i >= 0; --i)
        {
            RENDERER_LOG_DEBUG(
                "Looking for OSL shaders in path %s.",
                shaderPaths[i].string().c_str());

            register_shaders_in_directory(shader_map, shaderPaths[i], *query);
        }
    }

    void add_bump_parameters(
        ParamBlockDesc2*            pb_desc,
        IdNameVector&               texture_map,
        int&                        param_id)
    {
        pb_desc->AddParam(
            param_id++, L"bump_method", TYPE_INT, 0, IDS_BUMP_METHOD,
            p_ui, TYPE_INT_COMBOBOX, IDC_COMBO_BUMP_METHOD,
            2, IDS_COMBO_BUMP_METHOD_BUMPMAP, IDS_COMBO_BUMP_METHOD_NORMALMAP,
            p_vals, 0, 1,
            p_default, 0,
            p_end);

        const int p_id = param_id++;
        texture_map.push_back(std::make_pair(p_id, utf8_to_wide("Bump")));

        // Keep bump map the last in the list of textures.
        const int bump_param_index = static_cast<int>(texture_map.size()) - 1;
        pb_desc->AddParam(
            p_id, L"bump_texmap", TYPE_TEXMAP, 0, IDS_TEXMAP_BUMP_MAP,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_BUMP_MAP,
            p_subtexno, bump_param_index,
            p_accessor, &g_bump_accessor,
            p_end);

        pb_desc->AddParam(
            param_id++, L"bump_amount", TYPE_FLOAT, P_ANIMATABLE, IDS_BUMP_AMOUNT,
            p_default, 1.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_BUMP_AMOUNT, IDC_SPINNER_BUMP_AMOUNT, SPIN_AUTOSCALE,
            p_end);

        pb_desc->AddParam(
            param_id++, L"bump_up_vector", TYPE_INT, 0, IDS_BUMP_UP_VECTOR,
            p_ui, TYPE_INT_COMBOBOX, IDC_COMBO_BUMP_UP_VECTOR,
            2, IDS_COMBO_BUMP_UP_VECTOR_Y, IDS_COMBO_BUMP_UP_VECTOR_Z,
            p_vals, 0, 1,
            p_default, 1,
            p_end);
    }

    void add_output_parameter(
        ParamBlockDesc2*                    pb_desc,
        const std::vector<OSLParamInfo>&    output_params,
        IdNameMap&                          string_map,
        const int                           param_id,
        const int                           ctrl_id,
        int&                                string_id)
    {
        pb_desc->AddParam(
            param_id,
            L"Output",
            TYPE_INT,
            0,
            string_id,
            p_ui, TYPE_INT_COMBOBOX, ctrl_id,
            0,
            p_end
        );

        int num_items = static_cast<int>(output_params.size());

        Tab<int> string_items;
        string_items.SetCount(num_items);
        for (size_t i = 0, e = num_items; i < e; ++i)
        {
            int str_id = ++string_id;
            string_items[i] = str_id;

            string_map.insert(std::make_pair(str_id, utf8_to_wide(output_params[i].m_param_name)));
        }

        pb_desc->ParamOptionContentValues(param_id, string_items);
    }
}

void OSLShaderRegistry::create_class_descriptors()
{
    register_shading_nodes(m_shader_map);

    for (auto& shader_pair : m_shader_map)
    {
        OSLShaderInfo& shader = shader_pair.second;
        ClassDesc2* class_descr(new OSLPluginClassDesc(&shader));
        ParamBlockDesc2* param_block_descr(new ParamBlockDesc2(
            // --- Required arguments ---
            0,                                          // parameter block's ID
            L"oslTextureMapParams",                     // internal parameter block's name
            0,                                          // ID of the localized name string
            class_descr,                                // class descriptor
            P_AUTO_CONSTRUCT,                           // block flags

                                                        // --- P_AUTO_CONSTRUCT arguments ---
            0,                                          // parameter block's reference number
            p_end
        ));

        int param_id = 0;
        int ctrl_id = 100;
        int string_id = 100;
        for (auto& param_info : shader.m_params)
        {
            param_info.m_max_param.m_max_ctrl_id = ctrl_id++;
            param_info.m_max_param.m_max_param_id = param_id;

            shader.m_string_map.insert(std::make_pair(string_id, utf8_to_wide(param_info.m_max_param.m_max_label_str)));

            if (param_info.m_max_param.m_has_constant)
            {
                add_const_parameter(
                    param_block_descr,
                    param_info,
                    param_info.m_max_param,
                    shader.m_string_map,
                    param_id,
                    ctrl_id,
                    string_id);

                param_id++;
                string_id++;
            }

            if (param_info.m_max_param.m_connectable)
            {
                add_input_parameter(
                    param_block_descr,
                    param_info,
                    param_info.m_max_param,
                    shader.m_texture_id_map,
                    shader.m_material_id_map,
                    param_id,
                    ctrl_id,
                    string_id);

                param_id++;
                ctrl_id++;
                string_id++;
            }

        }

        shader.m_string_map.insert(std::make_pair(string_id++, L"Output"));

        MaxParam max_output_param;
        max_output_param.m_max_label_str = "Output";
        max_output_param.m_osl_param_name = "output";
        max_output_param.m_param_type = MaxParam::StringPopup;
        max_output_param.m_max_ctrl_id = ctrl_id++;
        max_output_param.m_max_param_id = param_id;
        max_output_param.m_connectable = false;
        max_output_param.m_has_constant = true;

        shader.m_output_param = max_output_param;
        
        add_output_parameter(
            param_block_descr,
            shader.m_output_params,
            shader.m_string_map,
            param_id,
            ctrl_id,
            string_id);

        auto tn_vec = shader.find_param("Tn");
        auto bump_normal = shader.find_maya_attribute("normalCamera");

        if (!shader.m_is_texture && 
            (tn_vec != nullptr || 
            bump_normal != nullptr))
        {
            ParamBlockDesc2* bump_param_block_descr(new ParamBlockDesc2(
                // --- Required arguments ---
                1,                                          // parameter block's ID
                L"oslBumpParams",                           // internal parameter block's name
                0,                                          // ID of the localized name string
                class_descr,                                // class descriptor
                P_AUTO_CONSTRUCT,                           // block flags

                                                            // --- P_AUTO_CONSTRUCT arguments ---
                1,                                          // parameter block's reference number
                p_end
            ));

            add_bump_parameters(
                bump_param_block_descr,
                shader.m_texture_id_map,
                param_id);

            m_paramblock_descriptors.push_back(MaxSDK::AutoPtr<ParamBlockDesc2>(bump_param_block_descr));
        }

        m_paramblock_descriptors.push_back(MaxSDK::AutoPtr<ParamBlockDesc2>(param_block_descr));
        m_class_descriptors.push_back(MaxSDK::AutoPtr<ClassDesc2>(class_descr));
    }
}

void OSLShaderRegistry::add_const_parameter(
    ParamBlockDesc2*        pb_desc,
    const OSLParamInfo&     osl_param,
    MaxParam&               max_param,
    IdNameMap&              string_map,
    const int               param_id,
    int&                    ctrl_id,
    int&                    string_id)
{
    auto param_str = utf8_to_wide(max_param.m_osl_param_name);
    if (max_param.m_param_type == MaxParam::Color)
    {
        Color def_val(0.0f, 0.0f, 0.0f);
        if (osl_param.m_has_default)
        {
            const float r = static_cast<float>(osl_param.m_default_value.at(0));
            const float g = static_cast<float>(osl_param.m_default_value.at(0));
            const float b = static_cast<float>(osl_param.m_default_value.at(0));
            def_val = Color(r, g, b);
        }

        const int ctrl_id_1 = ctrl_id++;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_RGBA,
            P_ANIMATABLE,
            string_id,
            p_default, def_val,
            p_ui, TYPE_COLORSWATCH, ctrl_id_1,
            p_end
        );
    }

    if (max_param.m_param_type == MaxParam::Float)
    {
        float min_val = -10000.0f;
        float max_val = 10000.0f;
        float step_val = 0.1f;
        float def_val = 0.5f;

        if (osl_param.m_has_min)
            min_val = static_cast<float>(osl_param.m_min_value);

        if (osl_param.m_has_max)
            max_val = static_cast<float>(osl_param.m_max_value);

        if (osl_param.m_has_default)
            def_val = static_cast<float>(osl_param.m_default_value.at(0));

        const int ctrl_id_1 = ctrl_id++;
        const int ctrl_id_2 = ctrl_id++;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_FLOAT,
            P_ANIMATABLE,
            string_id,
            p_default, def_val,
            p_range, min_val, max_val,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrl_id_1, ctrl_id_2, step_val,
            p_end
        );
    }
    
    if (max_param.m_param_type == MaxParam::IntNumber)
    {
        int min_val = -10000;
        int max_val = 10000;
        int step_val = 1;
        int def_val = 50;

        if (osl_param.m_has_min)
            min_val = static_cast<int>(osl_param.m_min_value);

        if (osl_param.m_has_max)
            max_val = static_cast<int>(osl_param.m_max_value);

        if (osl_param.m_has_default)
            def_val = static_cast<int>(osl_param.m_default_value.at(0));

        const int ctrl_id_1 = ctrl_id++;
        const int ctrl_id_2 = ctrl_id++;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_INT,
            P_ANIMATABLE,
            string_id,
            p_default, def_val,
            p_range, min_val, max_val,
            p_ui, TYPE_SPINNER, EDITTYPE_INT, ctrl_id_1, ctrl_id_2, step_val,
            p_end
        );
    }

    if (max_param.m_param_type == MaxParam::IntCheckbox)
    {
        int def_val = TRUE;
        if (osl_param.m_has_default)
            def_val = static_cast<int>(osl_param.m_default_value.at(0));

        const int ctrl_id_1 = ctrl_id++;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_INT,
            0,
            string_id,
            p_default, def_val,
            p_ui, TYPE_SINGLECHEKBOX, ctrl_id_1,
            p_end
        );
    }
    
    if (max_param.m_param_type == MaxParam::IntMapper)
    {
        int def_val = 0;
        if (osl_param.m_has_default)
            def_val = static_cast<int>(osl_param.m_default_value.at(0));

        const int ctrl_id_1 = ctrl_id++;
        
        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_INT,
            0,
            string_id,
            p_ui, TYPE_INT_COMBOBOX, ctrl_id_1,
            0,
            p_end
        );
        
        std::vector<std::string> fields;
        asf::tokenize(osl_param.m_options, "|", fields);
        const int num_items = static_cast<int>(fields.size());

        Tab<int> string_items;
        Tab<int> string_item_values;
        string_items.SetCount(num_items);
        string_item_values.SetCount(num_items);
        std::vector<std::string> subfields;
        for (size_t i = 0, e = fields.size(); i < e; ++i)
        {
            subfields.clear();
            asf::tokenize(fields[i].c_str(), ":", subfields);

            int str_id = ++string_id;
            string_items[i] = str_id;
            string_item_values[i] = asf::from_string<int>(subfields[1]);

            string_map.insert(std::make_pair(str_id, utf8_to_wide(subfields[0])));
        }

        pb_desc->ParamOptionContentValues(param_id, string_items, &string_item_values);
        pb_desc->ParamOption(param_id, p_default, def_val);
    }

    if (max_param.m_param_type == MaxParam::StringPopup)
    {
        int def_val = 0;
        std::string def_str;
        if (osl_param.m_has_default)
            def_str = osl_param.m_default_string_value;

        const int ctrl_id_1 = ctrl_id++;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_INT,
            0,
            string_id,
            p_ui, TYPE_INT_COMBOBOX, ctrl_id_1,
            0,
            p_end
        );

        std::vector<std::string> fields;
        asf::tokenize(osl_param.m_options, "|", fields);
        const int num_items = static_cast<int>(fields.size());

        Tab<int> string_items;
        string_items.SetCount(num_items);
        for (size_t i = 0, e = fields.size(); i < e; ++i)
        {
            int str_id = ++string_id;
            string_items[i] = str_id;

            if (def_str == fields[i])
                def_val = static_cast<int>(i);

            string_map.insert(std::make_pair(str_id, utf8_to_wide(fields[i])));
        }
        
        pb_desc->ParamOptionContentValues(param_id, string_items);
        pb_desc->ParamOption(param_id, p_default, def_val);
    }

    if (max_param.m_param_type == MaxParam::VectorParam ||
        max_param.m_param_type == MaxParam::NormalParam ||
        max_param.m_param_type == MaxParam::PointParam)
    {
        Point3 def_val(0.0f, 0.0f, 0.0f);
        if (osl_param.m_has_default)
        {
            const float x = static_cast<float>(osl_param.m_default_value.at(0));
            const float y = static_cast<float>(osl_param.m_default_value.at(1));
            const float z = static_cast<float>(osl_param.m_default_value.at(2));
            def_val = Point3(x, y, z);
        }

        const int ctrl_id_1 = ctrl_id++;
        const int ctrl_id_2 = ctrl_id++;
        const int ctrl_id_3 = ctrl_id++;
        const int ctrl_id_4 = ctrl_id++;
        const int ctrl_id_5 = ctrl_id++;
        const int ctrl_id_6 = ctrl_id++;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_POINT3,
            P_ANIMATABLE,
            string_id,
            p_default, def_val,
            p_range, -10.0f, 10.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrl_id_1, ctrl_id_2, ctrl_id_3, ctrl_id_4, ctrl_id_5, ctrl_id_6, 0.1f,
            p_end
        );
    }
    
    if (max_param.m_param_type == MaxParam::String)
    {
        int ctrl_id_1 = ctrl_id++;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_STRING,
            0,
            string_id,
            p_ui, TYPE_EDITBOX, ctrl_id_1,
            p_end
        );
    }
}

void OSLShaderRegistry::add_input_parameter(
    ParamBlockDesc2*        pb_desc,
    const OSLParamInfo&     osl_param,
    MaxParam&               max_param,
    IdNameVector&           texture_map,
    IdNameVector&           material_map,
    const int               param_id,
    const int               ctrl_id,
    const int               string_id)
{
    if (max_param.m_param_type == MaxParam::Closure)
    {
        auto param_str = utf8_to_wide(max_param.m_osl_param_name);
        material_map.push_back(std::make_pair(param_id, utf8_to_wide(max_param.m_max_label_str)));

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_MTL,
            0,
            string_id,
            p_ui, TYPE_MTLBUTTON, ctrl_id,
            p_accessor, &g_material_accessor,
            p_end
        );
    } 
    else
    {
        auto param_str = utf8_to_wide(max_param.m_osl_param_name + "_map");
        texture_map.push_back(std::make_pair(param_id, utf8_to_wide(max_param.m_max_label_str)));

        const bool short_button = max_param.m_has_constant &&
            (max_param.m_param_type == MaxParam::VectorParam || 
                max_param.m_param_type == MaxParam::NormalParam ||
                max_param.m_param_type == MaxParam::PointParam ||
                max_param.m_param_type == MaxParam::StringPopup ||
                max_param.m_param_type == MaxParam::IntMapper);

        const int flag = short_button ? P_NO_AUTO_LABELS : 0;
        PBAccessor* tex_accessor = &g_texture_accessor;
        if (short_button)
            tex_accessor = &g_texture_accessor_short;

        pb_desc->AddParam(
            param_id,
            param_str.c_str(),
            TYPE_TEXMAP,
            flag,
            string_id,
            p_ui, TYPE_TEXMAPBUTTON, ctrl_id,
            p_accessor, tex_accessor,
            p_end
        );
    }
}

OSLShaderRegistry::OSLShaderRegistry()
{
    m_class_descriptors.clear();
    m_shader_map.clear();
}

ClassDesc2* OSLShaderRegistry::get_class_descriptor(int index) const
{
    if (index < m_class_descriptors.size())
        return m_class_descriptors[index].Get();
    return nullptr;
}

int OSLShaderRegistry::get_size() const
{
    return static_cast<int>(m_class_descriptors.size());
}
