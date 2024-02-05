#version 330 core

out vec3 color;

in vec3 lineColor;

void main() {
    color = lineColor;
}
