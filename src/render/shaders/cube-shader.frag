#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
flat in int texID;
in vec3 Position_worldspace;
in vec3 Normal_modelspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh.
uniform sampler2D texSampler[24];
uniform mat4 MV;
uniform vec3 LightPosition_worldspace;

#define DEF_TEX_SAMPLER_ID(n) \
    if (texID == 6 * (n)) return texture(texSampler[6 * (n)], UV).rgb; \
    if (texID == 6 * (n) + 1) return texture(texSampler[6 * (n) + 1], UV).rgb; \
    if (texID == 6 * (n) + 2) return texture(texSampler[6 * (n) + 2], UV).rgb; \
    if (texID == 6 * (n) + 3) return texture(texSampler[6 * (n) + 3], UV).rgb; \
    if (texID == 6 * (n) + 4) return texture(texSampler[6 * (n) + 4], UV).rgb; \
    if (texID == 6 * (n) + 5) return texture(texSampler[6 * (n) + 5], UV).rgb; \

vec3 getTexSample(int id) {
    DEF_TEX_SAMPLER_ID(0)
    DEF_TEX_SAMPLER_ID(1)  // grass
    DEF_TEX_SAMPLER_ID(2)  // dirt
    DEF_TEX_SAMPLER_ID(3)  // stone
}

void main() {
    // Light emission properties
    vec3 LightColor = vec3(1);
    float LightPower = 0.7f;

    // Material properties
    vec3 MaterialDiffuseColor = getTexSample(texID);
    vec3 MaterialAmbientColor = vec3(0.3) * MaterialDiffuseColor;
    vec3 MaterialSpecularColor = vec3(0);

    // Normal of the computed fragment, in camera space
    vec3 n = normalize(Normal_modelspace);
    // Direction of the light (from the fragment to the light)
    vec3 l = normalize(vec3(0.2, 0.3, 0.4));
    // Cosine of the angle between the normal and the light direction,
    // clamped above 0
    //  - light is at the vertical of the triangle -> 1
    //  - light is perpendicular to the triangle -> 0
    //  - light is behind the triangle -> 0
    float cosTheta = clamp(dot(n, l), 0, 1);

    // Eye vector (towards the camera)
    vec3 E = normalize(EyeDirection_cameraspace);
    // Direction in which the triangle reflects the light
    vec3 R = reflect(-l, n);
    // Cosine of the angle between the Eye vector and the Reflect vector,
    // clamped to 0
    //  - Looking into the reflection => equal to 1
    //  - Looking elsewhere => less than 1
    float cosAlpha = clamp(dot(E, R), 0, 1);

    vec3 opaqueColor =
    // Ambient : simulates indirect lighting
    MaterialAmbientColor +
    // Diffuse : "color" of the object
    MaterialDiffuseColor * LightColor * LightPower * cosTheta +
    // Specular : reflective highlight, like a mirror
    MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, 5);

    color = vec4(opaqueColor, 1.0);
}