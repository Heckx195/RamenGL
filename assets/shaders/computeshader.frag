#version 460

layout(location = 0) in vec3 in_UV;
layout(location = 4) in vec3 in_VertexColor;

layout(location = 0) out vec4 out_Color;

layout(binding = 0) uniform samplerCube u_CubemapTexture;
layout(binding = 1) uniform sampler2DShadow u_ShadowMap;
layout(binding = 5) uniform sampler2D u_PatchDebugTexture;
layout(binding = 6) uniform sampler2D u_LightmapTexture;

layout(location = 0) uniform mat4 u_Model;
layout(location = 4) uniform mat4 u_View;
layout(location = 8) uniform mat4 u_Proj;

void main()
{
    vec4 patchColor = texture(u_LightmapTexture, in_UV.xy);
    out_Color = vec4(patchColor.rgb * in_VertexColor, 1.0f);
}
