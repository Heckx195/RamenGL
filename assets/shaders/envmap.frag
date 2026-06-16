#version 460

layout(location = 0) in vec3 in_WorldPos;
layout(location = 1) in vec3 in_WorldNormal;

out vec4 outColor;

layout(binding = 0) uniform samplerCube u_Cubemap;  // binding für sampler
layout(location = 3) uniform vec3 u_CameraPos;        // location für normale Werte

void main()
{
    vec3 I = normalize(in_WorldPos - u_CameraPos); // Einfallender Strahl
    vec3 N = normalize(in_WorldNormal); // Oberflaechennormale
    vec3 reflectDirection = reflect(I, N); // Normalize weil reflect benötigt Einheitsvektoren
        // I=Incident: Vektor von der Kamera zur Oberflaeche (der eintreffende Sichtstrahl)
        // N=Normal: Oberflaechennormale des Modells (zeigt vom Objekt weg)
        // == Reflexionsvektor

    outColor = texture(u_Cubemap, reflectDirection);
        // Sampelt über Richtungsvektor/Reflexionsvektor (reflectDirection) die korrekten Farbwerte an dem Punkt
        // u_Cubemap ist der texture unit, die in der Main auf "0" gebindet wurde
        // 1. Findet richtige Flaeche von der Cubemap, durch groesste Komponente im Richtungsvektor
        // 2. Findet Farbwert auf der Flaeche
}