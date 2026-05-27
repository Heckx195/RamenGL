#version 460

layout(location = 1) in vec3 in_Color;
layout(location = 3) in vec3 in_TexelPos;

out vec4 outColor;

// TODO: Aufgabe 5.1.2)
layout(binding = 0) uniform samplerCube u_Texture;


void main()
{
    vec4 texColor = texture(u_Texture, in_TexelPos);
        // Sampelt über Richtungsvektor (in_TexelPos) die korrekten Farbwerte an dem Punkt
        // u_Texture ist der texture unit, die in der Main auf "0" gebindet wurde
    
    outColor = texColor;
}
