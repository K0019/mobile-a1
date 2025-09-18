#version 450

// Render flags - matching the fragment shader
#define RENDER_FLAG_TEXT     0x01u
#define RENDER_FLAG_SOLID    0x02u
#define RENDER_FLAG_TRAIL    0x04u

// Vertex attributes
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat4 inInstanceMatrix;
layout(location = 6) in vec4 inInstanceColor;
layout(location = 7) in vec4 inTexCoords;
layout(location = 8) in uint inTexIndex;
layout(location = 9) in uint inFlags;

// Common outputs - used by all paths
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out uint fragTexIndex;
layout(location = 3) flat out uint fragFlags;

// Shared outputs with specialized meanings per path
layout(location = 4) out vec2 fragQuadPos;       // All: Original quad position
layout(location = 5) out vec2 fragStartPoint;    // Trail: Start point, Text/Sprite: UV origin
layout(location = 6) out vec2 fragEndPoint;      // Trail: End point, Text/Sprite: UV scale/max
layout(location = 7) out float fragStartWidth;   // Trail: Start width, Text: Smoothing, Sprite: Solid color flag
layout(location = 8) out float fragEndWidth;     // Trail: End width, Text: Outline scale, Sprite: Alpha
layout(location = 9) out vec4 fragEndColor;      // Trail: End color, Text: Shadow params, Sprite: Original color

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 projection;
} sceneData;

// Helper function to extract trail parameters from instance matrix
void extractTrailData(mat4 matrix, out vec2 startPoint, out vec2 endPoint, 
                     out float startWidth, out float endWidth,
                     out vec2 startJointPerp, out vec2 endJointPerp) {
    startPoint = vec2(matrix[0][3], matrix[1][3]);
    endPoint = vec2(matrix[2][3], matrix[2][0]);
    startWidth = matrix[2][1];
    endWidth = matrix[2][2];
    
    // Extract joint information from matrix's unused parts
    startJointPerp = vec2(matrix[0][1], matrix[0][2]);
    endJointPerp = vec2(matrix[1][0], matrix[1][2]);
}

// Process trail vertex
void processTrailVertex() {
    // Extract trail data from instance matrix
    vec2 startPoint, endPoint, startJointPerp, endJointPerp;
    float startWidth, endWidth;
    extractTrailData(inInstanceMatrix, startPoint, endPoint, startWidth, endWidth,
                    startJointPerp, endJointPerp);
    
    // Position along trail [0, 1]
    float t = inPosition.x + 0.5;
    
    // Side multiplier (-1 for left edge, +1 for right edge)
    float side = sign(inPosition.y);
    
    // Calculate width at this position
    float width = mix(startWidth, endWidth, t);
    
    // Calculate world position along the trail centerline
    vec2 centerPos = mix(startPoint, endPoint, t);
    
    // Interpolate between start and end joint perpendicular directions
    vec2 perpDir = normalize(mix(startJointPerp, endJointPerp, t));
    
    // Apply the offset using the joint-aware perpendicular direction
    vec2 finalPos = centerPos + perpDir * (side * width * 0.5);
    
    // Set the position with depth from instance matrix
    gl_Position = sceneData.projection * vec4(finalPos, inInstanceMatrix[3][2], 1.0);
    
    // Pass data to fragment shader
    fragQuadPos = inTexCoord;
    fragColor = inInstanceColor;
    fragEndColor = inTexCoords;
    fragStartPoint = startPoint;
    fragEndPoint = endPoint;
    fragStartWidth = startWidth;
    fragEndWidth = endWidth;
    fragTexIndex = inTexIndex;
    
    // Set default texture coordinates (not used for trails)
    fragTexCoord = vec2(0.0);
}

// Process text vertex
void processTextVertex() {
    // Calculate texture coordinates
    fragTexCoord = inTexCoords.xy + inTexCoord * inTexCoords.zw;
    fragColor = inInstanceColor;
    fragTexIndex = inTexIndex;
    
    // Precompute text rendering parameters
    float smoothFactor = float(inTexIndex) / 65535.0;
    fragStartWidth = smoothFactor;            // Smoothing factor
    fragEndWidth = float(inTexIndex & 0xFFu); // Outline width scale
    
    // Store quad and UV information
    fragQuadPos = inTexCoord;    // Original quad position
    fragStartPoint = inTexCoords.xy;  // UV origin
    fragEndPoint = inTexCoords.zw;    // UV scale
    
    // Store shadow parameters
    fragEndColor = vec4(1.0, 1.0, 0.0, 0.0);  // Shadow parameters or offsets
    
    // Standard vertex position
    gl_Position = sceneData.projection * inInstanceMatrix * vec4(inPosition, 0.0, 1.0);
}

// Process sprite vertex
void processSpriteVertex(bool useSolidColor) {
    // Calculate texture coordinates by interpolating between min and max UV
    vec2 baseTexCoord = inTexCoord;
    
    // Calculate the texture coordinates range
    vec2 uvMin = inTexCoords.xy;
    vec2 uvMax = inTexCoords.zw;
    vec2 uvRange = uvMax - uvMin;
    
    // Apply flipping while maintaining sampler repeat behavior
    // If uvMin > uvMax for a component, it indicates flipping
    vec2 flippedTexCoord = baseTexCoord;
    if (uvMin.x > uvMax.x) {
        flippedTexCoord.x = 1.0 - flippedTexCoord.x;
    }
    if (uvMin.y > uvMax.y) {
        flippedTexCoord.y = 1.0 - flippedTexCoord.y;
    }
    
    // Calculate final texture coordinates
    fragTexCoord = uvMin + flippedTexCoord * abs(uvRange);
    fragColor = inInstanceColor;
    fragTexIndex = inTexIndex;
    
    // Store sprite-specific parameters
    fragStartWidth = useSolidColor ? 1.0 : 0.0;  // Flag for solid color mode
    fragEndWidth = fragColor.a;                  // Store alpha for optimization
    
    // Store transformation data
    fragQuadPos = inTexCoord;        // Original quad position
    fragStartPoint = inTexCoords.xy; // UV min
    fragEndPoint = inTexCoords.zw;   // UV max
    
    // Store unmodified color for effects
    fragEndColor = vec4(fragColor.rgb, 1.0);
    
    // Standard vertex position
    gl_Position = sceneData.projection * inInstanceMatrix * vec4(inPosition, 0.0, 1.0);
}

void main() {
    // Always pass the flags to fragment shader
    fragFlags = inFlags;
    
    // Determine element type
    bool isTrail = (inFlags & RENDER_FLAG_TRAIL) != 0;
    bool isText = (inFlags & RENDER_FLAG_TEXT) != 0;
    bool useSolidColor = (inFlags & RENDER_FLAG_SOLID) != 0;
    
    // Process vertex based on element type
    // Use separate function calls instead of if-else to improve readability
    if (isTrail) {
        processTrailVertex();
    } 
    else if (isText) {
        processTextVertex();
    }
    else {
        processSpriteVertex(useSolidColor);
    }
}