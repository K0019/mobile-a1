#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D lightingResult;
layout(push_constant) uniform PushConstants {
    float threshold;
} pc;

void main() {
    vec4 color = texture(lightingResult, fragTexCoord);
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > pc.threshold) {
        outColor = vec4(color.rgb * (brightness - pc.threshold), 1.0);
    } else {
        outColor = vec4(0.0);
    }
}