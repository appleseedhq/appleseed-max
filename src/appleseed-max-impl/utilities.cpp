
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
#include "appleseedoslplugin/osltexture.h"
#include "osloutputselectormap/osloutputselector.h"
#include "main.h"

// appleseed.renderer headers.
#include "renderer/api/color.h"
#include "renderer/api/source.h"
#include "renderer/api/texture.h"

// appleseed.foundation headers.
#include "foundation/image/canvasproperties.h"
#include "foundation/image/tile.h"
#include "foundation/utility/searchpaths.h"
#include "foundation/utility/siphash.h"
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <AssetManagement/AssetUser.h>
#include <assert1.h>
#include <bitmap.h>
#include <imtl.h>
#include <iparamm2.h>
#include <maxapi.h>
#include <plugapi.h>
#include <stdmat.h>

// Windows headers.
#include <Shlwapi.h>

namespace asf = foundation;
namespace asr = renderer;

std::string wide_to_utf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    const int wstr_size = static_cast<int>(wstr.size());
    const int result_size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_size, nullptr, 0, nullptr, nullptr);

    std::string result(result_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_size, &result[0], result_size, nullptr, nullptr);

    return result;
}

std::string wide_to_utf8(const wchar_t* wstr)
{
    const int result_size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);

    std::string result(result_size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], result_size - 1, nullptr, nullptr);

    return result;
}

std::wstring utf8_to_wide(const std::string& str)
{
    if (str.empty())
        return std::wstring();

    const int str_size = static_cast<int>(str.size());
    const int result_size = MultiByteToWideChar(CP_UTF8, 0, &str[0], str_size, nullptr, 0);

    std::wstring result(result_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], str_size, &result[0], result_size);

    return result;
}

std::wstring utf8_to_wide(const char* str)
{
    const int result_size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);

    std::wstring result(result_size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &result[0], result_size - 1);

    return result;
}

std::string get_root_path()
{
    wchar_t path[MAX_PATH];
    const DWORD path_length = sizeof(path) / sizeof(wchar_t);

    const auto result = GetModuleFileName(g_module, path, path_length);
    DbgAssert(result != 0);

    PathRemoveFileSpec(path);

    return wide_to_utf8(path);
}

WStr replace_extension(const WStr& file_path, const WStr& new_ext)
{
    const int i = file_path.last(L'.');
    WStr new_file_path = i == -1 ? file_path : file_path.Substr(0, i);
    new_file_path.Append(new_ext);
    return new_file_path;
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

bool is_osl_texture(Texmap* map)
{
    return dynamic_cast<OSLTexture*>(map) != nullptr;
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
    Bitmap*         bitmap,
    const size_t    image_width,
    const size_t    image_height,
    const size_t    tile_width,
    const size_t    tile_height)
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
    asr::BaseGroup& base_group,
    Texmap*         texmap,
    const bool      use_max_procedural_maps,
    const TimeValue time,
    asr::ParamArray texture_params,
    asr::ParamArray texture_instance_params)
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
    asr::BaseGroup& base_group,
    BitmapTex*      bitmap_tex,
    asr::ParamArray texture_params,
    asr::ParamArray texture_instance_params)
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
    asr::BaseGroup& base_group,
    Texmap*         texmap,
    const TimeValue time,
    asr::ParamArray texture_params,
    asr::ParamArray texture_instance_params)
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

asf::int16 pearson_hash16(const std::string& param_name)
{

    const asf::uint8 T[256] = {
        73, 191, 186, 32, 110, 23, 189, 42, 52, 133, 155, 10, 62, 41, 201, 234,
        203, 113, 50, 18, 111, 139, 247, 231, 244, 30, 131, 168, 68, 194, 59, 195,
        172, 159, 118, 99, 115, 13, 29, 141, 21, 98, 219, 235, 19, 107, 83, 31,
        223, 119, 197, 87, 6, 158, 241, 163, 226, 93, 126, 199, 252, 17, 106, 132,
        109, 169, 214, 85, 33, 91, 4, 254, 15, 227, 233, 187, 5, 170, 7, 101,
        24, 116, 108, 153, 45, 250, 217, 64, 36, 149, 206, 105, 27, 138, 150, 210,
        202, 37, 200, 78, 12, 143, 212, 137, 117, 136, 183, 79, 165, 47, 182, 162,
        228, 243, 215, 53, 127, 44, 35, 178, 167, 253, 0, 204, 58, 121, 198, 161,
        135, 54, 232, 43, 242, 176, 16, 84, 154, 80, 177, 76, 3, 82, 171, 100,
        88, 125, 104, 140, 192, 2, 209, 147, 237, 134, 75, 49, 129, 211, 90, 145,
        151, 26, 124, 128, 224, 39, 251, 164, 40, 48, 221, 102, 218, 9, 94, 61,
        103, 28, 181, 89, 229, 156, 246, 69, 51, 205, 57, 184, 120, 70, 213, 114,
        144, 86, 130, 255, 157, 220, 148, 196, 230, 239, 245, 208, 95, 34, 193, 97,
        180, 146, 11, 225, 71, 20, 216, 66, 46, 166, 25, 14, 236, 77, 160, 142,
        38, 1, 8, 190, 72, 123, 222, 179, 63, 122, 248, 175, 60, 174, 112, 249,
        238, 152, 74, 240, 92, 65, 81, 207, 22, 96, 173, 56, 185, 188, 55, 67,

        //126, 1, 171, 217, 54, 173, 246, 214, 16, 223, 93, 113, 67, 62, 26, 163,
        //121, 56, 178, 203, 137, 19, 50, 176, 233, 209, 245, 206, 90, 185, 30, 17,
        //188, 112, 168, 14, 147, 86, 11, 108, 140, 35, 222, 23, 80, 104, 31, 201,
        //208, 21, 169, 187, 248, 241, 136, 142, 232, 192, 44, 84, 91, 96, 74, 22,
        //128, 37, 196, 227, 120, 132, 220, 202, 244, 166, 247, 239, 64, 69, 53, 194,
        //111, 149, 89, 131, 119, 39, 213, 243, 4, 135, 83, 152, 103, 0, 250, 125,
        //183, 59, 28, 114, 57, 253, 105, 170, 186, 242, 195, 224, 36, 42, 179, 216,
        //78, 162, 204, 66, 189, 229, 124, 238, 110, 122, 40, 123, 58, 82, 107, 71,
        //236, 205, 138, 47, 13, 5, 156, 70, 212, 102, 9, 3, 144, 221, 6, 127,
        //117, 65, 159, 210, 254, 79, 211, 97, 18, 63, 151, 191, 92, 27, 55, 164,
        //38, 20, 109, 77, 106, 180, 34, 130, 72, 200, 172, 181, 129, 45, 165, 225,
        //139, 41, 43, 230, 155, 49, 75, 145, 167, 133, 61, 215, 252, 199, 15, 218,
        //116, 76, 143, 207, 148, 182, 219, 68, 81, 161, 157, 158, 237, 7, 146, 24,
        //240, 32, 177, 197, 94, 33, 2, 193, 175, 46, 160, 153, 115, 100, 174, 249,
        //255, 88, 184, 234, 198, 95, 87, 101, 8, 52, 85, 12, 10, 226, 25, 228,
        //231, 73, 51, 134, 60, 235, 141, 29, 190, 48, 98, 154, 150, 251, 118, 99
    };

    asf::uint8 h;
    asf::uint16 hh[2];
    const char* param_chars = param_name.c_str();
    const size_t len = param_name.length();

    for (size_t j = 0; j < 2; ++j)
    {
        h = T[(param_chars[0] + j) % 256];
        for (size_t i = 1; i < len; ++i)
        {
            h = T[h ^ param_chars[i]];
        }
        hh[j] = h;
    }
    
    asf::int16 hash = ((hh[0] << 8) | (hh[1] << 0)) & 0x7fff;
    return hash;
}
