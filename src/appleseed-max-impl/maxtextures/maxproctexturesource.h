
// appleseed.renderer headers.
#include "renderer/modeling/input/maxsource.h"

// Forward declarations.
class Texmap;

class MaxProceduralSource
    : public renderer::MaxSource
{
public:
    // Constructor.
    MaxProceduralSource(
        Texmap* max_texmap);

    // Destructor.
    virtual ~MaxProceduralSource();

    virtual foundation::uint64 compute_signature() const;

    virtual foundation::Color4f sample_max_texture(
        const foundation::Vector2f& uv) const override;

private:
    // Max texture
    Texmap* m_texmap;

    // ShaderContext
    class MaxSC;
};
