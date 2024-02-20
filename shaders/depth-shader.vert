#version 330 core
layout(location = 0) in ivec3 vertexPosition;

uniform mat4 MVP;

void main() {
    vec3 vertexPosition_modelspace = vertexPosition;
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);
}
