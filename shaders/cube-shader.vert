#version 330 core

layout(location = 0) in ivec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in ivec2 vertexUV;
layout(location = 3) in int textureID;

out vec4 vertexPosition_lightSpace;
out vec3 Normal_modelspace;
out vec2 UV;
flat out int texID;

uniform mat4 MVP;
uniform mat4 lightMVP;
uniform vec3 LightDirection_worldspace;

void main() {
    vec3 vertexPosition_modelspace = vertexPosition;
    Normal_modelspace = vertexNormal;
    UV = vertexUV;

    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);

    vertexPosition_lightSpace = lightMVP * vec4(vertexPosition_modelspace, 1.0);

    texID = textureID;
}
