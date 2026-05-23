#version 460

out vec4 outColor;

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_WorldSpacePos;
layout(location = 2) in vec3 in_Color;

layout(location = 3) uniform vec3 u_LightPos;

void main()
{
    vec3 L = normalize(u_LightPos - in_WorldSpacePos); // L vector goes from worldSpacePos to lightPos
    float diffuse = max(0.0f, dot(in_Normal, L));
    float ambient = 0.1f;
    outColor = vec4(in_Color * (diffuse + ambient), 1.0f);
}
