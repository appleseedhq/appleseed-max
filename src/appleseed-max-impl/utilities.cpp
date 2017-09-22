
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
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
#include "main.h"

// appleseed.renderer headers.
#include "renderer/api/color.h"
#include "renderer/api/source.h"
#include "renderer/api/texture.h"

// appleseed.foundation headers.
#include "foundation/utility/searchpaths.h"
#include "foundation/utility/siphash.h"
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <AssetManagement/AssetUser.h>
#include <assert1.h>
#include <bitmap.h>
#include <imtl.h>
#include <maxapi.h>
#include <plugapi.h>
#include <stdmat.h>

// Windows headers.
#include <Shlwapi.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    class MaxShadeContext
      : public ShadeContext
    {
      public:
        TimeValue   cur_time;
        Point2      duv;
        Point2      uv;
        int         proj_type;

        MaxShadeContext()
          : curve(0.0f)
          , dp(Point3(0.0f, 0.0f, 0.0f))
        {
            doMaps = TRUE;
        }

        MaxShadeContext(const asf::Vector2f& tex_uv)
        {
            uv.x = tex_uv.x;
            uv.y = tex_uv.y;
            curve = 0.0f;
            dp = Point3(0.0f, 0.0f, 0.0f);
            mode = SCMODE_NORMAL;
            proj_type = 0;        // 0: perspective, 1: parallel
            cur_time = GetCOREInterface()->GetTime();
            filterMaps = false;
            mtlNum = 1;
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
            return cur_time;
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
            return proj_type;
        }

        Point3 Normal() override
        {
            return Point3(0, 0, 0);
        }

        Point3 GNormal() override
        {
            return Point3(0, 0, 0);
        }

        Point3 ReflectVector() override
        {
            return Point3(0, 0, 0);
        }

        Point3 RefractVector(float ior) override
        {
            return Point3(0, 0, 0);
        }

        Point3 CamPos() override
        {
            return Point3(0, 0, 0);
        }

        Point3 V() override
        {
            return view;
        }

        void SetView(Point3 v) override
        {
            view = v;
        }

        Point3 P() override
        {
            return light_pos;
        }

        Point3 DP() override
        {
            return dp;
        }

        Point3 PObj() override
        {
            return light_pos;
        }

        Point3 DPObj() override
        {
            return Point3(0, 0, 0);
        }

        Box3 ObjectBox() override
        {
            return Box3(Point3(-1, -1, -1), Point3(1, 1, 1));
        }

        Point3 PObjRelBox() override
        {
            return view;
        }

        Point3 DPObjRelBox() override
        {
            return Point3(0, 0, 0);
        }

        void ScreenUV(Point2& UV, Point2 &Duv) override
        {
            UV = uv;
            Duv = duv;
        }

        IPoint2 ScreenCoord() override
        {
            return scr_pos;
        }

        Point3 UVW(int chan) override
        {
            return Point3(uv.x, uv.y, 0.0f);
        }

        Point3 DUVW(int chan) override
        {
            return Point3(duv.x, duv.y, 0.0f);
        }

        void DPdUVW(Point3 dP[3], int chan) override
        {
        }

        void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG = TRUE)
        {
        }

        float Curve() override
        {
            return curve;
        }

        Point3 PointTo(const Point3& p, RefFrame ito)
        {
            return p;
        }

        Point3 PointFrom(const Point3& p, RefFrame ifrom)
        {
            return p;
        }

        Point3 VectorTo(const Point3& p, RefFrame ito)
        {
            return p;
        }

        Point3 VectorFrom(const Point3& p, RefFrame ifrom)
        {
            return p;
        }

      private:
        Point3      light_pos;   // Position of point in light space.
        Point3      view;        // Unit vector from light to point, in light space.
        IPoint2     scr_pos;
        Point3      dp;
        float       curve;
    };
}

namespace
{
    class MaxProceduralTextureSource
      : public asr::Source
    {
      public:
        MaxProceduralTextureSource(Texmap* texmap)
            : asr::Source(false)
            , m_texmap(texmap)
        {
        }

        virtual asf::uint64 compute_signature() const override
        {
            return asf::siphash24(m_texmap);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            float&                      scalar) const override
        {
            scalar = evaluate_float(uv);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asf::Color3f&               linear_rgb) const override
        {
            evaluate_color(uv, linear_rgb.r, linear_rgb.g, linear_rgb.b);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asr::Spectrum&              spectrum) const override
        {
            DbgAssert(spectrum.size() == 3);
            asf::Color3f color;
            evaluate_color(uv, spectrum[0], spectrum[1], spectrum[2]);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asr::Alpha&                 alpha) const override
        {
            alpha.set(evaluate_float(uv));
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asf::Color3f&               linear_rgb,
            asr::Alpha&                 alpha) const override
        {
            evaluate_color(uv, linear_rgb.r, linear_rgb.g, linear_rgb.b, &alpha);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asr::Spectrum&              spectrum,
            asr::Alpha&                 alpha) const override
        {
            DbgAssert(spectrum.size() == 3);
            asf::Color3f color;
            evaluate_color(uv, spectrum[0], spectrum[1], spectrum[2], &alpha);
        }

      private:
        float evaluate_float(const asf::Vector2f& uv) const
        {
            MaxShadeContext maxsc(uv);

            return m_texmap->EvalMono(maxsc);
        }

        void evaluate_color(const asf::Vector2f& uv, float& r, float& g, float& b, asr::Alpha* alpha = nullptr) const
        {
            MaxShadeContext maxsc(uv);
            
            AColor tex_color = m_texmap->EvalColor(maxsc);

            r = tex_color.r;
            g = tex_color.g;
            b = tex_color.b;

            if (alpha)
                alpha->set(tex_color.a);
        }
        
        Texmap*     m_texmap;
    };

    class MaxProceduralTexture
      : public asr::Texture
    {
      public:
        MaxProceduralTexture(const char* name, Texmap* texmap)
            : asr::Texture(name, asr::ParamArray())
            , m_texmap(texmap)
        {
            m_properties = asf::CanvasProperties(512, 512, 64, 64, 3, asf::PixelFormat::PixelFormatUInt8); // Dummy values.
        }

        virtual void release() override
        {
            delete this;
        }

        virtual const char* get_model() const override
        {
            return "max_procedural_texture";
        }

        virtual asf::ColorSpace get_color_space() const override
        {
            return asf::ColorSpaceLinearRGB;
        }

        virtual const asf::CanvasProperties& properties() override
        {
            return m_properties;
        }

        virtual asr::Source* create_source(
            const asf::UniqueID         assembly_uid,
            const asr::TextureInstance& texture_instance) override
        {
            return new MaxProceduralTextureSource(m_texmap);
        }

        virtual asf::Tile* load_tile(
            const size_t                tile_x,
            const size_t                tile_y) override
        {
            return nullptr;
        }

        virtual void unload_tile(
            const size_t                tile_x,
            const size_t                tile_y,
            const asf::Tile*            tile) override
        {
        }

      private:
        asf::CanvasProperties   m_properties;
        Texmap*                 m_texmap;
    };
}

namespace
{
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
            if (sub_tex)
                load_map_files_recursively(sub_tex, time);
        }
    }
}

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

bool is_supported_texture(Texmap* map)
{
    if (map == nullptr)
        return false;

    auto part_a = map->ClassID().PartA();
    auto part_b = map->ClassID().PartB();

    switch (part_a)
    {
      case 0x64035FB9:              // Tiles.
        return part_b == 0x69664CDC;
      case 0x1DEC5B86:              // Gradient Ramp.
        return part_b == 0x43383A51;
      case 0x72C8577F:              // Swirl.
        return part_b == 0x39A00A1B;
      case 0x23AD0AE9:              // Perlin Marble.
        return part_b == 0x158D7A88;
      case 0x243E22C6:              // Normal Bump.
        return part_b == 0x63F6A014;
      case 0x93A92749:              // Vector Map.
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
      case 0x0000214:               // WOOD_CLASS_ID
      case 0x0000218:               // DENT_CLASS_ID
      case 0x46396cf1:              // PLANET_CLASS_ID
      case 0x7712634e:              // WATER_CLASS_ID
      case 0xa845e7c:               // SMOKE_CLASS_ID
      case 0x62c32b8a:              // SPECKLE_CLASS_ID
      case 0x90b04f9:               // SPLAT_CLASS_ID
        return true;
    }

    return false;
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
    bool            use_max_source,
    asr::ParamArray texture_params,
    asr::ParamArray texture_instance_params)
{
    std::string texture_instance_name;
    std::string texture_name;

    if (use_max_source)
    {
        if (is_supported_texture(texmap))
        {
            TimeValue curr_time = GetCOREInterface()->GetTime();

            texmap->Update(curr_time, FOREVER);
            load_map_files_recursively(texmap, curr_time);

            if (!texture_params.strings().exist("color_space"))
            {
                // todo: should probably check max's gamma settings here.
                texture_params.insert("color_space", "linear_rgb");
            }

            texture_name = wide_to_utf8(texmap->GetName());
            if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
            {
                base_group.textures().insert(
                    asf::auto_release_ptr<asr::Texture>(
                        new MaxProceduralTexture(
                            texture_name.c_str(), texmap)));
            }
        }
    }
    else
    {
        if (is_bitmap_texture(texmap))
        {
            BitmapTex* bitmap_tex = static_cast<BitmapTex*>(texmap);

            const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());
            texture_params.insert("filename", filepath);

            if (!texture_params.strings().exist("color_space"))
            {
                if (asf::ends_with(filepath, ".exr"))
                    texture_params.insert("color_space", "linear_rgb");
                else texture_params.insert("color_space", "srgb");
            }

            texture_name = wide_to_utf8(texmap->GetName());
            if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
            {
                base_group.textures().insert(
                    asr::DiskTexture2dFactory::static_create(
                        texture_name.c_str(),
                        texture_params,
                        asf::SearchPaths()));
            }
        }
    }

    if (!texture_name.empty())
    {
        texture_instance_name = texture_name + "_inst";
        if (base_group.texture_instances().get_by_name(texture_instance_name.c_str()) == nullptr)
        {
            base_group.texture_instances().insert(
                asr::TextureInstanceFactory::create(
                    texture_instance_name.c_str(),
                    texture_instance_params,
                    texture_name.c_str()));
        }
    }

    return texture_instance_name;
}

std::string insert_bitmap_texture_and_instance(
    asr::BaseGroup& base_group,
    Texmap*         texmap,
    asr::ParamArray texture_params,
    asr::ParamArray texture_instance_params)
{
    BitmapTex* bitmap_tex = static_cast<BitmapTex*>(texmap);

    const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());
    texture_params.insert("filename", filepath);

    if (!texture_params.strings().exist("color_space"))
    {
        if (asf::ends_with(filepath, ".exr"))
            texture_params.insert("color_space", "linear_rgb");
        else texture_params.insert("color_space", "srgb");
    }

    const std::string texture_name = wide_to_utf8(texmap->GetName());
    if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
    {
        base_group.textures().insert(
            asr::DiskTexture2dFactory::static_create(
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

WStr replace_extension(const WStr& file_path, const WStr& new_ext)
{
    const int i = file_path.last(L'.');
    WStr new_file_path = i == -1 ? file_path : file_path.Substr(0, i);
    new_file_path.Append(new_ext);
    return new_file_path;
}
