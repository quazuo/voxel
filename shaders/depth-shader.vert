#version 330 core
layout(location = 0) in uint packedVertex;

uniform mat4 MVP;

vec3 unpackVertexPosition(uint packedVertex) {
    return vec3(
        float((packedVertex & 0xF8000000u) >> 27),
        float((packedVertex & 0x07C00000u) >> 22),
        float((packedVertex & 0x003E0000u) >> 17)
    );
}

void main() {
    vec3 vertexPosition_modelspace = unpackVertexPosition(packedVertex);
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);
}
