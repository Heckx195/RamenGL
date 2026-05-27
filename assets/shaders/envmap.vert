#version 460

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

layout(location = 0) out vec3 out_WorldPos;
layout(location = 1) out vec3 out_WorldNormal;

/* NOTE:
   One could use 'glGetUniformLocation' on CPU-side instead
   of fixed location = ... qualifiers.
   But this is not recommended anymore.
   @See: OpenGL Superbible 7, page 156.
*/
layout(location = 0) uniform mat4 u_ModelMat;
layout(location = 1) uniform mat4 u_ViewMat;
layout(location = 2) uniform mat4 u_ProjMat;

void main()
{
   vec4 position = u_ProjMat * u_ViewMat * u_ModelMat * vec4(in_Position, 1.0f);
   gl_Position = position;
   
   vec4 worldPos = u_ModelMat * vec4(in_Position, 1.0f);
   out_WorldPos = worldPos.xyz;
   out_WorldNormal = mat3(u_ModelMat) * in_Normal; // mat3 hier möglich, weil Normals keine Translation benötigen, die in der vierten Spalte liegen.
}
