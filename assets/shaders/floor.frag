#version 460

layout(location = 0) in vec3 in_UV;
layout(location = 1) in vec4 in_LightSpacePos;

out vec4 outColor;

layout(binding = 0) uniform sampler2D u_Texture;
// TODO: Aufgabe 6.4)
layout(binding = 1) uniform sampler2D u_ShadowMap;
layout(location = 5) uniform bool u_UseShadow;
layout(location = 6) uniform bool u_UseTexture;


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
        projCoords = projCoords * 0.5 + 0.5;

        if (projCoords.z <= 1.0) {
            // ShadowMap sampeln
            vec4 sampleShadowMap = texture(u_ShadowMap, projCoords.xy); // return vec4 {r,g,b,a}
                // Sample mit xy-Wert des transformierten Fragment-Coords den Tiefenwert
                //  des jeweiligen Fragment aus der ShadowMap
            float closetDepth = sampleShadowMap.r;
                // Eine Depth-Texture speichert ihren Tiefenwert im ersten Kanal, also .r (Rot-Kanal)
                //  deshalb mit .r diesen extrahieren

            float currentDepth = projCoords.z; // Depth-Wert aus normalen Rendern.
            shadow = currentDepth > closetDepth ? 1.0 : 0.0;
        }
    }

    outColor = texColor * (1 - shadow);
    // (0.3 + 0.7 * (1.0 - shadow))
        // Im Schatten 30% Helligkeit
        // Im Licht 100% Helligkeit
}
