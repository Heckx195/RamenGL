#include <glad/glad.h>

#include <assert.h>
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
        // Dreieck 1: a, b, c
        vertices.push_back({a, n, color, uva});
        vertices.push_back({b, n, color, uvb});
        vertices.push_back({c, n, color, uvc});
        // Dreieck 2: a, c, d
        vertices.push_back({a, n, color, uva});
        vertices.push_back({c, n, color, uvc});
        vertices.push_back({d, n, color, uvd});
    };

    // Immer das ccw vom inneren der Würfeloberfläche sichtbare Dreiecksnetz definieren
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

std::vector<Vertex> CreateCylinder(const Vec3f& color)
{
    // placeholder.
    return std::vector<Vertex>{};
}

std::vector<Vertex> CreateSphere(const Vec3f& color)
{
    // placeholder.
    return std::vector<Vertex>{};
}

int main(int argc, char** argv)
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

    // TODO: Aufgabe 3.2) Erzeugung von Zylinder, Quader, Kugeln
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
    while ( isRunning )
    {

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

        glDrawArrays(GL_LINES, 0, 6);
        // GL_Lines = Zeichenmodus; jedes Paar von Vertices wird als Linie interpretiert. (6 Vertices = 3 Linien für die Koordinatenachsen)

        // Draw the cube.
        glBindVertexArray(VAO_Cube);
        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(pRamen->GetWindow());
    }

    /* GL Resources shutdown. */
    shader.Delete();
    glDeleteBuffers(1, &VBO_Coord);
    glDeleteVertexArrays(1, &VAO_Coord);

    glDeleteBuffers(1, &VBO_Cube);
        // 1=Anzahl der zu löschenden Vertex Buffer Objects, &VBO_Cube=Adresse des zu löschenden Vertex Buffer Objects
    glDeleteVertexArrays(1, &VAO_Cube);

    /* Ramen Shutdown */
    pRamen->Shutdown();

    /* Filesystem deinit */
    PHYSFS_deinit();

    return 0;
}
