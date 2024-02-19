#version 330 core

in vec2 UV;
flat in uint texID;
in vec3 Normal_modelspace;
in vec4 vertexPosition_lightSpace;

out vec4 color;

uniform vec3 LightDirection_worldspace;
uniform sampler2D texSampler[4];
uniform bool doDrawShadows;
uniform sampler2D shadowMap;

#define DEF_TEX_SAMPLER_ID(n) \
    if (texID == (n)) return texture(texSampler[(n)], UV).rgb;

vec3 getTexSample(uint id) {
    // there's 4 textures currently
    DEF_TEX_SAMPLER_ID(0u)
    DEF_TEX_SAMPLER_ID(1u)
    DEF_TEX_SAMPLER_ID(2u)
    DEF_TEX_SAMPLER_ID(3u)
    return vec3(0);
}

float calcShadow(vec4 fragPos_lightSpace) {
    vec3 projCoords = fragPos_lightSpace.xyz / fragPos_lightSpace.w; // perspective division
    projCoords = projCoords * 0.5 + 0.5; // [-1, 1] -> [0, 1]
    float currentDepth = projCoords.z;

    if (currentDepth > 1.0) {
        return 0.0;
    }

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float bound = 1.5;
    float bias = 0.0005;

    for (float x = -bound; x <= bound; ++x) {
        for (float y = -bound; y <= bound; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }

    float samplesCount = (bound * 2 + 1) * (bound * 2 + 1);
    return shadow / samplesCount;
}

void main() {
    vec3 LightColor = vec3(1.0, 1.0, 0.9);
    float LightPower = 0.6f;

    vec3 texColor = getTexSample(texID);

    vec3 ambient = vec3(0.1) * texColor;

    vec3 n = normalize(Normal_modelspace);
    vec3 l = normalize(LightDirection_worldspace);
    float cosTheta = clamp(dot(n, l), 0, 1);
    vec3 diffuse = texColor * LightColor * LightPower * cosTheta;

    float shadow = doDrawShadows ? calcShadow(vertexPosition_lightSpace) : 0.0;

    vec3 opaqueColor = ambient + (1.0 - shadow) * diffuse;

    float gamma = 2.2;
    vec3 gammaCorrected = pow(opaqueColor, vec3(1.0 / gamma));

    color = vec4(gammaCorrected, 1.0);
}
