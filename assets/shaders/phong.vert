#version 460

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

layout(location = 0) out vec3 out_WorldPos;
layout(location = 1) out vec3 out_WorldNormal;
layout(location = 2) out vec3 out_color;

layout(location = 0) uniform mat4 u_ModelMat;
layout(location = 1) uniform mat4 u_ViewMat;
layout(location = 2) uniform mat4 u_ProjMat;

layout(location = 3) uniform bool u_usePhongShading;
layout(location = 4) uniform vec3 u_lightPos;
layout(location = 5) uniform vec3 u_cameraPos;

layout(location = 6) uniform vec3 u_materialEmissive; // Selbststrahlend
layout(location = 7) uniform vec3 u_materialLightAmbient; // Hintergrundlicht, Objekte sowie Lichtquelle
layout(location = 8) uniform vec3 u_materialLightDiffuse; // Diffuses Licht, Objekte sowie Lichtquelle
layout(location = 9) uniform vec3 u_materialLightSpecular; // Spekulare Licht, physikalisch gleich zwischen Material und Lichtquelle, aber in Spielen nicht (rotes Licht, aber weißes Hightlight)
layout(location = 10) uniform float u_shininess;

void main()
{
    vec4 position = u_ProjMat * u_ViewMat * u_ModelMat * vec4(in_Position, 1.0f);
    gl_Position = position;
   
    vec4 worldPos = u_ModelMat * vec4(in_Position, 1.0f);
    out_WorldPos = worldPos.xyz;
    out_WorldNormal = mat3(u_ModelMat) * in_Normal;

    if (u_usePhongShading)
    {
        // Phong Shading: Normale interpolieren lassen
    }
    else
    {
        // Gouraud Shading: Normale pro Vertex berechnen
        // emissive
        vec3 emissive = u_materialEmissive;

        // ambient
        vec3 ambient = u_materialLightAmbient;
        
        // diffus
        vec3 N = normalize(out_WorldNormal);
        vec3 L = normalize(u_lightPos - out_WorldPos); // Ziel minus Ursprung
        float diffus = max(dot(N, L), 0.0);

        // spekular
        vec3 V = normalize(u_cameraPos - out_WorldPos); // Ziel minus Ursprung
        vec3 H = normalize(L + V);
        float spekular = max(dot(H, N), 0.0);
        spekular = pow(spekular, u_shininess);

        out_color = emissive + ambient + diffus * u_materialLightDiffuse + spekular * u_materialLightSpecular;
            // u_materialLightDiffuse = sorgt dafür das ein rotes Material auch nur rotes Licht reflektiert
    }

}
