#version 460


layout(location = 0) in vec3 in_Color;

out vec4 outColor;

void main()
{
    outColor = vec4(in_Color, 1.0f);
}
