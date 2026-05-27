#include <glad/glad.h>

#include <assert.h>
#include <cmath>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
#include "../ramen/rgl_model.h"
#include "../ramen/rgl_shader.h"

#define NUM_QUAD_VERTICES 4
#define NUM_QUAD_INDICES 6

// TODO: Aufgabe 4.1) Texturkoordinaten zu Quad
// Auf Kopf die Textur (0,0 unten links)
// Gleiche Normale weil gleiche Ebene
// static Vertex quadVertices[ NUM_QUAD_VERTICES ] = {
//     // Pos, Normal, Color, UV
//     { Vec3f{ -1.0f, -1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 0.0f } },
//     { Vec3f{ 1.0f, -1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 1.0f, 0.0f, 0.0f } },
//     { Vec3f{ 1.0f, 1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 0.0f } },
//     { Vec3f{ -1.0f, 1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 0.0f, 1.0f, 0.0f } }
// };
// Textur korrekt herum (0,0 oben links)
static Vertex quadVertices[ NUM_QUAD_VERTICES ] = {
    // Pos, Normal, Color, UV
    { Vec3f{ -1.0f, -1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 0.0f, 1.0f, 0.0f } },
    { Vec3f{ 1.0f, -1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 0.0f } },
    { Vec3f{ 1.0f, 1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 1.0f, 0.0f, 0.0f } },
    { Vec3f{ -1.0f, 1.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 1.0f }, Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 0.0f } }
};
static uint16_t quadIndices[ NUM_QUAD_INDICES ]   = { 0, 1, 2, 2, 3, 0 };

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

int main(int argc, char** argv)
{
    Filesystem* pFS = Filesystem::Init(argc, argv, "../../assets");

    Ramen* pRamen = Ramen::Instance();
    pRamen->Init("Model loading", 800, 600);

    /* Load shaders. */
    Shader shader{};
    if ( !shader.Load("shaders/textures.vert", "shaders/textures.frag") )
    {
        fprintf(stderr, "Could not load shader.\n");
    }

    Image image{};
    if ( !image.Load("textures/linux-quake-512x512.png") )
    {
        fprintf(stderr, "Could not load texture file.\n");
    }

    Image imageEarth{};
    if ( !imageEarth.Load("textures/worldmap.jpg") )
    {
        fprintf(stderr, "Could not load texture file (Worldmap).\n");
    }

    /* Create GPU buffer and upload model's vertices. */
    GLuint VBO;
    glCreateBuffers(1, &VBO);
    glNamedBufferData(VBO, NUM_QUAD_VERTICES * sizeof(Vertex), quadVertices, GL_STATIC_DRAW);
    GLuint EBO; // Ermöglicht Wiederverwendung von Vertexdaten durch Indizierung (Index Buffer Object)
    glCreateBuffers(1, &EBO);
    glNamedBufferData(EBO, NUM_QUAD_INDICES * sizeof(uint16_t), quadIndices, GL_STATIC_DRAW);

    /* Create camera */
    Camera camera(Vec3f{ 0.0f, 0.0f, 5.0f });
    camera.RotateAroundSide(0.0f);

    /* Model mat*/
    Mat4f modelMat = Mat4f::Identity();

    /* VAO. */
    GLuint VAO_Quad;
    glCreateVertexArrays(1, &VAO_Quad);
    glVertexArrayVertexBuffer(VAO_Quad, 0, VBO, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(VAO_Quad, EBO);
    /* Position */
    glVertexArrayAttribFormat(VAO_Quad, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(VAO_Quad, 0);
    glVertexArrayAttribBinding(VAO_Quad, 0, 0);
    /* Normal */
    glVertexArrayAttribFormat(VAO_Quad, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Quad, 1);
    glVertexArrayAttribBinding(VAO_Quad, 1, 0);
    /* Color */
    glVertexArrayAttribFormat(VAO_Quad, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Quad, 2);
    glVertexArrayAttribBinding(VAO_Quad, 2, 0);
    /* UV */
    glVertexArrayAttribFormat(VAO_Quad, 3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
    glEnableVertexArrayAttrib(VAO_Quad, 3);
    glVertexArrayAttribBinding(VAO_Quad, 3, 0);

    // TODO: Aufgabe 4.2) OpenGL Textur auf GPU erstellen
    GLuint textureHandle;
    glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);
    glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureStorage2D(textureHandle, 1, GL_RGBA8, image.GetWidth(), image.GetHeight());
    glTextureSubImage2D(
        textureHandle, 0, 0, 0, image.GetWidth(), image.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, image.Data()
    );

    // TODO: Aufgabe 4.5) Texturkoordinaten für Kugel erstellen
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

    GLuint textureHandleEarth;
    glCreateTextures(GL_TEXTURE_2D, 1, &textureHandleEarth);
    glTextureParameteri(textureHandleEarth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureHandleEarth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureHandleEarth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(textureHandleEarth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureStorage2D(textureHandleEarth, 1, GL_RGBA8, imageEarth.GetWidth(), imageEarth.GetHeight());
    glTextureSubImage2D(
        textureHandleEarth, 0, 0, 0, imageEarth.GetWidth(), imageEarth.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, imageEarth.Data()
    );

    /* Some global GL states */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    // glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Aufgabe 4.6) Rotation der Erde um eigene Achse
    float earthCameraAngleX = 0.0f;
    float earthCameraAngleY = 0.0f;

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
                        break;
                    }
                    default:
                    {
                    }
                }
            }
        }

        /* Camera movement - Rotation of earth; camera stays still */
        // Rotate earth around X-Axis.
        if ( pRamen->KeyWentDown(SDLK_UP) || pRamen->KeyPressed(SDLK_UP) ) {
            earthCameraAngleX += 50.0f * (float)msPerFrame / 1000.0f; // 50 degrees per second
        }
        else if ( pRamen->KeyWentDown(SDLK_DOWN) || pRamen->KeyPressed(SDLK_DOWN) ) {
            earthCameraAngleX -= 50.0f * (float)msPerFrame / 1000.0f; // 50 degrees per second
        }

        // Rotate earth around Y-Axis.
        if ( pRamen->KeyWentDown(SDLK_LEFT) || pRamen->KeyPressed(SDLK_LEFT) ) {
            earthCameraAngleY += 50.0f * (float)msPerFrame / 1000.0f; // 50 degrees per second
        }
        else if ( pRamen->KeyWentDown(SDLK_RIGHT) || pRamen->KeyPressed(SDLK_RIGHT) ) {
            earthCameraAngleY -= 50.0f * (float)msPerFrame / 1000.0f; // 50 degrees per second
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

        /* Rendering */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

        shader.Use();
        glBindVertexArray(VAO_Quad); // Packe unteranderem UV in den Vertex-Shader und Fragment-Shader.
        glUniformMatrix4fv(0, 1, GL_FALSE, modelMat.Data());
        glUniformMatrix4fv(1, 1, GL_FALSE, viewMat.Data());
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());
        // TODO: Aufgabe 4.4)
        // Bindet Texture-Handler an Texture Unit0. Im Shader über uniform wird darauf zugegriffen.
        glBindTextureUnit(0, textureHandle);
        // Zeichnet basierend auf dem EBO die Geometry. Oben wurde EBO an VAO_Quad gepackt.
        // TODO: IMPORTANT! Entkommentieren fürs Ausführen vom Linux Quad.                          !!!!!!
        // glDrawElementsBaseVertex(GL_TRIANGLES, NUM_QUAD_INDICES, GL_UNSIGNED_SHORT, 0, 0);

        // TODO: Aufgabe 4.5)
        Mat4f baseScale = Scale(Vec3f{ 1.0f, 1.0f, 1.0f });

        // Reihenfolge für Rotation wichtig! Erst Welt-Y-Achse dann Welt-X-Achse
        Mat4f earthModelMat = modelMat
            * Translate(Vec3f{ 0.0f, 0.0f, 0.0f })
            * baseScale
            * Rotate(RAMEN_WORLD_RIGHT, earthCameraAngleX)
            * Rotate(RAMEN_WORLD_UP, earthCameraAngleY)
        ;

        glUniformMatrix4fv(0, 1, GL_FALSE, earthModelMat.Data());
        glBindVertexArray(VAO_Earth);
        glBindTextureUnit(0, textureHandleEarth);
        glDrawArrays(GL_TRIANGLES, 0, earthVertices.size());


        SDL_GL_SwapWindow(pRamen->GetWindow());

        endCounter = SDL_GetPerformanceCounter();
    }

    /* GL Resources shutdown. */
    shader.Delete();
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO_Quad);

    /* Ramen Shutdown */
    pRamen->Shutdown();

    /* Filesystem deinit */
    PHYSFS_deinit();

    return 0;
}
