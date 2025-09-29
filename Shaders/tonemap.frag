layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform ToneMappingData {
    uint hdrSceneColorIndex;
    float exposure;
    int mode;
    float maxWhite;
    float P, a, m, l, c, b; // Uchimura parameters
} pc;

// Corrected ACES implementation
vec3 ACESFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Corrected Reinhard implementation
vec3 Reinhard(vec3 color) {
    // Formula: color / (1 + color / maxWhite)
    return color / (1.0 + color / pc.maxWhite);
}

// Corrected Uchimura implementation - simplified but accurate
vec3 Uchimura(vec3 x) {
    float P = pc.P;  // max display brightness
    float a = pc.a;  // contrast
    float m = pc.m;  // linear section start
    float l = pc.l;  // linear section length
    float c = pc.c;  // black tightness
    float b = pc.b;  // pedestal
    
    // Pre-compute constants
    float l0 = ((P - m) * l) / a;
    float L0 = m - (m / a);
    float L1 = m + ((1.0 - m) / a);
    float S0 = m + l0;
    float S1 = m + (a * l0);
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    vec3 result;
    for(int i = 0; i < 3; i++) {
        float val = x[i];
        
        if(val < m) {
            // Toe section
            result[i] = m * pow(val / m, c) + b;
        }
        else if(val < m + l0) {
            // Linear section
            result[i] = m + a * (val - m);
        }
        else {
            // Shoulder section
            result[i] = P - (P - S1) * exp(CP * (val - S0));
        }
    }
    
    return result;
}

vec3 Uncharted2(vec3 x) {
    const float A = 0.15; // Shoulder strength
    const float B = 0.50; // Linear strength  
    const float C = 0.10; // Linear angle
    const float D = 0.20; // Toe strength
    const float E = 0.02; // Toe numerator
    const float F = 0.30; // Toe denominator
    
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 Unreal(vec3 x) {
    return x / (x + 0.155) * 1.019;
}

// Corrected Khronos PBR Neutral implementation (from official spec)
vec3 KhronosPBR(vec3 color) {
    const float startCompression = 0.76; // 0.8 - 0.04
    const float desaturation = 0.15;
    
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    
    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;
    
    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;
    
    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, vec3(newPeak), g);
}

void main() {
    // Sample HDR color
    vec3 hdrColor = textureBindless2D(pc.hdrSceneColorIndex, 0, uv).rgb;
    
    // Apply exposure
    hdrColor *= pc.exposure;
    
    // Apply tone mapping
    vec3 toneMapped;
    switch (pc.mode) {
        case 1: // Reinhard
            toneMapped = Reinhard(hdrColor);
            break;
        case 2: // Uchimura
            toneMapped = Uchimura(hdrColor);
            break;
        case 3: // ACES
            toneMapped = ACESFilm(hdrColor);
            break;
        case 4: // Khronos PBR
            toneMapped = KhronosPBR(hdrColor);
            break;
        case 5: // Uncharted 2
            toneMapped = Uncharted2(hdrColor);
            break;
        case 6: // Unreal
            toneMapped = Unreal(hdrColor);
            // Skip gamma correction for Unreal as it's baked in
            outColor = vec4(toneMapped, 1.0);
            return;
        default: // None
            toneMapped = clamp(hdrColor, 0.0, 1.0);
            break;
    }
    
    // Gamma correction (sRGB conversion)
    toneMapped = pow(toneMapped, vec3(1.0 / 2.2));
    
    outColor = vec4(toneMapped, 1.0);
}