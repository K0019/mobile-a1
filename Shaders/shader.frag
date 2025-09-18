#version 450
#extension GL_EXT_nonuniform_qualifier : require

// Render flags
#define RENDER_FLAG_TEXT     0x01u
#define RENDER_FLAG_SOLID    0x02u
#define RENDER_FLAG_TRAIL    0x04u

// Common inputs - used by all paths
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint fragTexIndex;
layout(location = 3) flat in uint fragFlags;

// Path-specific inputs
layout(location = 4) in vec2 fragQuadPos;       // Trail: quad position
layout(location = 5) in vec2 fragStartPoint;    // Trail: start point
layout(location = 6) in vec2 fragEndPoint;      // Trail: end point
layout(location = 7) in float fragStartWidth;   // Trail: start width, Text: smoothing
layout(location = 8) in float fragEndWidth;     // Trail: end width, Sprite: alpha scale
layout(location = 9) in vec4 fragEndColor;      // Trail: end color

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

layout(push_constant) uniform PushConstants {
    vec4 outlineColor;
    vec4 shadowColor;
    vec2 shadowOffset;
    float outlineWidth;
    float smoothing;
} pc;

// Safe texture sampling function to prevent nonuniform access issues
vec4 sampleTexture(uint texIndex, vec2 texCoord) {
    // Early-out for invalid texture indices
    if (texIndex == 0xFFFFFFFFu) {
        return vec4(0.0);
    }
    
    // Using nonuniformEXT safely by ensuring it's only called once per sampling
    uint safeIndex = nonuniformEXT(texIndex);
    return texture(texSamplers[safeIndex], texCoord);
}

// MSDF utilities
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

// Trail rendering function
vec4 renderTrail() {
    // Get world coordinates of the trail points
    vec2 startPoint = fragStartPoint;
    vec2 endPoint = fragEndPoint;
    float startWidth = fragStartWidth;
    float endWidth = fragEndWidth;
    
    // Get the position of this fragment in the quad's local space [-0.5, 0.5]
    vec2 localPos = fragQuadPos * 2.0 - 1.0; // Convert [0,1] to [-1,1]
    localPos *= 0.5;                        // Scale to [-0.5, 0.5]
    
    // Calculate position along trail (t) and perpendicular offset
    float t = localPos.x + 0.5; // Map [-0.5, 0.5] to [0, 1]
    float perpOffset = localPos.y; // Already in [-0.5, 0.5]
    
    // Discard if outside the width bounds
    if (abs(perpOffset) > 0.5) {
        discard;
    }
    
    // Interpolate color
    vec4 color = mix(fragColor, fragEndColor, t);
    
    // Apply edge smoothing
    float edgeDist = abs(perpOffset) * 2.0; // Map [0, 0.5] to [0, 1]
    float smoothingFactor = float(fragTexIndex) / 65535.0;
    color.a *= smoothstep(1.0, 1.0 - smoothingFactor * 2.0, edgeDist);
    
    return color;
}

// Sprite rendering function
vec4 renderSprite(bool useSolidColor) {
    // Safely sample the texture
    vec4 texColor = sampleTexture(fragTexIndex, fragTexCoord);
    
    vec4 finalColor;
    
    if (useSolidColor) {
        // Apply solid color while preserving original alpha
        // Use the cached alpha value from vertex shader
        finalColor = vec4(fragColor.rgb, texColor.a * fragEndWidth);
    } else {
        // Regular sprite rendering with cached colors
        finalColor = texColor * fragColor;
        
        // Pre-multiply alpha
        finalColor.rgb *= finalColor.a;
    }
    
    return finalColor;
}

// Text rendering function
vec4 renderText() {
    // Sample the signed distance from the MTSDF - use safe sampling
    vec3 mtsdf = sampleTexture(fragTexIndex, fragTexCoord).rgb;
    float sd = median(mtsdf.r, mtsdf.g, mtsdf.b);
    
    // Calculate the screen-space scale factor
    vec2 unitRange = vec2(fwidth(fragTexCoord.x), fwidth(fragTexCoord.y));
    float pixelDist = pc.outlineWidth * max(unitRange.x, unitRange.y);
    
    // Use smoothing from vertex shader instead of from push constants when possible
    float textSmoothing = max(fragStartWidth, pc.smoothing);
    
    // Base text
    float textEdge = 0.5;
    float textAlpha = smoothstep(textEdge - textSmoothing, textEdge + textSmoothing, sd);
    
    // Outline
    float outlineEdge = textEdge - pixelDist;
    float outlineAlpha = smoothstep(outlineEdge - textSmoothing, outlineEdge + textSmoothing, sd);
    
    // Shadow - using safe texture sampling
    vec2 shadowUV = fragTexCoord - pc.shadowOffset * unitRange;
    vec3 shadowMTSDF = sampleTexture(fragTexIndex, shadowUV).rgb;
    float shadowDist = median(shadowMTSDF.r, shadowMTSDF.g, shadowMTSDF.b);
    float shadowAlpha = smoothstep(textEdge - textSmoothing, textEdge + textSmoothing, shadowDist);
    
    // Combine effects
    vec4 shadowColor = vec4(pc.shadowColor.rgb, pc.shadowColor.a * shadowAlpha);
    vec4 outlineColor = vec4(pc.outlineColor.rgb, pc.outlineColor.a * outlineAlpha);
    vec4 textColor = vec4(fragColor.rgb, fragColor.a * textAlpha);
    
    // Layer the effects
    vec4 result = mix(shadowColor, outlineColor, outlineAlpha);
    result = mix(result, textColor, textAlpha);
    
    return result;
}

void main() {
    // Determine element type flags
    bool isText = (fragFlags & RENDER_FLAG_TEXT) != 0;
    bool isSolid = (fragFlags & RENDER_FLAG_SOLID) != 0 || (fragStartWidth > 0.5);
    bool isTrail = (fragFlags & RENDER_FLAG_TRAIL) != 0;
    
    // Avoid nested if-else branches to reduce divergence
    // Each path computes its result independently
    vec4 result = vec4(0.0);
    
    if (isTrail) {
        result = renderTrail();
    }
    else if (isText) {
        result = renderText();
    }
    else {
        result = renderSprite(isSolid);
    }
    
    outColor = result;
}