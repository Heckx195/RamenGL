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
            Vec3f{0,0,1}, // Normale zeigt nach innen (zum Ursprung)
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Back face (z = +1)
    pushQuad(corners[4], corners[7], corners[6], corners[5],
            Vec3f{0,0,-1},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Left face (x = -1)
    pushQuad(corners[0], corners[3], corners[7], corners[4],
            Vec3f{1,0,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Right face (x = +1)
    pushQuad(corners[1], corners[5], corners[6], corners[2],
            Vec3f{-1,0,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Bottom face (y = -1)
    pushQuad(corners[0], corners[4], corners[5], corners[1],
            Vec3f{0,1,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});

    // Top face (y = +1)
    pushQuad(corners[3], corners[2], corners[6], corners[7],
            Vec3f{0,-1,0},
            Vec3f{0,0,0}, Vec3f{1,0,0}, Vec3f{1,1,0}, Vec3f{0,1,0});
    
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

int main(int argc, char** argv)
{
    Filesystem* pFS = Filesystem::Init(argc, argv, "../../assets");

    Ramen* pRamen = Ramen::Instance();
    pRamen->Init("Task 05 - Cubemapping", 800, 600);

    /* Load shaders. */
    Shader shader{};
    if ( !shader.Load("shaders/task05.vert", "shaders/task05.frag") )
    {
        fprintf(stderr, "Could not load shader.\n");
    }

    // TODO: Aufgabe 5.2) Environment Mapping
    Shader envmapShader{};
    if ( !envmapShader.Load("shaders/envmap.vert", "shaders/envmap.frag") )
    {
        fprintf(stderr, "Could not load environment map shader.\n");
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
    Camera camera(Vec3f{ 0.0f, 0.0f, 0.0f }); // inside cube.
    camera.RotateAroundSide(0.0f);
    Vec3f cameraPosition = Vec3f{ 0.0f, 0.0f, 0.0f };

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

    std::vector<Vertex> cubeNormals     = CreateNormalLines(skyboxVertices);
    GLuint VBO_CubeNormals, VAO_CubeNormals;
    makeNormalVAO(VBO_CubeNormals, VAO_CubeNormals, cubeNormals);

    // TODO: Aufgabe 5.1.1
    GLuint textureHandleCubemap;
    std::string prefix = "textures/cubemaps/Colosseum/";
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

        // TODO: Aufgabe 5.2) Environment Mapping
        cameraPosition = camera.GetPosition();

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

        ImGui::Begin("Cubemap settings");

        ImGui::Checkbox("glDepthMask(GL_FALSE) fuer Skybox (True: Skybox immer im Hintergrund)", &useDepthMask);
        ImGui::Checkbox("Skybox View Matrix (Translation entfernen)", &useSkyboxViewMat);

        ImGui::End();

        /* ImGUI Rendering */
        ImGui::Render();

        /* Rendering */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

        if (useDepthMask)
            glDepthMask(GL_FALSE);

        Mat4f skyboxViewMat = viewMat;
        if (useSkyboxViewMat)
        {
            skyboxViewMat[3][0] = 0.0f;
            skyboxViewMat[3][1] = 0.0f;
            skyboxViewMat[3][2] = 0.0f;
        }
        
        shader.Use();
        glUniformMatrix4fv(0, 1, GL_FALSE, modelMat.Data());
        glUniformMatrix4fv(1, 1, GL_FALSE, skyboxViewMat.Data());
        glUniformMatrix4fv(2, 1, GL_FALSE, projMat.Data());
        
        glBindVertexArray(VAO_SkyboxCube);
        glBindTextureUnit(0, textureHandleCubemap);
        glDrawArrays(GL_TRIANGLES, 0, skyboxVertices.size());

        if (useDepthMask)
            glDepthMask(GL_TRUE);
        glBindVertexArray(VAO_CubeNormals);
        glDrawArrays(GL_LINES, 0, (GLsizei)cubeNormals.size());

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
    shader.Delete();
    glDeleteVertexArrays(1, &VAO_SkyboxCube);
    glDeleteVertexArrays(1, &VAO_CubeNormals);
    glDeleteTextures(1, &textureHandleCubemap);

    /* Ramen Shutdown */
    pRamen->Shutdown();

    /* Filesystem deinit */
    PHYSFS_deinit();

    return 0;
}
