#include <glad/glad.h>

#define _USE_MATH_DEFINES

#include <assert.h>
#include <cmath>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl3.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <physfs.h>

#include "../ramen/ramen.h"
#include "../ramen/rgl_camera.h"
#include "../ramen/rgl_defines.h"
#include "../ramen/rgl_filesystem.h"
#include "../ramen/rgl_image.h"
#include "../ramen/rgl_math.h"
#include "../ramen/rgl_model.h"
#include "../ramen/rgl_shader.h"
#include "../ramen/rgl_utils.h"

#include "./matrixStack.h"

std::vector<Vertex> CreateCube(const Vec3f& color)
{
    std::vector<Vertex> vertices;
    
    // Define the 8 corners of the cube
        // symmetrisch um den Ursprung.
    Vec3f corners[8] = {
        Vec3f{-1.f, -1.f, -1.f},
        Vec3f{ 1.f, -1.f, -1.f},
        Vec3f{ 1.f,  1.f, -1.f},
        Vec3f{-1.f,  1.f, -1.f},
        Vec3f{-1.f, -1.f,  1.f},
        Vec3f{ 1.f, -1.f,  1.f},
        Vec3f{ 1.f,  1.f,  1.f},
        Vec3f{-1.f,  1.f,  1.f}
    };
    
    // Helper to add quad: 2 Dreiecke (6 Vertices)
    auto pushQuad = [&](Vec3f a, Vec3f b, Vec3f c, Vec3f d,
                        Vec3f n,
                        Vec3f uva, Vec3f uvb, Vec3f uvc, Vec3f uvd) {
        // Dreieck 1: a, c, b -> von außen ccw
        vertices.push_back({a, n, color, uva});
        vertices.push_back({c, n, color, uvb});
        vertices.push_back({b, n, color, uvc});
        // Dreieck 2: a, d, c -> von außen ccw
        vertices.push_back({a, n, color, uva});
        vertices.push_back({d, n, color, uvc});
        vertices.push_back({c, n, color, uvd});
    };

    // CCW von außen der Würfeloberfläche sichtbare Dreiecksnetz definieren
    // Front face (z = -1)
    pushQuad(corners[0], corners[1], corners[2], corners[3],
            Vec3f{0,0,-1},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Back face (z = +1)
    pushQuad(corners[4], corners[7], corners[6], corners[5],
            Vec3f{0,0,1},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Left face (x = -1)
    pushQuad(corners[0], corners[3], corners[7], corners[4],
            Vec3f{-1,0,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Right face (x = +1)
    pushQuad(corners[1], corners[5], corners[6], corners[2],
            Vec3f{1,0,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Bottom face (y = -1)
    pushQuad(corners[0], corners[4], corners[5], corners[1],
            Vec3f{0,-1,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Top face (y = +1)
    pushQuad(corners[3], corners[2], corners[6], corners[7],
            Vec3f{0,1,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});
    
    return vertices;
}

// Cylinder: radius=1, height from y=-1 to y=+1, centered at origin
std::vector<Vertex> CreateCylinder(const Vec3f& color, int slices = 32)
{
    std::vector<Vertex> vertices;

    const float pi = RAMEN_PI;

    // Vorberechnung der Punkte auf dem Ring für Top und Bottom.
    std::vector<Vec3f> top(slices);
    std::vector<Vec3f> bot(slices);
    for (int i = 0; i < slices; ++i)
    {
        // Aktueller Winkel für die Berechnung der x- und z-Koordinaten der Ringpunkte.
        float theta = 2.f * pi * i / slices;
        
        // Berechnet x-Koordinate auf Einheitskreis.
        float x = std::cos(theta);

        // Berechnet z-Koordinate auf Einheitskreis.
        float z = std::sin(theta);

        top[i] = Vec3f{ x,  1.f, z }; // y=0 ist Zentrum vom Zylinder, also y=1 ist die Oberseite.
        bot[i] = Vec3f{ x, -1.f, z };
    }

    Vec3f capTop{ 0.f,  1.f, 0.f }; // Mittelpunkt der oberen Kreisfläche (y=1)
    Vec3f capBot{ 0.f, -1.f, 0.f }; // Mittelpunkt der unteren Kreisfläche (y=-1)
    Vec3f nTop  { 0.f,  1.f, 0.f }; // Normalenvektor für die obere Kreisfläche (zeigt nach oben)
    Vec3f nBot  { 0.f, -1.f, 0.f }; // Normalenvektor für die untere Kreisfläche (zeigt nach unten)

    // Pro Durchgang werden erzeugt:
    //  2 Dreiecke für den Mantel
    //  1 Dreieck für den Deckel (Kreisfläche)
    //  1 Dreieck für den Boden (Kreisfläche)
    for (int i = 0; i < slices; ++i)
    {
        int j = (i + 1) % slices; // Index des nächsten Punkts im Ring, mit Wrap-Around
        
        // UV-Koordinaten für die Seitenfläche: u entlang des Rings
        float u0 = float(i) / slices; // u0=0 am Startpunkt
        float u1 = float(i + 1) / slices; // u1=1 am Ende des Rings (wenn i+1 == slices, dann wrap-around zu 0, was korrekt ist)

        // Side quads (Mantel) (zwei Dreiecke pro Segment)
        // Normalen für die Seitenfläche: zeigen radial nach außen, also in Richtung der xz-Koordinaten der Ringpunkte.
        // [Bild zeichnen]
        Vec3f n0{ bot[i].x, 0.f, bot[i].z }; // Normalenvektor für den Punkt bot[i] auf der Seitenfläche (y-Komponente ist 0, da die Normalen horizontal sind)
        Vec3f n1{ bot[j].x, 0.f, bot[j].z }; // Normalenvektor für den Punkt bot[j] auf der Seitenfläche

        
        // Triangle 1: bot[i], top[i], top[j]
            // UV-Koordinaten gehen von (0,0) unten links bis (1,1) oben rechts.
        vertices.push_back({ bot[i], n0, color, Vec3f{u0, 0.f, 0.f} });
        vertices.push_back({ top[i], n0, color, Vec3f{u0, 1.f, 0.f} });
        vertices.push_back({ top[j], n1, color, Vec3f{u1, 1.f, 0.f} });
        // Triangle 2: bot[i], top[j], bot[j]
        vertices.push_back({ bot[i], n0, color, Vec3f{u0, 0.f, 0.f} });
        vertices.push_back({ top[j], n1, color, Vec3f{u1, 1.f, 0.f} });
        vertices.push_back({ bot[j], n1, color, Vec3f{u1, 0.f, 0.f} });

        // Top cap (Deckel)
        // UV: Map Einheitskreis auf [0,1]^2
            // top[i].x und top[i].z liegen im Bereich [-1, 1]. Um sie in den Bereich [0, 1] zu bringen, wird die Formel (x * 0.5 + 0.5) verwendet.
            // Die Skalierung mit 0.5 reduziert den Bereich von [-1, 1] auf [-0.5, 0.5],
            // und die anschließende Addition von 0.5 verschiebt den Bereich auf [0, 1]. 
            // Ein Randpunkt (-1, -1) wird zu (0, 0), (+1, +1) zu (1, 1).
        Vec3f uvC { 0.5f, 0.5f, 0.f }; // Mittelpunkt der Kreisfläche (0,0) soll auf (0.5,0.5) in UV liegen
        Vec3f uv0T{ top[i].x * .5f + .5f, top[i].z * .5f + .5f, 0.f };
        Vec3f uv1T{ top[j].x * .5f + .5f, top[j].z * .5f + .5f, 0.f };
        // ccw when viewed from +y
        vertices.push_back({ capTop, nTop, color, uvC  });
        vertices.push_back({ top[j], nTop, color, uv1T });
        vertices.push_back({ top[i], nTop, color, uv0T });

        // Bottom cap (Boden)
        // UV: Map Einheitskreis auf [0,1]^2
        Vec3f uv0B{ bot[i].x * .5f + .5f, bot[i].z * .5f + .5f, 0.f };
        Vec3f uv1B{ bot[j].x * .5f + .5f, bot[j].z * .5f + .5f, 0.f };
        // ccw when viewed from -y
        vertices.push_back({ capBot, nBot, color, uvC  });
        vertices.push_back({ bot[i], nBot, color, uv0B });
        vertices.push_back({ bot[j], nBot, color, uv1B });
    }

    return vertices;
}

std::vector<Vertex> CreateSphere(const Vec3f& color, int stacks = 16, int slices = 32)
{
    std::vector<Vertex> vertices;

    const float pi = RAMEN_PI;

    // Stacks: horizontale Bänder von Dreiecken, von oben (phi=0) nach unten (phi=pi) (Breitengrade)
    // Slices: vertikale Segmente von Dreiecken, um die y-Achse herum, von 0 bis 2*pi (Längengrade)

    // phi   in [0, pi]   top→bottom
    // theta in [0, 2*pi] around y-axis

    // Man läuft Ring für Ring durch (stacks/ vertikale Bänder) und läuft dann horizontal durch die Segmente (slices) eines Bandes.
        // Pro Segment werden 2 Dreiecke erzeugt, die ein kleines Rechteck auf der Kugeloberfläche bilden.  
    for (int s = 0; s < stacks; ++s)
    {
        float phi0 = pi * s       / stacks;   // top of band
        float phi1 = pi * (s + 1) / stacks;   // bottom of band

        for (int i = 0; i < slices; ++i)
        {
            // theta ist der Drehwinkel um y-Achse, der die Position der Punkte auf dem Ring definiert.
                // theta0: Startwinkel des aktuellen Segements
                // theta1: Endwinkel des aktuellen Segments
            float theta0 = 2.f * pi * i       / slices;
            float theta1 = 2.f * pi * (i + 1) / slices;

            // Unit-sphere positions (= outward normals)
            auto pos = [](float phi, float theta) -> Vec3f {
                return Vec3f{
                    std::sin(phi) * std::cos(theta), // x
                    std::cos(phi),                   // y
                    std::sin(phi) * std::sin(theta)  // z
                };
            };

            Vec3f p00 = pos(phi0, theta0);  Vec3f p01 = pos(phi0, theta1); // Ring oben, rechts nach links
            Vec3f p10 = pos(phi1, theta0);  Vec3f p11 = pos(phi1, theta1); // Ring unten, rechts nah links

            // UV: u = theta/(2pi), v = phi/pi -> Umrechnung in [0,1]^2
                // theta / (2*pi) = Ein Wert zwischen 0.0 und 1.0 auf der U-Achse (links nach rechts).
                // phi / pi = Ein Wert zwischen 0.0 und 1.0 auf der V-Achse (oben nach unten).
            // 1.0f - phi0 / pi, 0.f -> sodass uv-Koordinaten (0, 0) oben links und (1, 1) unten rechts
            Vec3f uv00{ theta0 / (2 * pi), 1.0f - phi0 / pi, 0.f };
            Vec3f uv01{ theta1 / (2 * pi), 1.0f - phi0 / pi, 0.f };
            Vec3f uv10{ theta0 / (2 * pi), 1.0f - phi1 / pi, 0.f };
            Vec3f uv11{ theta1 / (2 * pi), 1.0f - phi1 / pi, 0.f };

            // Auf einer Einheitskugel ist die Normalenrichtung gleich der Position,
                // da die Normalenvektoren von der Mitte der Kugel nach außen zeigen
            // Triangle 1: p00, p11, p10  (ccw from outside)
            vertices.push_back({ p00, p00, color, uv00 });
            vertices.push_back({ p11, p11, color, uv11 });
            vertices.push_back({ p10, p10, color, uv10 });
            // Triangle 2: p00, p01, p11  (ccw from outside)
            vertices.push_back({ p00, p00, color, uv00 });
            vertices.push_back({ p01, p01, color, uv01 });
            vertices.push_back({ p11, p11, color, uv11 });
        }
    }

    return vertices;
}

// Erzeugt Liniensegmente zur Visualisierung der Normalenvektoren.
std::vector<Vertex> CreateNormalLines(const std::vector<Vertex>& mesh, float scale = 0.2f)
{
    std::vector<Vertex> lines;

    // Pro Vertex wird eine Linie von der Position zur Position + Normale * scale erzeugt.
    for (const Vertex& v : mesh)
    {
        // Die Farbe der Linie wird aus der Normalenrichtung berechnet: Mapping von [-1,1] auf [0,1].
        Vec3f color{
            v.normal.x * 0.5f + 0.5f, // *0.5f + 0.5f -> Verschiebung des RGB-Werts von [-1,1] auf [0,1]
            v.normal.y * 0.5f + 0.5f,
            v.normal.z * 0.5f + 0.5f
        };
        lines.push_back({ v.position, v.normal, color, Vec3f{0,0,0} });
        lines.push_back({
            Vec3f{ v.position.x + v.normal.x * scale, // position -> Ankerpunkt
                   v.position.y + v.normal.y * scale,
                   v.position.z + v.normal.z * scale },
            v.normal, color, Vec3f{0,0,0}
        });
    }
    return lines;
}

// Erzeugt eine Linie von Ursprung Richtung Y-Achse
// Visualisiert Rotationsachse, um SelfRotation der Planeten zu zeigen.
std::vector<Vertex> CreateRotationAxis(float scale = 1.5f) {
    std::vector<Vertex> lines;

    // 60° von der Y-Achse geneigt (in XY-Ebene): sin(60°)=0.866, cos(60°)=0.5
    Vec3f dir{ 0.866025f, 0.5f, 0.0f };
    Vec3f color{1.0f, 0.0f, 1.0f};
    lines.push_back({ Vec3f{0,0,0},                                  dir, color, Vec3f{0,0,0} });
    lines.push_back({ Vec3f{dir.x*scale, dir.y*scale, dir.z*scale}, dir, color, Vec3f{0,0,0} });
    return lines;
}

    int
    main(int argc, char** argv)
{
    Filesystem* pFS = Filesystem::Init(argc, argv, "../../assets");

    Ramen* pRamen = Ramen::Instance();
    pRamen->Init("GUI", 800, 600);

    /* Load shaders. */
    Shader shader{};
    if ( !shader.Load("shaders/task03.vert", "shaders/task03.frag") )
    {
        fprintf(stderr, "Could not load shader.\n");
    }

    /* Create camera */
    Camera camera(Vec3f{ 0.0f, 1.0f, 3.0f });
    camera.RotateAroundSide(-10.0f); // leicht nach unten geneigt

    /* Model mat*/
    Mat4f modelMat = Mat4f::Identity();

    /* Use coordinate system as a dummy model so you see how VBO, VAOs work again. */
    GLuint VBO_Coord;
    glCreateBuffers(1, &VBO_Coord);
    glNamedBufferData(VBO_Coord, 6 * sizeof(Vertex), Utils::CoordSystemRHZU(), GL_STATIC_DRAW);

    /* VAO. */
    GLuint VAO_Coord;
    glCreateVertexArrays(1, &VAO_Coord);
    glVertexArrayVertexBuffer(VAO_Coord, 0, VBO_Coord, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
        // 0=index of the generic vertex attribute to be modified.
        // 3=number of components per generic vertex attribute. Must be 1, 2, 3, or 4. (x,y,z)
        // GL_FLOAT=data type of each component in the array. (float)
        // GL_FALSE=normalized (GL_FALSE means that the values will be directly converted to float without normalization)
        // 0=offset of the first component of the first generic vertex attribute in the array in the data store of the buffer currently bound to the GL_ARRAY_BUFFER target. (position is at offset 0)
    glVertexArrayAttribFormat(VAO_Coord, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Coord, 0);
        // Aktiviert Vertex-Attribut mit Index 0
    glVertexArrayAttribBinding(VAO_Coord, 0, 0);
        // Binds the vertex buffer to the vertex array object at the specified binding index (0 in this case).
        //  This tells OpenGL which buffer to use for the vertex attribute data when rendering with this VAO.
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Coord, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Coord, 1);
    glVertexArrayAttribBinding(VAO_Coord, 1, 0);
    // TODO: Aufgabe 3.1) Nutzen der Farbattribute.
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Coord, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Coord, 2);
    glVertexArrayAttribBinding(VAO_Coord, 2, 0);

    // TODO: Aufgabe 3.2) Erzeugung von Quader
    std::vector<Vertex> cubeVertices = CreateCube(Vec3f{1.0f, 0.0f, 0.0f}); // Red cube
    GLuint VBO_Cube;
    glCreateBuffers(1, &VBO_Cube);
    glNamedBufferData(VBO_Cube, cubeVertices.size() * sizeof(Vertex), cubeVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_Cube;
    glCreateVertexArrays(1, &VAO_Cube);
    glVertexArrayVertexBuffer(VAO_Cube, 0, VBO_Cube, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Cube, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Cube, 0);
    glVertexArrayAttribBinding(VAO_Cube, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Cube, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Cube, 1);
    glVertexArrayAttribBinding(VAO_Cube, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Cube, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Cube, 2);
    glVertexArrayAttribBinding(VAO_Cube, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Cube, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Cube, 3);
    glVertexArrayAttribBinding(VAO_Cube, 3, 0);

    // TODO: Aufgabe 3.2) Erzeugung von Zylinder
    std::vector<Vertex> cylinderVertices = CreateCylinder(Vec3f{0.0f, 1.0f, 0.0f}); // Green cylinder
    GLuint VBO_Cylinder;
    glCreateBuffers(1, &VBO_Cylinder);
    glNamedBufferData(VBO_Cylinder, cylinderVertices.size() * sizeof(Vertex), cylinderVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_Cylinder;
    glCreateVertexArrays(1, &VAO_Cylinder);
    glVertexArrayVertexBuffer(VAO_Cylinder, 0, VBO_Cylinder, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Cylinder, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Cylinder, 0);
    glVertexArrayAttribBinding(VAO_Cylinder, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Cylinder, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Cylinder, 1);
    glVertexArrayAttribBinding(VAO_Cylinder, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Cylinder, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Cylinder, 2);
    glVertexArrayAttribBinding(VAO_Cylinder, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Cylinder, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Cylinder, 3);
    glVertexArrayAttribBinding(VAO_Cylinder, 3, 0);

    // TODO: Aufgabe 3.2) Erzeugung von Kugeln
    std::vector<Vertex> sphereVertices = CreateSphere(Vec3f{0.0f, 0.0f, 1.0f}); // Blue sphere
    GLuint VBO_Sphere;
    glCreateBuffers(1, &VBO_Sphere);
    glNamedBufferData(VBO_Sphere, sphereVertices.size() * sizeof(Vertex), sphereVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_Sphere;
    glCreateVertexArrays(1, &VAO_Sphere);
    glVertexArrayVertexBuffer(VAO_Sphere, 0, VBO_Sphere, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Sphere, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Sphere, 0);
    glVertexArrayAttribBinding(VAO_Sphere, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Sphere, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Sphere, 1);
    glVertexArrayAttribBinding(VAO_Sphere, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Sphere, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Sphere, 2);
    glVertexArrayAttribBinding(VAO_Sphere, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Sphere, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Sphere, 3);
    glVertexArrayAttribBinding(VAO_Sphere, 3, 0);

    // Normal visualization: Liniensegmente für jeden Mesh
    auto makeNormalVAO = [](GLuint& vbo, GLuint& vao, const std::vector<Vertex>& lines)
    {
        glCreateBuffers(1, &vbo);
        glNamedBufferData(vbo, lines.size() * sizeof(Vertex), lines.data(), GL_STATIC_DRAW);
        glCreateVertexArrays(1, &vao);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
        glVertexArrayAttribFormat(vao, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
        glEnableVertexArrayAttrib(vao, 2);
        glVertexArrayAttribBinding(vao, 2, 0);
    };

    // TODO: Aufgabe 3.4) Anzeige/ Debugging von Normalenvektoren
    std::vector<Vertex> cubeNormals     = CreateNormalLines(cubeVertices);
    std::vector<Vertex> cylinderNormals = CreateNormalLines(cylinderVertices);
    std::vector<Vertex> sphereNormals   = CreateNormalLines(sphereVertices);

    GLuint VBO_CubeNormals, VAO_CubeNormals;
    makeNormalVAO(VBO_CubeNormals, VAO_CubeNormals, cubeNormals);

    GLuint VBO_CylinderNormals, VAO_CylinderNormals;
    makeNormalVAO(VBO_CylinderNormals, VAO_CylinderNormals, cylinderNormals);

    GLuint VBO_SphereNormals, VAO_SphereNormals;
    makeNormalVAO(VBO_SphereNormals, VAO_SphereNormals, sphereNormals);

    // TODO: Aufgabe 3.7) Animation
    // Sonne
    std::vector<Vertex> sunVertices = CreateSphere(Vec3f{ 1.0f, 1.0f, 0.0f }); // Yellow sphere
    GLuint              VBO_Sun;
    glCreateBuffers(1, &VBO_Sun);
    glNamedBufferData(VBO_Sun, sunVertices.size() * sizeof(Vertex), sunVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_Sun;
    glCreateVertexArrays(1, &VAO_Sun);
    glVertexArrayVertexBuffer(VAO_Sun, 0, VBO_Sun, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Sun, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Sun, 0);
    glVertexArrayAttribBinding(VAO_Sun, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Sun, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Sun, 1);
    glVertexArrayAttribBinding(VAO_Sun, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Sun, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Sun, 2);
    glVertexArrayAttribBinding(VAO_Sun, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Sun, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Sun, 3);
    glVertexArrayAttribBinding(VAO_Sun, 3, 0);

    // Visualisiere Rotationsachse des Planetens.
    std::vector<Vertex> sunRotationAxis = CreateRotationAxis();
    GLuint VBO_SunRotationAxis, VAO_SunRotationAxis;
    makeNormalVAO(VBO_SunRotationAxis, VAO_SunRotationAxis, sunRotationAxis);

    // Erde
    std::vector<Vertex> earthVertices = CreateSphere(Vec3f{ 0.0f, 0.0f, 1.0f }); // Blue sphere
    GLuint              VBO_Earth;
    glCreateBuffers(1, &VBO_Earth);
    glNamedBufferData(VBO_Earth, earthVertices.size() * sizeof(Vertex), earthVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_Earth;
    glCreateVertexArrays(1, &VAO_Earth);
    glVertexArrayVertexBuffer(VAO_Earth, 0, VBO_Earth, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Earth, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Earth, 0);
    glVertexArrayAttribBinding(VAO_Earth, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Earth, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Earth, 1);
    glVertexArrayAttribBinding(VAO_Earth, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Earth, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Earth, 2);
    glVertexArrayAttribBinding(VAO_Earth, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Earth, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Earth, 3);
    glVertexArrayAttribBinding(VAO_Earth, 3, 0);

    // Visualisiere Rotationsachse des Planetens.
    std::vector<Vertex> earthRotationAxis = CreateRotationAxis();
    GLuint VBO_EarthRotationAxis, VAO_EarthRotationAxis;
    makeNormalVAO(VBO_EarthRotationAxis, VAO_EarthRotationAxis, earthRotationAxis);

    // Mond
    std::vector<Vertex> moonVertices = CreateSphere(Vec3f{ 0.5f, 0.5f, 0.5f }); // Gray sphere
    GLuint              VBO_Moon;
    glCreateBuffers(1, &VBO_Moon);
    glNamedBufferData(VBO_Moon, moonVertices.size() * sizeof(Vertex), moonVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_Moon;
    glCreateVertexArrays(1, &VAO_Moon);
    glVertexArrayVertexBuffer(VAO_Moon, 0, VBO_Moon, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Moon, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Moon, 0);
    glVertexArrayAttribBinding(VAO_Moon, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Moon, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Moon, 1);
    glVertexArrayAttribBinding(VAO_Moon, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Moon, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Moon, 2);
    glVertexArrayAttribBinding(VAO_Moon, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Moon, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Moon, 3);
    glVertexArrayAttribBinding(VAO_Moon, 3, 0);

    // Visualisiere Rotationsachse des Planetens.
    std::vector<Vertex> moonRotationAxis = CreateRotationAxis();
    GLuint VBO_MoonRotationAxis, VAO_MoonRotationAxis;
    makeNormalVAO(VBO_MoonRotationAxis, VAO_MoonRotationAxis, moonRotationAxis);

    // Mars
    std::vector<Vertex> marsVertices = CreateSphere(Vec3f{ 1.0f, 0.0f, 0.0f }); // Red sphere
    GLuint              VBO_Mars;
    glCreateBuffers(1, &VBO_Mars);
    glNamedBufferData(VBO_Mars, marsVertices.size() * sizeof(Vertex), marsVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_Mars;
    glCreateVertexArrays(1, &VAO_Mars);
    glVertexArrayVertexBuffer(VAO_Mars, 0, VBO_Mars, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Mars, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Mars, 0);
    glVertexArrayAttribBinding(VAO_Mars, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Mars, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Mars, 1);
    glVertexArrayAttribBinding(VAO_Mars, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Mars, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Mars, 2);
    glVertexArrayAttribBinding(VAO_Mars, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Mars, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Mars, 3);
    glVertexArrayAttribBinding(VAO_Mars, 3, 0);

    // Visualisiere Rotationsachse des Planetens.
    std::vector<Vertex> marsRotationAxis = CreateRotationAxis();
    GLuint VBO_MarsRotationAxis, VAO_MarsRotationAxis;
    makeNormalVAO(VBO_MarsRotationAxis, VAO_MarsRotationAxis, marsRotationAxis);

    // Variablen für Sonnensystem
    float sunAngle = 0.0f;
    float earthOrbitAngle = 0.0f;
    float earthSelfAngle = 0.0f;
    float moonAngle = 0.0f;
    float marsAngle = 0.0f;

    /* Some global GL states */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    // glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* Main loop */
    bool isRunning = true;
    SDL_GL_SetSwapInterval(1); /* 1 = VSync enabled; 0 = VSync disabled */

    Uint64 ticksPerSecond = SDL_GetPerformanceFrequency();
    Uint64 startCounter   = SDL_GetPerformanceCounter();
    Uint64 endCounter     = SDL_GetPerformanceCounter();
    while ( isRunning ) {
        double ticksPerFrame = (double)endCounter - (double)startCounter;
        double msPerFrame    = (ticksPerFrame / (double)ticksPerSecond) * 1000.0;
        double deltaTime = ticksPerFrame / (double)ticksPerSecond; // in Sekunden
        startCounter         = SDL_GetPerformanceCounter();

        SDL_Event e;
        while ( SDL_PollEvent(&e) ) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            pRamen->ProcessInputEvent(e);

            if ( e.type == SDL_EVENT_QUIT ) {
                isRunning = false;
            }

            if ( e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE )
                isRunning = false;
        }

        // TODO: Aufgabe 3.6)
        // Nicht unter SDL_PollEvent, weil der Input dann hackt, durch Hardware-Delay Falling/ Rising-Edge Delay.
        {
            const float rotateStep = 1.5f;  /* degrees per frame */
            const float moveStep   = 0.05f; /* units per frame   */

            /* Camera movement - Rotation */
            if ( pRamen->KeyWentDown(SDLK_UP) || pRamen->KeyPressed(SDLK_UP) )
                /* TODO: Pitch up camera */
                camera.Pitch(rotateStep);
            else if ( pRamen->KeyWentDown(SDLK_DOWN) || pRamen->KeyPressed(SDLK_DOWN) )
                /* TODO: Pitch down camera */
                camera.Pitch(-rotateStep);
            else if ( pRamen->KeyWentDown(SDLK_LEFT) || pRamen->KeyPressed(SDLK_LEFT) )
                /* TODO: Yaw left camera */
                camera.Yaw(rotateStep);
            else if ( pRamen->KeyWentDown(SDLK_RIGHT) || pRamen->KeyPressed(SDLK_RIGHT) )
                /* TODO: Yaw right camera */
                camera.Yaw(-rotateStep);
            else if ( pRamen->KeyWentDown(SDLK_E) || pRamen->KeyPressed(SDLK_E) )
                /* TODO: Roll right camera */
                camera.Roll(rotateStep);
            else if ( pRamen->KeyWentDown(SDLK_Q) || pRamen->KeyPressed(SDLK_Q) )
                /* TODO: Roll left camera */
                camera.Roll(-rotateStep);

            /* Camera movement - Translation */
            else if ( pRamen->KeyWentDown(SDLK_W) || pRamen->KeyPressed(SDLK_W) )
                /* TODO: Move camera forward */
                camera.DollyForward(moveStep);
            else if ( pRamen->KeyWentDown(SDLK_S) || pRamen->KeyPressed(SDLK_S) )
                /* TODO: Move camera backward */
                camera.DollyForward(-moveStep);
            else if ( pRamen->KeyWentDown(SDLK_A) || pRamen->KeyPressed(SDLK_A) )
                /* TODO: Move camera left */
                camera.DollySide(-moveStep);
            else if ( pRamen->KeyWentDown(SDLK_D) || pRamen->KeyPressed(SDLK_D) )
                /* TODO: Move camera right */
                camera.DollySide(moveStep);
        }

        /* Query new frame dimensions */
        int windowWidth, windowHeight;
        SDL_GetWindowSize(pRamen->GetWindow(), &windowWidth, &windowHeight);

        /* Adjust viewport and perspective projection accordingly. */
        glViewport(0, 0, windowWidth, windowHeight);

        /* View mat */
        Mat4f viewMat = LookAt(
            camera.GetPosition(), camera.GetPosition() + camera.GetForward(), camera.GetUp()); // Mat4f::Identity();

        /* Projection mat */
        float aspect  = (float)windowWidth / (float)windowHeight;
        Mat4f projMat = PerspectiveProjection(TO_RAD(60.0f), aspect, 0.01f, 500.0f);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        /* ImGUI Rendering */
        ImGui::Render();

        /* Rendering */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

        shader.Use();
        glBindVertexArray(VAO_Coord);
        
        // Beschreibt, wie das Modell im Weltenkoordinatensystem transformiert wird. (z.B. Translation, Rotation, Skalierung)
        glUniformMatrix4fv(0, 1, GL_FALSE, modelMat.Data());
        
        // Beschreibt die Position und Orientierung der Kamera. (z.B. Kamera-Position, Blickrichtung, Up-Vektor)
        glUniformMatrix4fv(1, 1, GL_FALSE, viewMat.Data());
        
        // Beschreibt die Projektion, also wie die 3D-Szene auf den 2D-Bildschirm projiziert wird. (z.B. Perspektivische oder orthografische Projektion)
            // Sorgt, dass entfernte Objekte kleiner erscheinen, was einen realistischen 3D-Effekt erzeugt.
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());

        // TODO: Aufgabe 3.5)
        // Lichtquelle in Weltkoordinaten
        Vec3f lightPos{ 15.0f, 0.0f, 0.0f }; // 5.0f, 0.0f, -5.0f
        glUniform3fv(3, 1, lightPos.Data()); // als uniform in fragment-shader an pos3 packen

        glDrawArrays(GL_LINES, 0, 6); // Zeichne die Weltkoordinatenachsen (6 Vertices = 3 Linien für die Koordinatenachsen)
        // GL_Lines = Zeichenmodus; jedes Paar von Vertices wird als Linie interpretiert. (6 Vertices = 3 Linien für die Koordinatenachsen)

        // Basis-Transformation für alle Modelle: Skalierung, damit sie nicht zu groß sind.
        Mat4f baseScale = Scale(Vec3f{ 0.4f, 0.4f, 0.4f });

        // Draw the cube.
        Mat4f cubeModel = modelMat * Translate(Vec3f{0.0f, 0.0f, 0.0f}) * baseScale;
        glUniformMatrix4fv(0, 1, GL_FALSE, cubeModel.Data());
        glBindVertexArray(VAO_Cube);
        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());
        // Normalenvektoren des Würfels
        glBindVertexArray(VAO_CubeNormals);
        glDrawArrays(GL_LINES, 0, (GLsizei)cubeNormals.size());

        // Draw the cylinder.
        Mat4f cylinderModel = modelMat * Translate(Vec3f{-1.2f, 0.0f, 0.0f}) * baseScale;
        glUniformMatrix4fv(0, 1, GL_FALSE, cylinderModel.Data());
        glBindVertexArray(VAO_Cylinder);
        glDrawArrays(GL_TRIANGLES, 0, cylinderVertices.size());
        // Normalenvektoren des Zylinders
        glBindVertexArray(VAO_CylinderNormals);
        glDrawArrays(GL_LINES, 0, (GLsizei)cylinderNormals.size());

        // Draw the sphere.
        Mat4f sphereModel = modelMat * Translate(Vec3f{1.2f, 0.0f, 0.0f}) * baseScale;
        glUniformMatrix4fv(0, 1, GL_FALSE, sphereModel.Data());
        glBindVertexArray(VAO_Sphere);
        glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size());
        // Normalenvektoren der Kugel
        glBindVertexArray(VAO_SphereNormals);
        glDrawArrays(GL_LINES, 0, (GLsizei)sphereNormals.size());

        // TODO: Aufgabe 3.7) Animation -> Sonnnsystem
        MatrixStack matrixStack = MatrixStack();
        matrixStack.Translate(Vec3f{15.0f, 0.0f, 0.0f}); // Verschiebung des gesamten Sonnensystems nach rechts
        matrixStack.Scale(Vec3f{0.4f, 0.4f, 0.4f}); // Basis-Skalierung für alle Modelle

        sunAngle += 30.0f * deltaTime;
        earthOrbitAngle += 10.0f * deltaTime;
        earthSelfAngle += 5.0f * deltaTime;
        moonAngle += 150.0f * deltaTime;
        marsAngle += 8.0f * deltaTime;
        
        // Wichtig: Verständnis: Vertices erleben die Operationen von rechts nach links — also zuletzt hinzugefügt = zuerst angewendet
        // Sonne
        matrixStack.Push();
        matrixStack.Rotate(RAMEN_WORLD_UP, sunAngle);
        glUniformMatrix4fv(0, 1, GL_FALSE, matrixStack.Last().Data());
        glBindVertexArray(VAO_Sun);
        glDrawArrays(GL_TRIANGLES, 0, sunVertices.size());
        // Rotationsachse
        glBindVertexArray(VAO_SunRotationAxis);
        glDrawArrays(GL_LINES, 0, (GLsizei)sunRotationAxis.size());

        // Erde
        matrixStack.Push();
        matrixStack.Rotate(RAMEN_WORLD_UP, earthOrbitAngle);
        matrixStack.Translate(Vec3f{4.0f, 0.0f, 0.0f});
        matrixStack.Rotate(RAMEN_WORLD_UP, earthSelfAngle);
        matrixStack.Scale(Vec3f{ 0.5f, 0.5f, 0.5f }); // Halb so groß wie Sonne
        glUniformMatrix4fv(0, 1, GL_FALSE, matrixStack.Last().Data());
        glBindVertexArray(VAO_Earth);
        glDrawArrays(GL_TRIANGLES, 0, earthVertices.size());
        // Rotationsachse
        glBindVertexArray(VAO_EarthRotationAxis);
        glDrawArrays(GL_LINES, 0, (GLsizei)earthRotationAxis.size());

        // Mond
        matrixStack.Push();
        matrixStack.Rotate(RAMEN_WORLD_UP, moonAngle);
        matrixStack.Translate(Vec3f{ 2.0f, 0.0f, 0.0f });
        matrixStack.Scale(Vec3f{ 0.5f, 0.5f, 0.5f }); // Mond halb so groß wie Erde
        glUniformMatrix4fv(0, 1, GL_FALSE, matrixStack.Last().Data());
        glBindVertexArray(VAO_Moon);
        glDrawArrays(GL_TRIANGLES, 0, moonVertices.size());
        // Rotationsachse
        glBindVertexArray(VAO_MoonRotationAxis);
        glDrawArrays(GL_LINES, 0, (GLsizei)moonRotationAxis.size());

        // Mars
        matrixStack.Pop(); // Moon
        matrixStack.Pop(); // Earth
        matrixStack.Push(); // Ebene Sonne
        matrixStack.Rotate(RAMEN_WORLD_UP, marsAngle);
        matrixStack.Translate(Vec3f{ 8.0f, 0.0f, 0.0f });
        matrixStack.Scale(Vec3f{ 0.25f, 0.25f, 0.25f }); // 1/4 so groß wie Sonne
        glUniformMatrix4fv(0, 1, GL_FALSE, matrixStack.Last().Data());
        glBindVertexArray(VAO_Mars);
        glDrawArrays(GL_TRIANGLES, 0, marsVertices.size());
        // Rotationsachse
        glBindVertexArray(VAO_MarsRotationAxis);
        glDrawArrays(GL_LINES, 0, (GLsizei)marsRotationAxis.size());

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        pRamen->EndFrame();
        SDL_GL_SwapWindow(pRamen->GetWindow());

        endCounter = SDL_GetPerformanceCounter();
    }

    /* GL Resources shutdown. */
    shader.Delete();
    glDeleteBuffers(1, &VBO_Coord);
    glDeleteVertexArrays(1, &VAO_Coord);

    glDeleteBuffers(1, &VBO_Cube);
    glDeleteVertexArrays(1, &VAO_Cube);
    glDeleteBuffers(1, &VBO_Cylinder);
    glDeleteVertexArrays(1, &VAO_Cylinder);
    glDeleteBuffers(1, &VBO_Sphere);
    glDeleteVertexArrays(1, &VAO_Sphere);

    glDeleteBuffers(1, &VBO_CubeNormals);
    glDeleteVertexArrays(1, &VAO_CubeNormals);
    glDeleteBuffers(1, &VBO_CylinderNormals);
    glDeleteVertexArrays(1, &VAO_CylinderNormals);
    glDeleteBuffers(1, &VBO_SphereNormals);
    glDeleteVertexArrays(1, &VAO_SphereNormals);

    /* Ramen Shutdown */
    pRamen->Shutdown();

    /* Filesystem deinit */
    PHYSFS_deinit();

    return 0;
}
