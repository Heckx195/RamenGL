#version 460

layout(location = 0) in vec3 in_WorldPos;
layout(location = 1) in vec3 in_WorldNormal;

out vec4 outColor;

layout(binding = 0) uniform samplerCube u_Cubemap;  // binding für sampler
layout(location = 3) uniform vec3 cameraPos;        // location für normale Werte

void main()
{
    vec3 I = normalize(in_WorldPos - cameraPos);
    vec3 N = normalize(in_WorldNormal);
    vec3 reflectDirection = reflect(I, N); // Normalize weil reflect benötigt Einheitsvektoren
        // I=Incident: Vektor von der Kamera zur Oberfläche (der eintreffende Sichtstrahl)
        // N=Normal: Oberflächennormale des Modells (zeigt vom Objekt weg)

    outColor = texture(u_Cubemap, reflectDirection);
        // Sampelt über Richtungsvektor/Reflexionsvektor (reflectDirection) die korrekten Farbwerte an dem Punkt
        // u_Cubemap ist der texture unit, die in der Main auf "0" gebindet wurde
}