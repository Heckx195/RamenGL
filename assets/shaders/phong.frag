#version 460

layout(location = 0) in vec3 in_WorldPos;
layout(location = 1) in vec3 in_WorldNormal;
layout(location = 2) in vec3 in_Color;
layout(location = 3) in vec2 in_UV;
layout(location = 4) in vec3 in_WorldTangent;
layout(location = 5) in vec3 in_WorldBitangent;

out vec4 outColor;

layout(binding = 0) uniform samplerCube u_Cubemap;  // binding für sampler
layout(binding = 1) uniform sampler2D u_NormalMap;

layout(location = 3) uniform bool u_usePhongShading;
layout(location = 4) uniform vec3 u_lightPos;
layout(location = 5) uniform vec3 u_cameraPos;

layout(location = 6) uniform vec3 u_materialEmissive; // Selbststrahlend
layout(location = 7) uniform vec3 u_materialLightAmbient; // Hintergrundlicht, Objekte sowie Lichtquelle
layout(location = 8) uniform vec3 u_materialLightDiffuse; // Diffuses Licht, Objekte sowie Lichtquelle
layout(location = 9) uniform vec3 u_materialLightSpecular; // Spekulare Licht, physikalisch gleich zwischen Material und Lichtquelle, aber in Spielen nicht (rotes Licht, aber weißes Hightlight)
layout(location = 10) uniform float u_shininess;
layout(location = 11) uniform float u_reflectivity;
layout(location = 12) uniform bool u_useBumpMapping;

void main()
{
    vec3 N = normalize(in_WorldNormal); // Oberflaechennormale
    if (u_useBumpMapping)
    {
        // Bump Mapping logik hier
        vec3 N_TangentSpace = normalize(texture(u_NormalMap, in_UV).rgb * 2 - 1.0); // Normal aus der Normalmap holen und in [-1, 1] Bereich umwandeln
            // Sample Normalmap basierend auf UV Koordinaten
            // Umwandlung der XYZ-Koordinaten der gesampelten NormalenTangentSpace in [-1, 1] transformieren

        // TBN-Matrix (Tangent, Bitangent, Normal) erstellen
        mat3 TBN = mat3(normalize(in_WorldTangent), normalize(in_WorldBitangent), normalize(in_WorldNormal));
        N = normalize(TBN * N_TangentSpace); // NormaleTangentSpace in Weltkoordinaten transformieren
            // Über TBN-Matrix
    }
    
    vec3 I = normalize(in_WorldPos - u_cameraPos); // Einfallender Strahl, Ziel minus Ursprung
    vec3 reflectDirection = reflect(I, N); // Normalize weil reflect benötigt Einheitsvektoren
        // I=Incident: Vektor von der Kamera zur Oberflaeche (der eintreffende Sichtstrahl)
        // N=Normal: Oberflaechennormale des Modells (zeigt vom Objekt weg)
        // == Reflexionsvektor

    vec3 reflectColor = texture(u_Cubemap, reflectDirection).rgb;
        // Sampelt über Richtungsvektor/Reflexionsvektor (reflectDirection) die korrekten Farbwerte an dem Punkt
        // u_Cubemap ist der texture unit, die in der Main auf "0" gebindet wurde
        // 1. Findet richtige Flaeche von der Cubemap, durch groesste Komponente im Richtungsvektor
        // 2. Findet Farbwert auf der Flaeche

    if (u_usePhongShading)
    {
        // emissive
        vec3 emissive = u_materialEmissive;

        // ambient
        vec3 ambient = u_materialLightAmbient;
        
        // diffus
        vec3 L = normalize(u_lightPos - in_WorldPos); // Ziel minus Ursprung
        float diffus = max(dot(N, L), 0.0);

        // spekular
        vec3 V = normalize(u_cameraPos - in_WorldPos); // Ziel minus Ursprung
        vec3 H = normalize(L + V);
        float spekular = max(dot(H, N), 0.0);
        spekular = pow(spekular, u_shininess);

        vec3 phong_color = emissive + ambient + diffus * u_materialLightDiffuse + spekular * u_materialLightSpecular;
            // u_materialDiffuse = sorgt dafür, dass ein rotes Material auch nur rotes Licht reflektiert
        outColor = vec4(mix(phong_color, reflectColor, u_reflectivity), 1.0f);
    }
    else
    {
        vec3 gouraud_color = in_Color;
        outColor = vec4(mix(gouraud_color, reflectColor, u_reflectivity), 1.0f);
    }
}