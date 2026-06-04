#version 460

layout(location = 0) in vec3 in_UV;
layout(location = 1) in vec4 in_LightSpacePos;

out vec4 outColor;

layout(binding = 0) uniform sampler2D u_Texture;
// TODO: Aufgabe 6.4)
layout(binding = 1) uniform sampler2DShadow u_ShadowMap;
layout(location = 5) uniform bool u_UseShadow;
layout(location = 6) uniform bool u_UseTexture;
layout(location = 7) uniform float u_BiasAmount;
layout(location = 8) uniform bool u_UsePCF;

void main()
{
    vec4 texColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (u_UseTexture) {
        texColor = texture(u_Texture, in_UV.st);
            // Sampelt über UV (in_UV) die korrekten Farbwerte an dem Punkt in der Textur
            // u_Texture ist der texture unit, die in der Main auf "0" gebindet wurde

        // vec4 reflectColor = reflect(I, N) //
            // I=Incident: Vektor von der Kamera zur Oberfläche (der eintreffende Sichtstrahl)
            // N=Normal: Oberflächennormale des Modells (zeigt vom Objekt weg)
    }

    float shadow = 0.0;
    if (u_UseShadow) {
        vec3 projCoords = in_LightSpacePos.xyz / in_LightSpacePos.w;  // NDC
        projCoords = projCoords * 0.5 + 0.5; // auf [0,1] mappen


        if (projCoords.z <= 1.0) { // Ist Fragement im Frustum der LightCamera dann ist .z <=1.0
            
            if (u_UsePCF) {
                float sum = 0.0;
                projCoords.z -= u_BiasAmount; // Bias hinzufügen
                vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
                // x-offset = -1.5, -0.5, +0.5, +1.5
                for (float x = -1.5; x <= 1.5; ++x) {
                    // y-offset = -1.5, -0.5, +0.5, +1.5
                    for (float y = -1.5; y <= 1.5; ++y) {
                        vec2 offset = vec2(x, y) * texelSize; // UV-Offset an Texelgröße normalisieren.
                        // PCF Sampling
                        sum += texture(u_ShadowMap, projCoords.xyz + vec3(offset, 0.0));
                            // GPU vergleicht automatisch Tiefenwert von .xy mit .z und
                            // returnt 1.0 wenn beleuchtet, sonst 0.0 Schatten
                    }
                }

                shadow = 1 - (sum / 16); // Durchschnitt der Schattentests
            } else {
                // Einmaliges ShadowMap sampeln
                projCoords.z -= u_BiasAmount; // Bias hinzufügen
                shadow = 1 - texture(u_ShadowMap, projCoords.xyz);
            }
        }
    }

    outColor = texColor * (1 - shadow);
    // (0.3 + 0.7 * (1.0 - shadow))
        // Im Schatten 30% Helligkeit
        // Im Licht 100% Helligkeit
}
