
// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/modeling/input/source.h"

// Forward declarations.
class Texmap;

class MaxProceduralSource
    : public renderer::Source
{
public:
    // Constructor.
    MaxProceduralSource(
        Texmap* max_texmap);

    // Destructor.
    virtual ~MaxProceduralSource();

    virtual foundation::uint64 compute_signature() const;

    // Evaluate the source at a given shading point.
    virtual void evaluate(
        renderer::TextureCache&             texture_cache,
        const foundation::Vector2f&         uv,
        float&                              scalar) const override;
    virtual void evaluate(
        renderer::TextureCache&             texture_cache,
        const foundation::Vector2f&         uv,
        foundation::Color3f&                linear_rgb) const override;
    virtual void evaluate(
        renderer::TextureCache&             texture_cache,
        const foundation::Vector2f&         uv,
        renderer::Spectrum&                 spectrum) const override;
    virtual void evaluate(
        renderer::TextureCache&             texture_cache,
        const foundation::Vector2f&         uv,
        renderer::Alpha&                    alpha) const override;
    virtual void evaluate(
        renderer::TextureCache&             texture_cache,
        const foundation::Vector2f&         uv,
        foundation::Color3f&                linear_rgb,
        renderer::Alpha&                    alpha) const override;
    virtual void evaluate(
        renderer::TextureCache&             texture_cache,
        const foundation::Vector2f&         uv,
        renderer::Spectrum&                 spectrum,
        renderer::Alpha&                    alpha) const override;

    virtual foundation::Color4f sample_max_texture(
        const foundation::Vector2f& uv) const;

private:
    // Compute an alpha value given a linear RGBA color and the alpha mode of the texture instance.
    void evaluate_alpha(
        const foundation::Color4f&          color,
        renderer::Alpha&                              alpha) const;

    // Max texture
    Texmap* m_texmap;

    // ShaderContext
    class MaxSC;
};
