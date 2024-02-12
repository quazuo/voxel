#version 330 core

layout(location = 0) in uint packedVertex;
layout(location = 1) in int textureID;

out vec2 UV;
flat out int texID;
out vec3 Normal_modelspace;
out vec4 vertexPosition_lightSpace;

uniform mat4 MVP;
uniform mat4 lightMVP;
uniform vec3 LightDirection_worldspace;

vec3 unpackVertexPosition(uint packedVertex) {
    return vec3(
        float((packedVertex & 0xF8000000u) >> 27),
        float((packedVertex & 0x07C00000u) >> 22),
        float((packedVertex & 0x003E0000u) >> 17)
    );
}

vec3 unpackVertexNormal(uint packedVertex) {
    uint normalIndex = (packedVertex & 0x0001C000u) >> 14;

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
        float((packedVertex & 0x00003E00u) >> 9),
        float((packedVertex & 0x000001F0u) >> 4)
    );
}

void main() {
    vec3 vertexPosition_modelspace = unpackVertexPosition(packedVertex);
    vec3 vertexNormal_modelspace = unpackVertexNormal(packedVertex);
    vec2 vertexUV = unpackVertexUV(packedVertex);

    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);

    vertexPosition_lightSpace = lightMVP * vec4(vertexPosition_modelspace, 1.0);

    Normal_modelspace = vertexNormal_modelspace;

    UV = vertexUV;

    texID = textureID;
}
