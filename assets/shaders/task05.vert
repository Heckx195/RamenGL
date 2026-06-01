#version 460

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

layout(location = 1) out vec3 out_Direction;

layout(location = 0) uniform mat4 u_ModelMat;
layout(location = 1) uniform mat4 u_ViewMat;
layout(location = 2) uniform mat4 u_ProjMat;

void main()
{
   gl_Position = u_ProjMat * u_ViewMat * u_ModelMat * vec4(in_Position, 1.0f);
   out_Direction = in_Position; // Richtungsvektor vom Texel.
}
