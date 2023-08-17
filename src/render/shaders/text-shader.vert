#version 330 core

layout(location = 0) in vec2 vertexPosition_screenSpace;
layout(location = 1) in vec2 vertexUV;

out vec2 UV;

void main() {
    // map [0..1024][0..768] to [-1..1][-1..1]
    float halfWidth = 1024 / 2;
    float halfHeight = 768 / 2;
    vec2 vertexPosition_homoneneousSpace = vertexPosition_screenSpace - vec2(halfWidth, halfHeight);
    vertexPosition_homoneneousSpace /= vec2(halfWidth, halfHeight);
    gl_Position = vec4(vertexPosition_homoneneousSpace, 0, 1);

    UV = vertexUV;
}

