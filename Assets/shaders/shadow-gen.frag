#version 450

layout(location = 0) in vec4 fsEdge;
layout(location = 0) out float fragDistance;

layout(push_constant) uniform LightParams {
    vec2 lightPosition;
    float lightAngle;
    float lightConeAngle;
} light;

const float EPSILON = 1e-6;
const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float THREE_PI = 9.42477796077;
const uint SHADOW_MAP_WIDTH = 1024u;

void main() {
    // Calculate ray angle from fragment position
    float normalizedX = gl_FragCoord.x / float(SHADOW_MAP_WIDTH);
    float rayAngle = normalizedX * THREE_PI;
    
    // Compute ray direction
    vec2 rayDir = vec2(cos(rayAngle), sin(rayAngle));
    
    // Extract edge vertices
    vec2 edgeStart = fsEdge.xy;
    vec2 edgeEnd = fsEdge.zw;
    
    // Calculate edge vector
    vec2 edgeVec = edgeEnd - edgeStart;
    float edgeLength = length(edgeVec);
    
    // Handle degenerate edges (points)
    if (edgeLength < EPSILON) {
        fragDistance = length(edgeStart - light.lightPosition);
        return;
    }
    
    // Calculate normalized edge direction and perpendicular
    vec2 edgeDir = edgeVec / edgeLength;
    vec2 perpDir = vec2(-edgeDir.y, edgeDir.x);
    
    // Compute ray-edge intersection
    float denom = dot(rayDir, perpDir);
    
    // If ray is parallel to edge, discard
    if (abs(denom) < EPSILON) {
        discard;
        return;
    }
    
    // Calculate intersection parameter
    vec2 toStart = edgeStart - light.lightPosition;
    float t = dot(toStart, perpDir) / denom;
    
    // If intersection is behind light, discard
    if (t < 0.0) {
        discard;
        return;
    }
    
    // Calculate intersection point
    vec2 intersection = light.lightPosition + rayDir * t;
    
    // Validate intersection is within edge bounds
    vec2 toIntersect = intersection - edgeStart;
    float alongEdge = dot(toIntersect, edgeDir);
    
    // CRITICAL FIX: If intersection is outside edge bounds, discard
    if (alongEdge < 0.0 || alongEdge > edgeLength) {
        discard;
        return;
    }
    
    // Only valid intersections reach this point - record the distance
    fragDistance = t;
    
    // Apply light cone restriction if specified
    if (light.lightConeAngle < PI) {
        float angleDiff = abs(mod(rayAngle - light.lightAngle + THREE_PI, TWO_PI) - PI);
        if (angleDiff > light.lightConeAngle) {
            discard;
        }
    }
}