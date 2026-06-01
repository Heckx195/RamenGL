#include <glad/glad.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stb_image.h>

#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <physfs.h>

#include "../ramen/ramen.h"
#include "../ramen/rgl_camera.h"
#include "../ramen/rgl_defines.h"
#include "../ramen/rgl_filesystem.h"
#include "../ramen/rgl_image.h"
#include "../ramen/rgl_math.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../ramen/rgl_model.h"
#include "../ramen/rgl_shader.h"

// Creates a cube with frontfaces inside of the cube.
std::vector<Vertex> CreateSkyboxCube(const Vec3f& color)
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
        // Dreieck 1: a, b, c -> von innen ccw
        vertices.push_back({a, n, color, uva});
        vertices.push_back({b, n, color, uvb});
        vertices.push_back({c, n, color, uvc});
        // Dreieck 2: a, c, d -> von innen ccw
        vertices.push_back({a, n, color, uva});
        vertices.push_back({c, n, color, uvc});
        vertices.push_back({d, n, color, uvd});
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

// Von https://learnopengl.com/Advanced-OpenGL/Cubemaps
// aber auf neuste OpenGL Funktionen aktualisiert.
void loadCubemap(
    GLuint& textureID,
    const std::vector<std::string>& faces
) {
    
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);

    int width, height, nrChannels;
    bool storageAllocated = false;

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        File imgFile = Filesystem::Instance()->Read(faces[i].c_str());
        unsigned char* data = stbi_load_from_memory((unsigned char*)imgFile.data, imgFile.m_size, &width, &height, &nrChannels, 0);
        if (data)
        {
            if (!storageAllocated)
            {
                // Nur einmal Speicher für die gesamte Cubemap allokieren. Möglich weil alle 6 Seiten gleich groß sind.
                glTextureStorage2D(textureID, 1, GL_RGB8, width, height);
                storageAllocated = true;
            }
            // i = Face-Index
                // i=0 -> GL_TEXTURE_CUBE_MAP_POSITIVE_X
                // i=1 -> GL_TEXTURE_CUBE_MAP_NEGATIVE_X
                // i=2 -> GL_TEXTURE_CUBE_MAP_POSITIVE_Y
                // i=3 -> GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
                // i=4 -> GL_TEXTURE_CUBE_MAP_POSITIVE_Z
                // i=5 -> GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
            glTextureSubImage3D(
                textureID,
                0, // level
                0, 0, // xoffset, yoffset
                i, // zoffset = Face-Index
                width,
                height,
                1,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                data
            );
            stbi_image_free(data); // Garbage collect.
        }
        else
        {
            fprintf(stderr, "Cubemap tex failed to load at path: %s\n", faces[i].c_str());
            stbi_image_free(data); // Kein Memory-Leak hehe.
        }
    }

    // -> Siehe Doku.
    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

// Aus Aufgabe 3.)
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
    for ( int s = 0; s < stacks; ++s )
    {
        float phi0 = pi * s / stacks;       // top of band
        float phi1 = pi * (s + 1) / stacks; // bottom of band

        for ( int i = 0; i < slices; ++i )
        {
            // theta ist der Drehwinkel um y-Achse, der die Position der Punkte auf dem Ring definiert.
            // theta0: Startwinkel des aktuellen Segements
            // theta1: Endwinkel des aktuellen Segments
            float theta0 = 2.f * pi * i / slices;
            float theta1 = 2.f * pi * (i + 1) / slices;

            // Unit-sphere positions (= outward normals)
            auto pos = [](float phi, float theta) -> Vec3f
            {
                return Vec3f{
                    std::sin(phi) * std::cos(theta), // x
                    std::cos(phi),                   // y
                    std::sin(phi) * std::sin(theta)  // z
                };
            };

            Vec3f p00 = pos(phi0, theta0);
            Vec3f p01 = pos(phi0, theta1); // Ring oben, rechts nach links
            Vec3f p10 = pos(phi1, theta0);
            Vec3f p11 = pos(phi1, theta1); // Ring unten, rechts nah links

            // UV: u = theta/(2pi), v = phi/pi -> Umrechnung in [0,1]^2
                // theta / (2*pi) = Ein Wert zwischen 0.0 und 1.0 auf der U-Achse (links nach rechts).
                // phi / pi = Ein Wert zwischen 0.0 und 1.0 auf der V-Achse (oben nach unten).
            // Wenn auf dem Kopf dann V-Koordinate ändern -> 1.0f - phi0 / pi, 0.f -> sodass uv-Koordinaten (0, 0) oben links und (1, 1) unten rechts
            // Wenn spiegelverkehrt dann U-Koordinate ändern -> 1.0f - theta...
            Vec3f uv00{ 1.0f - theta0 / (2 * pi), phi0 / pi, 0.f };
            Vec3f uv01{ 1.0f - theta1 / (2 * pi), phi0 / pi, 0.f };
            Vec3f uv10{ 1.0f - theta0 / (2 * pi), phi1 / pi, 0.f };
            Vec3f uv11{ 1.0f - theta1 / (2 * pi), phi1 / pi, 0.f };

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

// Aus Aufgabe 4.)
#define NUM_QUAD_VERTICES 4
#define NUM_QUAD_INDICES 6
static Vertex quadVertices[ NUM_QUAD_VERTICES ] = {
    // Pos, Normal, Color, UV
    { Vec3f{ -1.0f, 0.0f, 1.0f }, Vec3f{ 0.0f, 1.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f } }, // vorne links
    { Vec3f{ 1.0f, 0.0f, 1.0f }, Vec3f{ 0.0f, 1.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 1.0f }, Vec3f{ 1.0f, 1.0f, 0.0f } }, // vorne rechts
    { Vec3f{ -1.0f, 0.0f, -1.0f }, Vec3f{ 0.0f, 1.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 1.0f }, Vec3f{ 0.0f, 0.0f, 0.0f } }, // hinten links
    { Vec3f{ 1.0f, 0.0f, -1.0f }, Vec3f{ 0.0f, 1.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 1.0f }, Vec3f{ 0.0f, 1.0f, 0.0f } } // hinten rechts
};
static uint16_t quadIndices[ NUM_QUAD_INDICES ]   = { 0, 1, 3, 3, 2, 0 };

int main(int argc, char** argv)
{
    Filesystem* pFS = Filesystem::Init(argc, argv, "../../assets");

    Ramen* pRamen = Ramen::Instance();
    pRamen->Init("Task 06 - Shadow-Mapping", 800, 600);

    /* Load shaders. */
    Shader skyboxShader{};
    if ( !skyboxShader.Load("shaders/task05.vert", "shaders/task05.frag") )
    {
        fprintf(stderr, "Could not load skybox shader.\n");
    }

    // TODO: Aufgabe 5.2) Environment Mapping
    Shader envmapShader{};
    if ( !envmapShader.Load("shaders/envmap.vert", "shaders/envmap.frag") )
    {
        fprintf(stderr, "Could not load environment map shader.\n");
    }

    // TODO: Aufgabe 5.2) Environment Mapping
    Shader shadowMapShader{};
    if ( !shadowMapShader.Load("shaders/shadowmap.vert", "shaders/shadowmap.frag") )
    {
        fprintf(stderr, "Could not load shadow map shader.\n");
    }

    Shader flatShader{};
    if ( !flatShader.Load("shaders/flat.vert", "shaders/flat.frag") )
    {
        fprintf(stderr, "Could not load flat shader.\n");
    }

    Shader floorShader{};
    if ( !floorShader.Load("shaders/floor.vert", "shaders/floor.frag") )
    {
        fprintf(stderr, "Could not load floor shader.\n");
    }

    Model model{};
    if ( !model.Load("models/skull.obj") ) // nvidiaknight_origin // cylinder doenst have normals // bunny doesnt have normals
    {
        fprintf(stderr, "Could not load model file.\n");
    }
    
    // TODO: Create a buffer on GPU and upload the model's vertices.
    GLuint VBO_Model;
    glCreateBuffers(1, &VBO_Model);
    glNamedBufferData(VBO_Model, model.NumVertices() * sizeof(Vertex), model.GetVertices().data(), GL_STATIC_DRAW);
    
    GLuint VAO_Model;
    glCreateVertexArrays(1, &VAO_Model);
    glVertexArrayVertexBuffer(VAO_Model, 0, VBO_Model, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_Model, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Model, 0);
    glVertexArrayAttribBinding(VAO_Model, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Model, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Model, 1);
    glVertexArrayAttribBinding(VAO_Model, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Model, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Model, 2);
    glVertexArrayAttribBinding(VAO_Model, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_Model, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Model, 3);
    glVertexArrayAttribBinding(VAO_Model, 3, 0);

    /* Create camera */
    Vec3f cameraPosition = Vec3f{ 0.0f, 2.0f, 8.0f };
    Camera camera(cameraPosition);
    camera.RotateAroundSide(0.0f);

    /* Create lightCamera */
    Vec3f lightPos{ 5.0f, 8.0f, 5.0f }; // Lichtquelle in Weltkoordinaten
    Camera lightCamera(lightPos); // at the light source.
    lightCamera.Orient(Vec3f{ 0.0f, 0.0f, 0.0f }); // look towards origin.
    Vec3f lightCameraPosition = lightPos;

    /* Model mat*/
    Mat4f modelMat = Mat4f::Identity();

    // TODO: Aufgabe 5.1) Cubemap erstellen
    std::vector<Vertex> skyboxVertices = CreateSkyboxCube(Vec3f{1.0f, 0.0f, 0.0f});
    GLuint VBO_SkyboxCube;
    glCreateBuffers(1, &VBO_SkyboxCube);
    glNamedBufferData(VBO_SkyboxCube, skyboxVertices.size() * sizeof(Vertex), skyboxVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_SkyboxCube;
    glCreateVertexArrays(1, &VAO_SkyboxCube);
    glVertexArrayVertexBuffer(VAO_SkyboxCube, 0, VBO_SkyboxCube, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_SkyboxCube, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_SkyboxCube, 0);
    glVertexArrayAttribBinding(VAO_SkyboxCube, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_SkyboxCube, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_SkyboxCube, 1);
    glVertexArrayAttribBinding(VAO_SkyboxCube, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_SkyboxCube, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_SkyboxCube, 2);
    glVertexArrayAttribBinding(VAO_SkyboxCube, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_SkyboxCube, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_SkyboxCube, 3);
    glVertexArrayAttribBinding(VAO_SkyboxCube, 3, 0);

    // TODO: Aufgabe 5.1.1
    GLuint textureHandleCubemap;
    std::string prefix = "textures/cubemaps/NissiBeach/";
    loadCubemap(
        textureHandleCubemap,
        std::vector<std::string>{
            prefix + "posx.jpg",
            prefix + "negx.jpg",
            prefix + "posy.jpg",
            prefix + "negy.jpg",
            prefix + "posz.jpg",
            prefix + "negz.jpg"
        }
    );

    // TODO: Aufgabe 6.1) Shadomapping Framebuffer
    const int SHADOW_MAP_SIZE = 1024;
    GLuint FBO_Shadowmap;
    glCreateFramebuffers(1, &FBO_Shadowmap);

    GLuint textureHandleShadowMap;
    glCreateTextures(GL_TEXTURE_2D, 1, &textureHandleShadowMap);
    glTextureStorage2D(textureHandleShadowMap, 1, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glTextureParameteri(textureHandleShadowMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(textureHandleShadowMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // border-color. Weiß bedeutet, dass außerhalb der Shadowmap alles beleuchtet ist. Schwarz würde bedeuten, dass außerhalb der Shadowmap alles im Schatten liegt.
    glTextureParameterfv(textureHandleShadowMap, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTextureParameteri(textureHandleShadowMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(textureHandleShadowMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glNamedFramebufferTexture(FBO_Shadowmap, GL_DEPTH_ATTACHMENT, textureHandleShadowMap, 0); /* 0 = mipmap level */

    // TODO: Aufgabe 6.2) Kugel um Lichtquelle zu visualieren.
    std::vector<Vertex> lightSphereVertices = CreateSphere(Vec3f{ 1.0f, 1.0f, 0.0f });
    GLuint VBO_LightSphere;
    glCreateBuffers(1, &VBO_LightSphere);
    glNamedBufferData(VBO_LightSphere, lightSphereVertices.size() * sizeof(Vertex), lightSphereVertices.data(), GL_STATIC_DRAW);

    GLuint VAO_LightSphere;
    glCreateVertexArrays(1, &VAO_LightSphere);
    glVertexArrayVertexBuffer(VAO_LightSphere, 0, VBO_LightSphere, 0, sizeof(Vertex));
    /* Position */ // Position is at offset 0
    glVertexArrayAttribFormat(VAO_LightSphere, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_LightSphere, 0);
    glVertexArrayAttribBinding(VAO_LightSphere, 0, 0);
    /* Normal */ // Normal is at offset 3 * bytes size(float)
    glVertexArrayAttribFormat(VAO_LightSphere, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_LightSphere, 1);
    glVertexArrayAttribBinding(VAO_LightSphere, 1, 0);
    /* Color */ // Color is at offset 6 * bytes size(float)
    glVertexArrayAttribFormat(VAO_LightSphere, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_LightSphere, 2);
    glVertexArrayAttribBinding(VAO_LightSphere, 2, 0);
    /* uv */ // uv is at offset 9 * bytes size(float)
    glVertexArrayAttribFormat(VAO_LightSphere, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_LightSphere, 3);
    glVertexArrayAttribBinding(VAO_LightSphere, 3, 0);

    // TODO: Aufgabe 6.3) Create ZX-Plane out of quadVertices as shadow receiver.
    GLuint VBO_ZXPlane;
    glCreateBuffers(1, &VBO_ZXPlane);
    glNamedBufferData(VBO_ZXPlane, NUM_QUAD_VERTICES * sizeof(Vertex), quadVertices, GL_STATIC_DRAW);
    GLuint EBO_ZXPlane; // Ermöglicht Wiederverwendung von Vertexdaten durch Indizierung (Index Buffer Object)
    glCreateBuffers(1, &EBO_ZXPlane);
    glNamedBufferData(EBO_ZXPlane, NUM_QUAD_INDICES * sizeof(uint16_t), quadIndices, GL_STATIC_DRAW);

    GLuint VAO_ZXPlane;
    glCreateVertexArrays(1, &VAO_ZXPlane);
    glVertexArrayVertexBuffer(VAO_ZXPlane, 0, VBO_ZXPlane, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(VAO_ZXPlane, EBO_ZXPlane);
    /* Position */
    glVertexArrayAttribFormat(VAO_ZXPlane, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_ZXPlane, 0);
    glVertexArrayAttribBinding(VAO_ZXPlane, 0, 0);
    /* Normal */
    glVertexArrayAttribFormat(VAO_ZXPlane, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_ZXPlane, 1);
    glVertexArrayAttribBinding(VAO_ZXPlane, 1, 0);
    /* Color */
    glVertexArrayAttribFormat(VAO_ZXPlane, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_ZXPlane, 2);
    glVertexArrayAttribBinding(VAO_ZXPlane, 2, 0);
    /* UV */
    glVertexArrayAttribFormat(VAO_ZXPlane, 3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_ZXPlane, 3);
    glVertexArrayAttribBinding(VAO_ZXPlane, 3, 0);

    Image image_ZXPlane{};
    if ( !image_ZXPlane.Load("textures/sand_diffuse.jpg") )
    {
        fprintf(stderr, "Could not load texture file sand_diffuse.jpg.\n");
    }

    GLuint textureHandleZXPlane;
    glCreateTextures(GL_TEXTURE_2D, 1, &textureHandleZXPlane);
    glTextureParameteri(textureHandleZXPlane, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureHandleZXPlane, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureHandleZXPlane, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(textureHandleZXPlane, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureStorage2D(textureHandleZXPlane, 1, GL_RGBA8, image_ZXPlane.GetWidth(), image_ZXPlane.GetHeight());
    glTextureSubImage2D(
        textureHandleZXPlane, 0, 0, 0, image_ZXPlane.GetWidth(), image_ZXPlane.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, image_ZXPlane.Data()
    );

    // TODO: Show shadow acne.
    std::vector<Vertex> sphereVertices = CreateSphere(Vec3f{1.0f, 1.0f, 1.0f});
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

    /* Some global GL states */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    // glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* Debug toggles */
    bool useDepthMask    = true;
    bool useSkyboxViewMat = true;
    bool useShadow = true;
    bool useFloorTexture = true;
    bool useBias = false;
    float biasAmount = 0.0001f;

    /* Main loop */
    bool isRunning = true;
    SDL_GL_SetSwapInterval(1); /* 1 = VSync enabled; 0 = VSync disabled */
    Uint64 ticksPerSecond = SDL_GetPerformanceFrequency();
    Uint64 startCounter   = SDL_GetPerformanceCounter();
    Uint64 endCounter     = SDL_GetPerformanceCounter();
    while ( isRunning )
    {
        double ticksPerFrame = (double)endCounter - (double)startCounter;
        double msPerFrame    = (ticksPerFrame / (double)ticksPerSecond) * 1000.0;
        startCounter         = SDL_GetPerformanceCounter();

        SDL_Event e;
        while ( SDL_PollEvent(&e) )
        {
            ImGui_ImplSDL3_ProcessEvent(&e);
            pRamen->ProcessInputEvent(e);

            if ( e.type == SDL_EVENT_QUIT )
            {
                isRunning = false;
            }

            if ( e.type == SDL_EVENT_KEY_DOWN )
            {
                switch ( e.key.key )
                {
                case SDLK_ESCAPE:
                {
                    isRunning = false;
                }
                break;

                default:
                {
                }
                }
            }
        }

        // TODO: Aufgabe 5.1.3) Kamera
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
                camera.RotateAroundWorldUp(rotateStep);
            else if ( pRamen->KeyWentDown(SDLK_RIGHT) || pRamen->KeyPressed(SDLK_RIGHT) )
                /* TODO: Yaw right camera */
                camera.RotateAroundWorldUp(-rotateStep);
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

        /* Light Camera mats */
        Mat4f lightViewMat = Mat4f::Identity(); // Will be changed further down.

        Mat4f lightProjMat = PerspectiveProjection(TO_RAD(60.0f), 1.0f, 0.01f, 500.0f);
            // 1.0f = aspect = (float)SHADOW_MAP_SIZE / (float)SHADOW_MAP_SIZE; 

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Cubemap settings");

        ImGui::Checkbox("Skybox im Hintergrund (glDepthMask(GL_FALSE))", &useDepthMask);
        ImGui::Checkbox("Skybox fest an Kamera (Skybox View Matrix - Translation entfernen)", &useSkyboxViewMat);
        ImGui::Checkbox("Aktiviere Schatten", &useShadow);
        ImGui::Checkbox("Aktiviere Textur für Boden", &useFloorTexture);
        ImGui::Checkbox("Aktiviere Bias (verhindert Shadow Acne)", &useBias);
        if (useBias)
            ImGui::DragFloat("Bias-Wert (0.0001=gut, >0.001=Peter Panning)", &biasAmount, 0.000001f, 0.0f, 0.01f, "%.6f");
        ImGui::DragFloat3("Lichtposition", lightPos.Data(), 0.1f);

        ImGui::End();

        /* ImGUI Rendering */
        ImGui::Render();

        /* Rendering */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

        // TODO: Aufgabe 5.2) Environment Mapping
        cameraPosition = camera.GetPosition();

        // TODO: Aufgabe 6.4) Lichtquelle beweglich machen
        lightCamera.Position() = lightPos;       // Position aktualisieren
        lightCamera.Orient(Vec3f{0, 0, 0});      // wieder Richtung Ursprung schauen
        lightViewMat = LookAt(
            lightCamera.GetPosition(),
            lightCamera.GetPosition() + lightCamera.GetForward(),
            lightCamera.GetUp()
        );

        if (useShadow) {
            // Shadowmap. - Erster Durchlauf (Shadow Pass): Szene aus Sicht der Lichtquelle rendern
            shadowMapShader.Use();
            glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE); // SHADOW_MAP_SIZE Welche größe?
            glNamedFramebufferDrawBuffer(FBO_Shadowmap, GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO_Shadowmap);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUniformMatrix4fv(0, 1, GL_FALSE, modelMat.Data());
            glUniformMatrix4fv(1, 1, GL_FALSE, lightViewMat.Data());
            glUniformMatrix4fv(2, 1, GL_FALSE, lightProjMat.Data());
            // Alle Hauptmodelle rendern (z.B. Skull)
            glBindVertexArray(VAO_Model);
            glBindTextureUnit(0, textureHandleCubemap);
            glDrawArrays(GL_TRIANGLES, 0, model.NumVertices());

            Mat4f sphereModelMat = modelMat * Translate(Vec3f{-3.0f, 0.0f, 0.0f});
            glUniformMatrix4fv(0, 1, GL_FALSE, sphereModelMat.Data());
            glBindVertexArray(VAO_Sphere);
            glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size());
        }
        // Reset für zweiten Durchlauf (Hauptpass): Szene aus Sicht der Kamera rendern
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // zurück zum default framebuffer.
        glViewport(0, 0, windowWidth, windowHeight); // Default-Framebuffer wieder auf volle Fenstergroesse setzen.
        

        // Lichtquelle in shader packen
        glUniform3fv(3, 1, lightPos.Data()); // als uniform in fragment-shader an pos3 packen

        // Skybox.
        if (useDepthMask)
            // Skybox immer in den Hintergrund rendern, da nicht im Depth Buffer - also wird immer überschrieben.
            glDepthMask(GL_FALSE);

        // Entfernt die Translation der Skybox und bleibt damit an der Kamera kleben
            // Rotation funktioniert weiterhin
        Mat4f skyboxViewMat = viewMat;
        if (useSkyboxViewMat)
        {
            skyboxViewMat[3][0] = 0.0f;
            skyboxViewMat[3][1] = 0.0f;
            skyboxViewMat[3][2] = 0.0f;
        }
        
        skyboxShader.Use();
        glUniformMatrix4fv(0, 1, GL_FALSE, modelMat.Data());
        glUniformMatrix4fv(1, 1, GL_FALSE, skyboxViewMat.Data());
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());
        glUniformMatrix4fv(3, 1, GL_FALSE, lightViewMat.Data());
        glUniformMatrix4fv(4, 1, GL_FALSE, lightProjMat.Data());
        
        glBindVertexArray(VAO_SkyboxCube);
        glBindTextureUnit(0, textureHandleCubemap);
        glDrawArrays(GL_TRIANGLES, 0, skyboxVertices.size());

        glDepthMask(GL_TRUE);

        // ZX-Plane als Schattenempfänger.
        floorShader.Use();
        Mat4f floorModelMat = modelMat * Translate(Vec3f{0.0f, -1.0f, 0.0f}) * Scale(Vec3f{10.0f, 1.0f, 10.0f});
        glUniformMatrix4fv(0, 1, GL_FALSE, floorModelMat.Data());
        glUniformMatrix4fv(1, 1, GL_FALSE, viewMat.Data());
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());
        glUniformMatrix4fv(3, 1, GL_FALSE, lightViewMat.Data());
        glUniformMatrix4fv(4, 1, GL_FALSE, lightProjMat.Data());
        glBindVertexArray(VAO_ZXPlane);
        glBindTextureUnit(0, textureHandleZXPlane);
        glBindTextureUnit(1, textureHandleShadowMap);
        glUniform1i(5, useShadow ? 1 : 0); // Setzt bool in fragment-shader
        glUniform1i(6, useFloorTexture ? 1 : 0); // Setzt bool in fragment-shader
        glUniform1f(7, useBias ? biasAmount : 0.0f); // Setzt bias-Wert in fragment-shader
        glDrawElementsBaseVertex(GL_TRIANGLES, NUM_QUAD_INDICES, GL_UNSIGNED_SHORT, 0, 0);

        // Kugel: weiß + Shadow Acne sichtbar (kein Bias in floor.frag)
        floorShader.Use();
        Mat4f sphereModelMat = modelMat * Translate(Vec3f{-3.0f, 0.0f, 0.0f});
        glUniformMatrix4fv(0, 1, GL_FALSE, sphereModelMat.Data());
        glUniformMatrix4fv(1, 1, GL_FALSE, viewMat.Data());
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());
        glUniformMatrix4fv(3, 1, GL_FALSE, lightViewMat.Data());
        glUniformMatrix4fv(4, 1, GL_FALSE, lightProjMat.Data());
        glBindVertexArray(VAO_Sphere);
        glBindTextureUnit(1, textureHandleShadowMap);  // binding 1 = Shadow Map
        glUniform1i(5, useShadow ? 1 : 0);  // u_UseShadow
        glUniform1i(6, 0);                   // u_UseTexture = false → weiß
        glUniform1f(7, useBias ? biasAmount : 0.0f); // u_BiasAmount
        glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size());

        flatShader.Use();
        // Sphere um Lichtquelle zu visualisieren.
        Mat4f lightSphereModelMat = modelMat * Translate(lightPos) * Scale(Vec3f{0.2f, 0.2f, 0.2f});
        glUniformMatrix4fv(0, 1, GL_FALSE, lightSphereModelMat.Data());
        glUniformMatrix4fv(1, 1, GL_FALSE, viewMat.Data());
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());
        glBindVertexArray(VAO_LightSphere);
        glDrawArrays(GL_TRIANGLES, 0, lightSphereVertices.size());

        // TODO: Aufgabe 5.3) Environment Mapping
        envmapShader.Use();
        glUniformMatrix4fv(0, 1, GL_FALSE, modelMat.Data());
        glUniformMatrix4fv(1, 1, GL_FALSE, viewMat.Data());
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());
        glUniform3fv(3, 1, cameraPosition.Data());
        glBindVertexArray(VAO_Model);
        glBindTextureUnit(0, textureHandleCubemap);
        glDrawArrays(GL_TRIANGLES, 0, model.NumVertices());

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(pRamen->GetWindow());

        endCounter = SDL_GetPerformanceCounter();
    }

    /* GL Resources shutdown. */
    skyboxShader.Delete();
    floorShader.Delete();
    flatShader.Delete();
    envmapShader.Delete();
    glDeleteVertexArrays(1, &VAO_SkyboxCube);
    glDeleteVertexArrays(1, &VAO_LightSphere);
    glDeleteVertexArrays(1, &VAO_Sphere);
    glDeleteVertexArrays(1, &VAO_Model);
    glDeleteVertexArrays(1, &VAO_ZXPlane);
    glDeleteTextures(1, &textureHandleZXPlane);
    glDeleteTextures(1, &textureHandleShadowMap);
    glDeleteTextures(1, &textureHandleCubemap);

    /* Ramen Shutdown */
    pRamen->Shutdown();

    /* Filesystem deinit */
    PHYSFS_deinit();

    return 0;
}
