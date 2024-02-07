#version 330 core

layout(location = 0) in uint packedVertex;
layout(location = 1) in int textureID;

out vec2 UV;
flat out int texID;
out vec3 Position_worldspace;
out vec3 Normal_modelspace;
out vec3 Normal_cameraspace;
out vec3 EyeDirection_cameraspace;

uniform vec3 LightDirection_worldspace;
uniform mat4 MVP;
uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

float rand(vec3 co) {
    return fract(sin(dot(co.xyz, vec3(12.9898, 78.233, 54.321))) * 43758.5453);
}

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

    // Output position of the vertex, in clip space: MVP * position
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);

    // Position of the vertex, in worldspace: M * position
    Position_worldspace = (M * vec4(vertexPosition_modelspace, 1)).xyz;

    // Vector that goes from the vertex to the camera, in camera space.
    // In camera space, the camera is at the origin (0, 0, 0).
    vec3 vertexPosition_cameraspace = (V * M * vec4(vertexPosition_modelspace, 1)).xyz;
    EyeDirection_cameraspace = vec3(0, 0, 0) - vertexPosition_cameraspace;

    // Normal of the the vertex, in camera space
    // Only correct if ModelMatrix does not scale the model! Use its inverse transpose if not.
    Normal_modelspace = vertexNormal_modelspace;
    Normal_cameraspace = (V * M * vec4(vertexNormal_modelspace, 0)).xyz;

    // UV of the vertex. No special space for this one.
    UV = vertexUV;

    // ID of the texture used by this vertex. Always equal for all vertices on the same face.
    texID = textureID;
}
