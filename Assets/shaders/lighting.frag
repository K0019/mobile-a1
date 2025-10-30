#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 projection;
} sceneData;

// Structures remain unchanged
struct Light {
    vec2 position;
    float lightAngle;
    float lightConeAngle;
};

struct LightProperties {
    vec3 color;
    float intensity;
    float radius;
    float innerRadius;
    float distanceFalloff;
    float coneFalloff;
    uint flags;
};

const uint MAX_LIGHTS = 64;
const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float EPSILON = 1e-6;
const float SHADOW_MAP_WIDTH = 1024.0;

// Flag definitions to match CPU-side
const uint LIGHT_FLAG_ENABLED = 1u << 0;
const uint LIGHT_FLAG_CAST_SHADOWS = 1u << 1;
const uint LIGHT_FLAG_SPOT = 1u << 2;

// Refined lighting parameters for pixel art aesthetics
const float FALLOFF_INTENSITY = 1.4;       // Enhanced falloff intensity for more distinct light regions
const float FALLOFF_POWER = 1.15;          // Fine-tune the exponent for more pixel-art appropriate falloff
const float EDGE_TRANSITION = 0.85;        // Controls the transition sharpness at light edges
const float EDGE_SOFTNESS = 0.15;          // Spotlight edge softness factor
const float BOUNDARY_FEATHER = 0.15;       // Light boundary feathering amount
const float BLOOM_INTENSITY = 0.18;        // Bloom contribution factor

// Soft shadow penumbra parameters
const float PENUMBRA_WIDTH = 0.03;         // Angular width for soft shadow sampling (in radians)
const int PENUMBRA_SAMPLES = 5;            // Number of angular samples for penumbra
const float PENUMBRA_STRENGTH = 0.85;      // Controls overall strength of the soft shadow effect
const float PENUMBRA_DISTANCE = 3.0;       // Distance-based transition rate into shadow

layout(set = 1, binding = 0) uniform sampler2D shadowMap;
layout(set = 1, binding = 1) uniform LightBuffer {
    Light lights[MAX_LIGHTS];
};
layout(set = 1, binding = 2) uniform LightPropertiesBuffer {
    LightProperties lightProps[MAX_LIGHTS];
};

// Unified mathematical model for soft shadow penumbra
float sampleSoftShadowPenumbra(vec2 lightVec, float worldDistance, int lightIndex) {
    // Base angle calculation with high-precision
    float baseAngle = atan(lightVec.y, lightVec.x);
    baseAngle = mod(baseAngle + TWO_PI, TWO_PI);
    
    // Optimization parameters
    const float PENUMBRA_WIDTH = 0.03;        // Angular width for sampling (radians)
    const int PENUMBRA_SAMPLES = 6;           // Minimal increase to even number for better distribution
    const float FILTER_SCALE = 0.48;          // Filter kernel scaling factor
    const float JITTER_AMOUNT = 0.4;          // Controlled jitter magnitude
    const float SMOOTHNESS_BIAS = 0.12;       // Transition bias for non-linear smoothing
    
    // Precompute filter weights using optimized Lanczos-2 approximation
    float weights[PENUMBRA_SAMPLES];
    float totalWeight = 0.0;
    
    for (int i = 0; i < PENUMBRA_SAMPLES; i++) {
        // Calculate Lanczos-windowed weight
        float x = (2.0 * float(i) / float(PENUMBRA_SAMPLES - 1) - 1.0) * FILTER_SCALE;
        
        // Fast Lanczos-2 approximation combined with Hann window
        float weight;
        if (abs(x) < 0.001) {
            weight = 1.0;
        } else if (abs(x) >= 2.0) {
            weight = 0.0;
        } else {
            // Optimized sinc × Lanczos window approximation
            weight = sin(PI * x) / (PI * x) * sin(PI * x * 0.5) / (PI * x * 0.5);
        }
        
        // Apply Hann window for better frequency response
        weight *= 0.5 + 0.5 * cos(PI * x * 0.5);
        
        weights[i] = weight;
        totalWeight += weight;
    }
    
    // Normalize weights for energy conservation
    for (int i = 0; i < PENUMBRA_SAMPLES; i++) {
        weights[i] /= totalWeight;
    }
    
    // Accumulate filtered visibility using jittered offsets
    float visibilitySum = 0.0;
    
    for (int i = 0; i < PENUMBRA_SAMPLES; i++) {
        // Standard sample position
        float t = float(i) / float(PENUMBRA_SAMPLES - 1);
        
        // Apply controlled jitter based on golden ratio to reduce banding
        // This creates a quasi-random distribution that better approximates continuous filtering
        float jitter = fract(t * 0.618033988749895) * 2.0 - 1.0;
        float offsetFactor = (t - 0.5) + jitter * JITTER_AMOUNT / float(PENUMBRA_SAMPLES);
        
        // Calculate angular offset with enhanced precision
        float offset = offsetFactor * PENUMBRA_WIDTH;
        float sampleAngle = mod(baseAngle + offset, TWO_PI);
        
        // Sample shadow map
        vec2 shadowUV = vec2(
            sampleAngle / TWO_PI,
            (float(lightIndex) + 0.5) / float(MAX_LIGHTS)
        );
        
        float shadowDistance = texture(shadowMap, shadowUV).r;
        float sampleDistToShadow = worldDistance - shadowDistance - 1.0;
        
        // Apply sigmoid-based transition function for smoother falloff
        float visibility;
        if (sampleDistToShadow <= -SMOOTHNESS_BIAS) {
            // Fully lit with slight transition zone
            visibility = 1.0;
        } else if (sampleDistToShadow >= 3.0 + SMOOTHNESS_BIAS) {
            // Fully shadowed with slight transition zone
            visibility = 0.0;
        } else {
            // Apply smooth sigmoid-based transition
            // This creates a C2-continuous transition function
            float t = (sampleDistToShadow + SMOOTHNESS_BIAS) / (3.0 + 2.0 * SMOOTHNESS_BIAS);
            visibility = 1.0 - smoothstep(0.0, 1.0, t);
            
            // Apply non-linear contrast adjustment for more natural falloff
            visibility = pow(visibility, 1.3);
        }
        
        // Accumulate weighted visibility
        visibilitySum += visibility * weights[i];
    }
    
    return visibilitySum;
}

// Spotlight mask with clean edge transition
float calculateSpotlightMask(Light light, LightProperties props, vec2 normalizedLightToFrag) {
    // Calculate angular deviation from spotlight direction vector
    float cosAngle = dot(normalizedLightToFrag, 
                       vec2(cos(light.lightAngle), sin(light.lightAngle)));
    float angleDiff = acos(cosAngle);
    
    // Normalize angle based on cone parameters
    float normalizedAngle = angleDiff / light.lightConeAngle;
    
    // Create a spotlight mask with controlled transition zone
    return 1.0 - smoothstep(1.0 - EDGE_SOFTNESS, 1.0, normalizedAngle);
}

// Pixel-art optimized attenuation function
float calculatePixelArtAttenuation(float normalizedDist, float distanceFalloff) {
    // Apply enhanced falloff intensity for more distinct regions
    float adjustedFalloff = distanceFalloff * FALLOFF_INTENSITY;
    
    // Base attenuation calculation
    float rawAttenuation = pow(1.0 - clamp(normalizedDist, 0.0, 1.0), adjustedFalloff);
    
    // Apply subtle power curve adjustment for pixel-art aesthetic
    // This creates slightly sharper transitions that better align with non-continuous art
    return pow(rawAttenuation, FALLOFF_POWER);
}

void main() {
    vec2 fragNDC = fragUV * 2.0 - 1.0;
    
    // Initialize accumulator for maximum-based composition
    vec3 maxLightContribution = vec3(0.0);
    
    vec2 projectionScale = vec2(
        sceneData.projection[0][0],
        sceneData.projection[1][1]
    );
    
    for(int i = 0; i < MAX_LIGHTS; i++) {
        Light light = lights[i];
        LightProperties props = lightProps[i];
        
        // Early exit if light is disabled
        if((props.flags & LIGHT_FLAG_ENABLED) == 0) continue;
        
        // Transform positions
        vec4 lightNDC = sceneData.projection * vec4(light.position, 0.0, 1.0);
        vec2 lightPosNDC = lightNDC.xy;
        
        vec2 lightToFrag = (fragNDC - lightPosNDC) / projectionScale;
        float distance = length(lightToFrag);
        
        // Early discard fragments beyond maximum radius (optimization)
        if (distance > props.radius * 1.05) continue;
        
        // Process spotlight mask first if applicable
        float spotMask = 1.0;
        if((props.flags & LIGHT_FLAG_SPOT) != 0) {
            spotMask = calculateSpotlightMask(light, props, normalize(lightToFrag));
            
            // Early exit if outside spotlight cone (optimization)
            if (spotMask < EPSILON) continue;
        }
        
        // Calculate normalized distance for attenuation
        float normalizedDist = max(0.0, distance - props.innerRadius) / 
                              max(props.radius - props.innerRadius, EPSILON);
        
        // Apply pixel-art optimized attenuation
        float attenuation = calculatePixelArtAttenuation(normalizedDist, props.distanceFalloff);
        
        // Apply spotlight mask
        attenuation *= spotMask;
        
        // Apply improved boundary feathering with sharper profile for pixel art
        float edgeDistance = clamp(1.0 - (normalizedDist / (1.0 + BOUNDARY_FEATHER)), 0.0, 1.0);
        float edgeFactor = smoothstep(0.0, BOUNDARY_FEATHER * EDGE_TRANSITION, edgeDistance);
        attenuation *= edgeFactor;
        
        // Apply soft shadow penumbra
        float visibility = 1.0;
        if((props.flags & LIGHT_FLAG_CAST_SHADOWS) != 0) {
            visibility = sampleSoftShadowPenumbra(lightToFrag, distance, i);
        }
        
        // Compute final light contribution with shadow visibility
        vec3 lightColor = props.color * (props.intensity * attenuation * visibility);
        
        // Apply bloom-friendly enhancement while maintaining energy conservation
        if (attenuation > 0.0 && visibility > 0.01) {
            float bloomBoost = 1.0 + pow(attenuation, 1.2) * BLOOM_INTENSITY;
            lightColor *= bloomBoost;
        }
        
        // CRITICAL CHANGE: Strict maximum-based light composition
        // This ensures identical lights don't create cumulative brightness increases
        maxLightContribution = max(maxLightContribution, lightColor);
    }
    
    // Final output - direct maximum contribution without additional processing
    outColor = vec4(maxLightContribution, 1.0);
}