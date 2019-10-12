
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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
#include "utilities.h"

// appleseed-max headers.
#include "osloutputselectormap/osloutputselector.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/color.h"
#include "renderer/api/source.h"
#include "renderer/api/texture.h"

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"
#include "foundation/core/thirdparties.h"
#include "foundation/image/canvasproperties.h"
#include "foundation/image/tile.h"
#include "foundation/utility/searchpaths.h"
#include "foundation/utility/siphash.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <AssetManagement/AssetUser.h>
#include <Materials/Texmap.h>
#include <Rendering/ShadeContext.h>
#include <assert1.h>
#include <bitmap.h>
#include <box3.h>
#include <imtl.h>
#include <interval.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <ipoint2.h>
#include <paramtype.h>
#include <plugapi.h>
#include <point3.h>
#include <stdmat.h>
#include "appleseed-max-common/_endmaxheaders.h"

namespace asf = foundation;
namespace asr = renderer;

const char* to_enabled_disabled(const bool value)
{
    return value ? "enabled" : "disabled";
}

void print_libraries_information()
{
    RENDERER_LOG_INFO(
        "appleseed for Autodesk 3ds Max using %s version %s, %s configuration\n"
        "compiled on %s at %s using %s version %s\n"
        "copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited\n"
        "copyright (c) 2014-2019 The appleseedhq Organization\n"
        "this software is released under the MIT license (https://opensource.org/licenses/MIT).\n"
        "visit https://appleseedhq.net/ for additional information and resources.",
        asf::Appleseed::get_lib_name(),
        asf::Appleseed::get_lib_version(),
        asf::Appleseed::get_lib_configuration(),
        asf::Appleseed::get_lib_compilation_date(),
        asf::Appleseed::get_lib_compilation_time(),
        asf::Compiler::get_compiler_name(),
        asf::Compiler::get_compiler_version());
}

void print_libraries_features()
{
        const bool WithDisneyMaterial =
    #ifdef APPLESEED_WITH_DISNEY_MATERIAL
            true;
    #else
            false;
    #endif

        const bool WithEmbree =
    #ifdef APPLESEED_WITH_EMBREE
            true;
    #else
            false;
    #endif

        const bool WithSpectralSupport =
    #ifdef APPLESEED_WITH_SPECTRAL_SUPPORT
            true;
    #else
            false;
    #endif

        const bool WithGPUSupport =
    #ifdef APPLESEED_WITH_GPU
            true;
    #else
            false;
    #endif

    RENDERER_LOG_INFO(
        "library features:\n"
        "  Instruction sets              %s\n"
        "  Disney material with SeExpr   %s\n"
        "  Embree                        %s\n"
        "  Spectral support              %s\n"
        "  GPU support                   %s",
        asf::Appleseed::get_lib_cpu_features(),
        to_enabled_disabled(WithDisneyMaterial),
        to_enabled_disabled(WithEmbree),
        to_enabled_disabled(WithSpectralSupport),
        to_enabled_disabled(WithGPUSupport));
}

void print_third_party_libraries_information()
{
    const asf::LibraryVersionArray versions = asf::ThirdParties::get_versions();
    RENDERER_LOG_INFO("third party libraries:\n");
    for (size_t i = 0, e = versions.size(); i < e; ++i)
    {
        const asf::APIStringPair& version = versions[i];
        const char* lib_name = version.m_first.c_str();
        const char* lib_version = version.m_second.c_str();
        const size_t lib_name_length = strlen(lib_name);
        const std::string spacing(30 - lib_name_length, ' ');
        RENDERER_LOG_INFO("  %s%s%s", lib_name, spacing.c_str(), lib_version);
    }
}


void update_map_buttons(IParamMap2* param_map)
{
    if (param_map == nullptr)
        return;

    IParamBlock2* param_block = param_map->GetParamBlock();

    for (int i = 0, e = param_block->NumParams(); i < e; ++i)
    {
        const int param_id = param_block->IndextoID(i);
        const ParamDef& param_def = param_block->GetParamDef(param_id);
        if (param_def.type == TYPE_TEXMAP && (param_def.flags & P_NO_AUTO_LABELS))
        {
            const auto texmap = param_block->GetTexmap(param_id, GetCOREInterface()->GetTime(), FOREVER);
            param_map->SetText(param_id, texmap == nullptr ? L"" : L"M");
        }
    }
}

bool is_bitmap_texture(Texmap* map)
{
    if (map == nullptr)
        return false;

    if (map->ClassID() != Class_ID(BMTEX_CLASS_ID, 0))
        return false;

    Bitmap* bitmap = static_cast<BitmapTex*>(map)->GetBitmap(0);

    if (bitmap == nullptr)
        return false;

    return true;
}

bool is_supported_procedural_texture(Texmap* map, const bool use_max_procedural_maps)
{
    if (map == nullptr)
        return false;

    auto part_a = map->ClassID().PartA();
    auto part_b = map->ClassID().PartB();

    if (!use_max_procedural_maps)
    {
        if (map->ClassID() == OSLOutputSelector::get_class_id())
            return true;

        switch (part_a)
        {
          case OUTPUT_CLASS_ID:
            return true;

          default:
            return false;
        }
    }

    switch (part_a)
    {
      case 0x896EF2FC:                 // Substance (Built-in)   
        return part_b == 0x44BD743F;
      case 0xA661C7FF:                 // Map Output Selector  
        return part_b == 0x68AB72CD;
      case 0x6769144B:                  // VRayHDRI
        return part_b == 0x02C1017D;
      case 0x58F82B74:                  // VRayColor
        return part_b == 0x73B75D7F;
      case 0x64035FB9:                  // tiles
        return part_b == 0x69664CDC;
      case 0x1DEC5B86:                  // gradient ramp
        return part_b == 0x43383A51;
      case 0x72C8577F:                  // swirl
        return part_b == 0x39A00A1B;
      case 0x23AD0AE9:                  // Perlin marble
        return part_b == 0x158D7A88;
      case 0x243E22C6:                  // normal bump
        return part_b == 0x63F6A014;
      case 0x93A92749:                  // vector map
        return part_b == 0x6B8D470A;
      case CHECKER_CLASS_ID:
      case MARBLE_CLASS_ID:
      case MASK_CLASS_ID:
      case MIX_CLASS_ID:
      case NOISE_CLASS_ID:
      case GRADIENT_CLASS_ID:
      case TINT_CLASS_ID:
      case BMTEX_CLASS_ID:
      case COMPOSITE_CLASS_ID:
      case RGBMULT_CLASS_ID:
      case OUTPUT_CLASS_ID:
      case COLORCORRECTION_CLASS_ID:
      case 0x00000214:                  // WOOD_CLASS_ID
      case 0x00000218:                  // DENT_CLASS_ID
      case 0x46396CF1:                  // PLANET_CLASS_ID
      case 0x7712634E:                  // WATER_CLASS_ID
      case 0x0A845E7C:                  // SMOKE_CLASS_ID
      case 0x62C32B8A:                  // SPECKLE_CLASS_ID
      case 0x090B04F9:                  // SPLAT_CLASS_ID
        return true;
    }

    return false;
}

bool is_linear_texture(BitmapTex* bitmap_tex)
{
    const auto filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());
    return
        asf::ends_with(filepath, ".exr") ||
        asf::ends_with(filepath, ".hdr");
}

asf::auto_release_ptr<asf::Image> render_bitmap_to_image(
    Bitmap*                 bitmap,
    const size_t            image_width,
    const size_t            image_height,
    const size_t            tile_width,
    const size_t            tile_height)
{
    asf::auto_release_ptr<asf::Image> image(
        new asf::Image(
            image_width,
            image_height,
            tile_width,
            tile_height,
            4,
            asf::PixelFormatFloat));

    const asf::CanvasProperties& props = image->properties();

    for (size_t ty = 0; ty < props.m_tile_count_y; ++ty)
    {
        for (size_t tx = 0; tx < props.m_tile_count_x; ++tx)
        {
            asf::Tile& tile = image->tile(tx, ty);

            for (size_t y = 0, ye = tile.get_height(); y < ye; ++y)
            {
                for (size_t x = 0, xe = tile.get_width(); x < xe; ++x)
                {
                    const int ix = static_cast<int>(tx * props.m_tile_width + x);
                    const int iy = static_cast<int>(ty * props.m_tile_height + y);

                    BMM_Color_fl c;
                    bitmap->GetLinearPixels(ix, iy, 1, &c);

                    tile.set_pixel(x, y, &c.r, 4);
                }
            }
        }
    }

    return image;
}

void insert_color(asr::BaseGroup& base_group, const Color& color, const char* name)
{
    base_group.colors().insert(
        asr::ColorEntityFactory::create(
            name,
            asr::ParamArray()
                .insert("color_space", "linear_rgb")
                .insert("color", to_color3f(color))));
}

std::string insert_texture_and_instance(
    asr::BaseGroup&         base_group,
    Texmap*                 texmap,
    const bool              use_max_procedural_maps,
    const TimeValue         time,
    const asr::ParamArray&  texture_params,
    const asr::ParamArray&  texture_instance_params)
{
    if (use_max_procedural_maps && is_supported_procedural_texture(texmap, use_max_procedural_maps))
    {
        return
            insert_procedural_texture_and_instance(
                base_group,
                texmap,
                time,
                texture_params,
                texture_instance_params);
    }
    else if (is_bitmap_texture(texmap))
    {
        return
            insert_bitmap_texture_and_instance(
                base_group,
                static_cast<BitmapTex*>(texmap),
                texture_params,
                texture_instance_params);
    }

    return std::string();
}

std::string insert_bitmap_texture_and_instance(
    asr::BaseGroup&         base_group,
    BitmapTex*              bitmap_tex,
    asr::ParamArray         texture_params,
    asr::ParamArray         texture_instance_params)
{
    // todo: it can happen that `filepath` is empty here; report an error.
    const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());
    texture_params.insert("filename", filepath);

    if (!texture_params.strings().exist("color_space"))
    {
        if (is_linear_texture(bitmap_tex))
            texture_params.insert("color_space", "linear_rgb");
        else texture_params.insert("color_space", "srgb");
    }

    const std::string texture_name = wide_to_utf8(bitmap_tex->GetName());
    if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
    {
        base_group.textures().insert(
            asr::DiskTexture2dFactory().create(
                texture_name.c_str(),
                texture_params,
                asf::SearchPaths()));
    }

    const std::string texture_instance_name = texture_name + "_inst";
    if (base_group.texture_instances().get_by_name(texture_instance_name.c_str()) == nullptr)
    {
        base_group.texture_instances().insert(
            asr::TextureInstanceFactory::create(
                texture_instance_name.c_str(),
                texture_instance_params,
                texture_name.c_str()));
    }

    return texture_instance_name;
}

namespace
{
    class MaxShadeContext
      : public ShadeContext
    {
      public:
        MaxShadeContext(const asr::SourceInputs& source_inputs, const TimeValue time)
            : m_cur_time(time)
        {
            doMaps = TRUE;
            filterMaps = FALSE;
            backFace = FALSE;
            mtlNum = 1;
            ambientLight.Black();
            xshadeID = 0;
            // todo: initialize `out`?

            m_uv.x = source_inputs.m_uv_x;
            m_uv.y = source_inputs.m_uv_y;
        }

        BOOL InMtlEditor() override
        {
            return false;
        }

        LightDesc* Light(int n) override
        {
            return nullptr;
        }

        TimeValue CurTime() override
        {
            return m_cur_time;
        }

        int NodeID() override
        {
            return -1;
        }

        int FaceNumber() override
        {
            return 0;
        }

        int ProjType() override
        {
            return 0;   // 0: perspective, 1: parallel
        }

        Point3 Normal() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 GNormal() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 ReflectVector() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 RefractVector(float ior) override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 CamPos() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 V() override
        {
            return m_view;
        }

        void SetView(Point3 v) override
        {
            m_view = v;
        }

        Point3 P() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 DP() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 PObj() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Point3 DPObj() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        Box3 ObjectBox() override
        {
            return Box3(Point3(-1.0f, -1.0f, -1.0f), Point3(1.0f, 1.0f, 1.0f));
        }

        Point3 PObjRelBox() override
        {
            return m_view;
        }

        Point3 DPObjRelBox() override
        {
            return Point3(0.0f, 0.0f, 0.0f);
        }

        void ScreenUV(Point2& UV, Point2 &Duv) override
        {
            UV = m_uv;
            Duv = m_duv;
        }

        IPoint2 ScreenCoord() override
        {
            return IPoint2(0, 0);
        }

        Point3 UVW(int chan) override
        {
            return Point3(m_uv.x, m_uv.y, 0.0f);
        }

        Point3 DUVW(int chan) override
        {
            return Point3(m_duv.x, m_duv.y, 0.0f);
        }

        void DPdUVW(Point3 dP[3], int chan) override
        {
        }

        void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG = TRUE) override
        {
        }

        float Curve() override
        {
            return 0.0f;
        }

        Point3 PointTo(const Point3& p, RefFrame ito) override
        {
            return p;
        }

        Point3 PointFrom(const Point3& p, RefFrame ifrom) override
        {
            return p;
        }

        Point3 VectorTo(const Point3& p, RefFrame ito) override
        {
            return p;
        }

        Point3 VectorFrom(const Point3& p, RefFrame ifrom) override
        {
            return p;
        }

      private:
        TimeValue   m_cur_time;
        Point2      m_uv;
        Point2      m_duv;
        Point3      m_view;             // unit vector from light to point, in light space
    };

    class MaxProceduralTextureSource
      : public asr::Source
    {
      public:
        MaxProceduralTextureSource(Texmap* texmap, const TimeValue time)
          : asr::Source(false)
          , m_texmap(texmap)
          , m_time(time)
        {
        }

        asf::uint64 compute_signature() const override
        {
            return asf::siphash24(m_texmap);
        }

        Hints get_hints() const override
        {
            Hints hints;

            if (is_bitmap_texture(m_texmap))
            {
                auto bitmap = static_cast<BitmapTex*>(m_texmap)->GetBitmap(0);
                hints.m_width = static_cast<size_t>(bitmap->Width());
                hints.m_height = static_cast<size_t>(bitmap->Height());
            }
            else
            {
                // Take a random guess.
                hints.m_width = 2048;
                hints.m_height = 1080;
            }

            return hints;
        }

        void evaluate(
            asr::TextureCache&          texture_cache,
            const asr::SourceInputs&    source_inputs,
            float&                      scalar) const override
        {
            scalar = evaluate_float(source_inputs);
        }

        void evaluate(
            asr::TextureCache&          texture_cache,
            const asr::SourceInputs&    source_inputs,
            asf::Color3f&               linear_rgb) const override
        {
            evaluate_color(source_inputs, linear_rgb.r, linear_rgb.g, linear_rgb.b);
        }

        void evaluate(
            asr::TextureCache&          texture_cache,
            const asr::SourceInputs&    source_inputs,
            asr::Spectrum&              spectrum) const override
        {
            DbgAssert(spectrum.size() == 3);
            evaluate_color(source_inputs, spectrum[0], spectrum[1], spectrum[2]);
        }

        void evaluate(
            asr::TextureCache&          texture_cache,
            const asr::SourceInputs&    source_inputs,
            asr::Alpha&                 alpha) const override
        {
            alpha.set(evaluate_float(source_inputs));
        }

        void evaluate(
            asr::TextureCache&          texture_cache,
            const asr::SourceInputs&    source_inputs,
            asf::Color3f&               linear_rgb,
            asr::Alpha&                 alpha) const override
        {
            evaluate_color(source_inputs, linear_rgb.r, linear_rgb.g, linear_rgb.b, alpha);
        }

        void evaluate(
            asr::TextureCache&          texture_cache,
            const asr::SourceInputs&    source_inputs,
            asr::Spectrum&              spectrum,
            asr::Alpha&                 alpha) const override
        {
            DbgAssert(spectrum.size() == 3);
            evaluate_color(source_inputs, spectrum[0], spectrum[1], spectrum[2], alpha);
        }

      private:
        Texmap*         m_texmap;
        const TimeValue m_time;

        float evaluate_float(const asr::SourceInputs& source_inputs) const
        {
            MaxShadeContext maxsc(source_inputs, m_time);

            return m_texmap->EvalMono(maxsc);
        }

        void evaluate_color(const asr::SourceInputs& source_inputs, float& r, float& g, float& b) const
        {
            MaxShadeContext maxsc(source_inputs, m_time);
            
            const AColor tex_color = m_texmap->EvalColor(maxsc);

            r = tex_color.r;
            g = tex_color.g;
            b = tex_color.b;
        }

        void evaluate_color(const asr::SourceInputs& source_inputs, float& r, float& g, float& b, asr::Alpha& alpha) const
        {
            MaxShadeContext maxsc(source_inputs, m_time);
            
            const AColor tex_color = m_texmap->EvalColor(maxsc);

            r = tex_color.r;
            g = tex_color.g;
            b = tex_color.b;

            alpha.set(tex_color.a);
        }
    };

    class MaxProceduralTexture
      : public asr::Texture
    {
      public:
        MaxProceduralTexture(const char* name, Texmap* texmap, const TimeValue time)
          : asr::Texture(name, asr::ParamArray())
          , m_texmap(texmap)
          , m_time(time)
        {
            // Dummy values.
            m_properties =
                asf::CanvasProperties(
                    512, 512,
                    64, 64,
                    3, asf::PixelFormat::PixelFormatUInt8);
        }

        void release() override
        {
            delete this;
        }

        const char* get_model() const override
        {
            return "max_procedural_texture";
        }

        asf::ColorSpace get_color_space() const override
        {
            return asf::ColorSpaceLinearRGB;
        }

        const asf::CanvasProperties& properties() override
        {
            return m_properties;
        }

        asr::Source* create_source(
            const asf::UniqueID         assembly_uid,
            const asr::TextureInstance& texture_instance) override
        {
            return new MaxProceduralTextureSource(m_texmap, m_time);
        }

        asf::Tile* load_tile(
            const size_t                tile_x,
            const size_t                tile_y) override
        {
            return nullptr;
        }

        void unload_tile(
            const size_t                tile_x,
            const size_t                tile_y,
            const asf::Tile*            tile) override
        {
        }

      private:
        asf::CanvasProperties   m_properties;
        Texmap*                 m_texmap;
        TimeValue               m_time;
    };

    void load_map_files_recursively(MtlBase* mat_base, TimeValue time)
    {
        if (IsTex(mat_base))
        {
            Texmap* tex_map = static_cast<Texmap*>(mat_base);
            tex_map->LoadMapFiles(time);
        }

        for (int i = 0, e = mat_base->NumSubTexmaps(); i < e; i++)
        {
            Texmap* sub_tex = mat_base->GetSubTexmap(i);
            if (sub_tex != nullptr)
                load_map_files_recursively(sub_tex, time);
        }
    }
}

std::string insert_procedural_texture_and_instance(
    asr::BaseGroup&         base_group,
    Texmap*                 texmap,
    const TimeValue         time,
    asr::ParamArray         texture_params,
    asr::ParamArray         texture_instance_params)
{
    texmap->Update(time, FOREVER);
    load_map_files_recursively(texmap, time);

    if (!texture_params.strings().exist("color_space"))
    {
        // todo: should probably check max's gamma settings here.
        texture_params.insert("color_space", "linear_rgb");
    }

    const std::string texture_name = wide_to_utf8(texmap->GetName());
    if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
    {
        base_group.textures().insert(
            asf::auto_release_ptr<asr::Texture>(
                new MaxProceduralTexture(
                    texture_name.c_str(),
                    texmap,
                    time)));
    }

    const std::string texture_instance_name = texture_name + "_inst";
    if (base_group.texture_instances().get_by_name(texture_instance_name.c_str()) == nullptr)
    {
        base_group.texture_instances().insert(
            asr::TextureInstanceFactory::create(
                texture_instance_name.c_str(),
                texture_instance_params,
                texture_name.c_str()));
    }

    return texture_instance_name;
}
