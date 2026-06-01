#version 460

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Color;

layout(location = 0) out vec3 out_Normal;
layout(location = 1) out vec3 out_WorldSpacePos;
layout(location = 2) out vec3 out_Color;

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

    mat3 normalMatrix = transpose(inverse(mat3(u_ModelMat)));
    out_Normal = normalize(normalMatrix * in_Normal);
    out_WorldSpacePos = vec3(u_ModelMat * vec4(in_Position, 1.0f));
    out_Color = in_Color;
}
