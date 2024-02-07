#version 330 core

in vec2 UV;
flat in int texID;
in vec3 Position_worldspace;
in vec3 Normal_modelspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;

out vec4 color;

uniform vec3 LightDirection_worldspace;
uniform sampler2D texSampler[24];
uniform mat4 MV;

#define DEF_TEX_SAMPLER_ID(n) \
    if (texID == 6 * (n)) return texture(texSampler[6 * (n)], UV).rgb; \
    if (texID == 6 * (n) + 1) return texture(texSampler[6 * (n) + 1], UV).rgb; \
    if (texID == 6 * (n) + 2) return texture(texSampler[6 * (n) + 2], UV).rgb; \
    if (texID == 6 * (n) + 3) return texture(texSampler[6 * (n) + 3], UV).rgb; \
    if (texID == 6 * (n) + 4) return texture(texSampler[6 * (n) + 4], UV).rgb; \
    if (texID == 6 * (n) + 5) return texture(texSampler[6 * (n) + 5], UV).rgb;

vec3 getTexSample(int id) {
    DEF_TEX_SAMPLER_ID(0)
    DEF_TEX_SAMPLER_ID(1)  // grass
    DEF_TEX_SAMPLER_ID(2)  // dirt
    DEF_TEX_SAMPLER_ID(3)  // stone
    return vec3(0);
}

void main() {
    vec3 LightColor = vec3(1.0, 1.0, 0.9);
    float LightPower = 0.7f;

    vec3 MaterialDiffuseColor = getTexSample(texID);
    vec3 MaterialAmbientColor = vec3(0.3) * MaterialDiffuseColor;
    vec3 MaterialSpecularColor = vec3(0);

    vec3 n = normalize(Normal_modelspace);
    vec3 l = normalize(LightDirection_worldspace);
    float cosTheta = clamp(dot(n, l), 0, 1);

    vec3 E = normalize(EyeDirection_cameraspace);
    vec3 R = reflect(-l, n);
    float cosAlpha = clamp(dot(E, R), 0, 1);

    vec3 opaqueColor =
        MaterialAmbientColor +
        MaterialDiffuseColor * LightColor * LightPower * cosTheta +
        MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, 5);

    color = vec4(opaqueColor, 1.0);
}
