#version 330 core

out vec4 color;

in vec3 texCoords;

uniform vec3 LightDirection_worldspace;
uniform samplerCube skyboxTexSampler;

void main()
{
    vec4 sunColor = vec4(1.0, 1.0, 0.9, 1.0);
    vec3 diff = normalize(texCoords) - normalize(LightDirection_worldspace);
    float eps = 0.03;

    if (abs(diff.x) < eps && abs(diff.y) < eps && abs(diff.z) < eps) {
        color = sunColor;
    } else {
        color = texture(skyboxTexSampler, texCoords);
    }
}
