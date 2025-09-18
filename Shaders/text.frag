#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint fragTexIndex;

layout(push_constant) uniform PushConstants {
    vec4 outlineColor;
    vec4 shadowColor;
    vec2 shadowOffset;
    float outlineWidth;
    float smoothing;
} pc;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float sampleMTSDF(vec2 uv) {
    vec3 mtsdf = texture(texSamplers[nonuniformEXT(fragTexIndex)], uv).rgb;
    return median(mtsdf.r, mtsdf.g, mtsdf.b);
}

void main() {
    // Sample the signed distance from the MTSDF
    float sd = sampleMTSDF(fragTexCoord);

    // Calculate the screen-space scale factor
    vec2 unitRange = vec2(fwidth(fragTexCoord.x), fwidth(fragTexCoord.y));
    float pixelDist = pc.outlineWidth * max(unitRange.x, unitRange.y);

    // Base text
    float textEdge = 0.5;
    float textAlpha = smoothstep(textEdge - pc.smoothing, textEdge + pc.smoothing, sd);

    // Outline
    float outlineEdge = textEdge - pixelDist;
    float outlineAlpha = smoothstep(outlineEdge - pc.smoothing, outlineEdge + pc.smoothing, sd);

    // Shadow
    vec2 shadowUV = fragTexCoord - pc.shadowOffset * unitRange;
    float shadowDist = sampleMTSDF(shadowUV);
    float shadowAlpha = smoothstep(textEdge - pc.smoothing, textEdge + pc.smoothing, shadowDist);

    // Combine effects
    vec4 shadowColor = vec4(pc.shadowColor.rgb, pc.shadowColor.a * shadowAlpha);
    vec4 outlineColor = vec4(pc.outlineColor.rgb, pc.outlineColor.a * outlineAlpha);
    vec4 textColor = vec4(fragColor.rgb, fragColor.a * textAlpha);

    // Layer the effects
    vec4 result = mix(shadowColor, outlineColor, outlineAlpha);
    result = mix(result, textColor, textAlpha);

    outColor = result;
}