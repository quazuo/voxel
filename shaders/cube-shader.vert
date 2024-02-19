#version 330 core

layout(location = 0) in ivec3 vertexPosition;
layout(location = 1) in uint packedNormalUvTex;

out vec4 vertexPosition_lightSpace;
out vec3 Normal_modelspace;
out vec2 UV;
flat out uint texID;

uniform mat4 MVP;
uniform mat4 lightMVP;
uniform vec3 LightDirection_worldspace;

vec3 unpackVertexNormal(uint packedVertex) {
    uint normalIndex = packedVertex >> 29;

    if (normalIndex == 0u) return vec3(0, 0, 1);
    if (normalIndex == 1u) return vec3(0, 0, -1);
    if (normalIndex == 2u) return vec3(1, 0, 0);
    if (normalIndex == 3u) return vec3(-1, 0, 0);
    if (normalIndex == 4u) return vec3(0, 1, 0);
    if (normalIndex == 5u) return vec3(0, -1, 0);
    return vec3(0);
}

vec2 unpackVertexUV(uint packedVertex) {
    return vec2(
        float((packedVertex >> 24) & 0x1Fu),
        float((packedVertex >> 19) & 0x1Fu)
    );
}

uint unpackVertexTexID(uint packedVertex) {
    return packedVertex & 0xFFu;
}

void main() {
    vec3 vertexPosition_modelspace = vertexPosition;
    Normal_modelspace = unpackVertexNormal(packedNormalUvTex);
    UV = unpackVertexUV(packedNormalUvTex);
    texID = unpackVertexTexID(packedNormalUvTex);

    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);

    vertexPosition_lightSpace = lightMVP * vec4(vertexPosition_modelspace, 1.0);
}
