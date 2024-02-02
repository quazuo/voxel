#version 330 core

out vec4 color;

in vec3 texCoords;

uniform samplerCube skyboxTexSampler;

void main()
{
    color = texture(skyboxTexSampler, texCoords);
}