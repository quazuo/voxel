#version 330 core

out vec4 fragColor;

in vec3 texCoords;

uniform samplerCube skyboxTexSampler;

void main()
{
    fragColor = vec4(texCoords, 1); //texture(skyboxTexSampler, texCoords);
}