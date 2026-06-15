#version 460

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Color;
layout(location = 3) in vec3 in_UV;
layout(location = 4) in vec3 in_Tangent;
layout(location = 5) in vec3 in_Bitangent;

layout(location = 0) out vec3 out_UV;
layout(location = 4) out vec3 out_VertexColor;

layout(location = 0) uniform mat4 u_Model;
layout(location = 4) uniform mat4 u_View;
layout(location = 8) uniform mat4 u_Proj;

void main()
{
    vec4 worldPos = u_Model * vec4(in_Position, 1.0f);
    gl_Position = u_Proj * u_View * worldPos;
    out_UV = in_UV;
    out_VertexColor = in_Color;
}
