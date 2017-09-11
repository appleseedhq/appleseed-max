
// Interface
#include "maxproctexturesource.h"

// appleseed.renderer headers.
#include "renderer/modeling/color/colorspace.h"

// appleseed.foundation headers.
#include "foundation/utility/siphash.h"

// 3ds max include
#include "imtl.h"
#include "maxapi.h"

namespace asf = foundation;
namespace asr = renderer;

//
// MaxSource class implementation.
//

MaxProceduralSource::MaxProceduralSource(
    Texmap* max_texmap)
    : asr::Source(false)
    , m_texmap(max_texmap)
{
}

MaxProceduralSource::~MaxProceduralSource()
{
}

inline void MaxProceduralSource::evaluate(
    asr::TextureCache&               texture_cache,
    const asf::Vector2f&             uv,
    float&                           scalar) const
{
    const asf::Color4f color = sample_max_texture(uv);
    scalar = color[0];
}

inline void MaxProceduralSource::evaluate(
    asr::TextureCache&               texture_cache,
    const asf::Vector2f&             uv,
    asf::Color3f&                    linear_rgb) const
{
    const asf::Color4f color = sample_max_texture(uv);
    linear_rgb = color.rgb();
}

inline void MaxProceduralSource::evaluate(
    asr::TextureCache&               texture_cache,
    const asf::Vector2f&             uv,
    asr::Spectrum&                   spectrum) const
{
    const asf::Color4f color = sample_max_texture(uv);
    spectrum[0] = color.rgb().r;
    spectrum[1] = color.rgb().g;
    spectrum[2] = color.rgb().b;
}

inline void MaxProceduralSource::evaluate(
    asr::TextureCache&               texture_cache,
    const asf::Vector2f&             uv,
    asr::Alpha&                      alpha) const
{
    const asf::Color4f color = sample_max_texture(uv);
    evaluate_alpha(color, alpha);
}

inline void MaxProceduralSource::evaluate(
    asr::TextureCache&               texture_cache,
    const asf::Vector2f&             uv,
    asf::Color3f&                    linear_rgb,
    asr::Alpha&                      alpha) const
{
    const asf::Color4f color = sample_max_texture(uv);
    linear_rgb = color.rgb();
    evaluate_alpha(color, alpha);
}

inline void MaxProceduralSource::evaluate(
    asr::TextureCache&               texture_cache,
    const asf::Vector2f&             uv,
    asr::Spectrum&                   spectrum,
    asr::Alpha&                      alpha) const
{
    const asf::Color4f color = sample_max_texture(uv);
    spectrum[0] = color.rgb().r;
    spectrum[1] = color.rgb().g;
    spectrum[2] = color.rgb().b;

    evaluate_alpha(color, alpha);
}

inline void MaxProceduralSource::evaluate_alpha(
    const asf::Color4f&              color,
    asr::Alpha&                      alpha) const
{
}

foundation::uint64 MaxProceduralSource::compute_signature() const
{
    return foundation::siphash24(m_texmap, sizeof(*m_texmap));
}

//--- Shade Context
class MaxProceduralSource::MaxSC
  :public ShadeContext 
{
  public:
    TimeValue curtime;
    Point3 ltPos; // position of point in light space
    Point3 view;  // unit vector from light to point, in light space
    Point3 dp;
    Point2 uv, duv;
    IPoint2 scrpos;
    float curve;
    int projType;

    BOOL InMtlEditor() { return false; }
    LightDesc* Light(int n) { return NULL; }
    TimeValue CurTime() { return GetCOREInterface()->GetTime(); }
    int NodeID() { return -1; }
    int FaceNumber() { return 0; }
    int ProjType() { return projType; }
    Point3 Normal() { return Point3(0, 0, 0); }
    Point3 GNormal() { return Point3(0, 0, 0); }
    Point3 ReflectVector() { return Point3(0, 0, 0); }
    Point3 RefractVector(float ior) { return Point3(0, 0, 0); }
    Point3 CamPos() { return Point3(0, 0, 0); }
    Point3 V() { return view; }
    void SetView(Point3 v) { view = v; }
    Point3 P() { return ltPos; }
    Point3 DP() { return dp; }
    Point3 PObj() { return ltPos; }
    Point3 DPObj() { return Point3(0, 0, 0); }
    Box3 ObjectBox() { return Box3(Point3(-1, -1, -1), Point3(1, 1, 1)); }
    Point3 PObjRelBox() { return view; }
    Point3 DPObjRelBox() { return Point3(0, 0, 0); }
    void ScreenUV(Point2& UV, Point2 &Duv) { UV = uv; Duv = duv; }
    IPoint2 ScreenCoord() { return scrpos; }
    Point3 UVW(int chan) { return Point3(uv.x, uv.y, 0.0f); }
    Point3 DUVW(int chan) { return Point3(duv.x, duv.y, 0.0f); }
    void DPdUVW(Point3 dP[3], int chan) {}  // dont need bump vectors
    void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG = TRUE) {}   // returns Background color, bg transparency
    float Curve() { return curve; }

    // Transform to and from internal space
    Point3 PointTo(const Point3& p, RefFrame ito) { return p; }
    Point3 PointFrom(const Point3& p, RefFrame ifrom) { return p; }
    Point3 VectorTo(const Point3& p, RefFrame ito) { return p; }
    Point3 VectorFrom(const Point3& p, RefFrame ifrom) { return p; }
    MaxSC() { doMaps = TRUE; curve = 0.0f; dp = Point3(0.0f, 0.0f, 0.0f); }
};

foundation::Color4f MaxProceduralSource::sample_max_texture(
    const foundation::Vector2f& uv) const
{
    MaxProceduralSource::MaxSC maxsc;

    maxsc.mode = SCMODE_NORMAL;
    maxsc.projType = 0; // 0: perspective, 1: parallel
    maxsc.curtime = GetCOREInterface()->GetTime();
    //maxsc.curve = curve;
    //maxsc.ltPos = plt;
    //maxsc.view = FNormalize(Point3(plt.x, plt.y, 0.0f));
    maxsc.uv.x = uv.x;
    maxsc.uv.y = uv.y;
    //maxsc.scrpos.x = (int)(x + 0.5);
    //maxsc.scrpos.y = (int)(y + 0.5);
    maxsc.filterMaps = false;
    maxsc.mtlNum = 1;
    //maxsc.globContext = sc.globContext;

    AColor color = m_texmap->EvalColor(maxsc);

    return foundation::Color4f(color.r, color.g, color.b, color.a);
}
