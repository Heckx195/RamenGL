#version 460

layout(location = 1) in vec3 in_Direction;

out vec4 outColor;

// TODO: Aufgabe 5.1.2)
layout(binding = 0) uniform samplerCube u_Cubemap;

void main()
{
    outColor = texture(u_Cubemap, in_Direction);
}
