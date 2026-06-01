#version 460

layout(location = 0) in vec3 in_Position;

/* NOTE:
   One could use 'glGetUniformLocation' on CPU-side instead
   of fixed location = ... qualifiers.
   But this is not recommended anymore.
   @See: OpenGL Superbible 7, page 156.
*/
layout(location = 0) uniform mat4 u_ModelMat;
layout(location = 1) uniform mat4 u_LightViewMat;
layout(location = 2) uniform mat4 u_LightProjMat;

void main()
{
    vec4 positionClipSpace = u_LightProjMat * u_LightViewMat * u_ModelMat * vec4(in_Position, 1.0f);
    gl_Position = positionClipSpace;

    // OpenGL macht automatisch gl_Position.z / gl_Position.w und berechnet damit
    //  automatisch den Tiefenwert und schreibt ihn in das Depth-Attachment des FBOs
}
