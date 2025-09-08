#pragma once

class TrailRendererComponent
    : public IRegisteredComponent<TrailRendererComponent>
    , public IEditorComponent<TrailRendererComponent>
{
public:
    // Constants
    static constexpr size_t MAX_TRAIL_POINTS = 128;

    struct TrailPoint {
        Vec3 position;    // World position when recorded
        float age;           // Current age of this point (in seconds)
    };

        // Glow configuration structure
    struct GlowSettings {
        bool enabled = false;             // Whether the glow effect is enabled
        Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);  // Glow color (separate from trail color)
        float radius = 2.0f;              // How much wider the glow is than the trail (multiplier)
        float intensity = 1.0f;           // Brightness of the glow
        float decay = 0.5f;               // How quickly glow fades along trail (0-1, higher = faster decay)
    };

    TrailRendererComponent();
    
    TrailRendererComponent(
        float minVertexDistance,
        float lifetime,
        float startWidth,
        float endWidth,
        const Vec4& startColor,
        const Vec4& endColor
    );

    // Core properties
    bool enabled = true;         // Whether trail is visible
    bool emitting = true;        // Whether new points are being added
    float minVertexDistance = 0.1f; // Minimum distance to add new points
    float lifetime = 1.0f;       // How long points last before fading
    int maxPoints = 100;         // Maximum number of points to store (clamped to MAX_TRAIL_POINTS)
    float startWidth = 1.0f;     // Width at start
    float endWidth = 0.1f;       // Width at end
    Vec4 startColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f); // Start color (RGBA)
    Vec4 endColor = Vec4(1.0f, 1.0f, 1.0f, 0.0f);   // End color (RGBA)
    float smoothing = 0.01f;     // Edge smoothing factor

    GlowSettings glow;
    // Public methods
    void ClearTrail();

    bool IsEmitting() const;

    void SetEmitting(bool value);

    bool IsEnabled() const;

    void SetEnabled(bool value);

    void SetStartColor(const Vec4& color);

    void SetEndColor(const Vec4& color);

    void SetStartWidth(float width);

    void SetEndWidth(float width);

    // Get point at index (handles circular buffer wrapping)
    const TrailPoint& GetPoint(int index) const;

    // Add a new point to the trail
    void AddPoint(const Vec3& position);

    // Remove points older than lifetime
    void RemoveExpiredPoints();

    // Update ages of all points
    void UpdateAges(float dt);

    // Calculate properties based on lifetime percentage (optimization: using direct formula)
    float CalculateWidth(float agePercent) const;

    Vec4 CalculateColor(float agePercent) const;

    // Get point count
    int GetPointCount() const;

    // Runtime data
    Vec3 m_lastPosition;
    
private:
    // Fixed-size circular buffer of points
    TrailPoint m_points[MAX_TRAIL_POINTS];
    int m_pointCount;
    int m_headIndex;
    
    virtual void EditorDraw() override;
    
    property_vtable()
};

// Property reflection
property_begin(TrailRendererComponent)
{
    property_var(enabled),
    property_var(emitting),
    property_var(minVertexDistance),
    property_var(lifetime),
    property_var(maxPoints),
    property_var(startWidth),
    property_var(endWidth),
    property_var(startColor),
    property_var(endColor),
    property_var(smoothing),
    property_var(glow.enabled),
    property_var(glow.color),
    property_var(glow.radius),
    property_var(glow.intensity),
    property_var(glow.decay)
}
property_vend_h(TrailRendererComponent)