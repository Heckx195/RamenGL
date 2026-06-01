#version 460


layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_WorldSpacePos;
layout(location = 2) in vec3 in_Color;

layout(location = 3) uniform vec3 u_LightPos;

out vec4 outColor;

void main()
{
    vec3 N = normalize(in_Normal);
    vec3 L = normalize(u_LightPos - in_WorldSpacePos);
    float diffuse = max(0.0f, dot(N, L));
    float ambient = 0.1f;
    outColor = vec4(in_Color * (diffuse + ambient), 1.0f);
}
