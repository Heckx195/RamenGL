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

#include <xatlas/xatlas.h>

#include "../ramen/ramen.h"
#include "../ramen/rgl_camera.h"
#include "../ramen/rgl_defines.h"
#include "../ramen/rgl_filesystem.h"
#include "../ramen/rgl_image.h"
#include "../ramen/rgl_math.h"
#include "../ramen/rgl_model.h"
#include "../ramen/rgl_shader.h"

#define DOLLY_SPEED 10.0f
#define LIGHT_MAP_SIZE 1024
#define NUM_PATCHES (LIGHT_MAP_SIZE * LIGHT_MAP_SIZE)

uint32_t Vec3f_to_32ABGR(const Vec3f& v3)
{
    uint32_t r = (uint32_t)(v3.x * 255.0f);
    uint32_t g = (uint32_t)(v3.y * 255.0f);
    uint32_t b = (uint32_t)(v3.z * 255.0f);
    uint32_t a = 255;
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

Vec3f gui_hsv_to_rgb(Vec3f hsv)
{
    float hf    = hsv.x >= 2.0f * RAMEN_PI ? (hsv.x - 2.0f * RAMEN_PI) : hsv.x;
    hf          = hf <= 0.0f ? (2.0f * RAMEN_PI + hf) : hf;
    float h_deg = (hf * 180.0f) / RAMEN_PI;
    int   h     = h_deg / 60.0f;
    float f     = (h_deg / 60.0f) - (int)h;
    float p     = hsv.z * (1.0f - hsv.y);
    float q     = hsv.z * (1.0f - hsv.y * f);
    float t     = hsv.z * (1.0f - hsv.y * (1.0f - f));

    switch ( h )
    {

    case 0:
    {
        return Vec3f{ hsv.z, t, p };
    }
    break;
    case 1:
    {
        return Vec3f{ q, hsv.z, p };
    }
    break;
    case 2:
    {
        return Vec3f{ p, hsv.z, t };
    }
    break;
    case 3:
    {
        return Vec3f{ p, q, hsv.z };
    }
    break;
    case 4:
    {
        return Vec3f{ t, p, hsv.z };
    }
    break;
    case 5:
    {
        return Vec3f{ hsv.z, p, q };
    }
    break;
    case 6:
    {
        return Vec3f{ hsv.z, t, p };
    }
    break;
    default:
    {
        return Vec3f{ 0.f, 0.f, 0.f };
    }
    }
}

struct Patch
{
    // Vec4f st;
    Vec4f worldpos;
    Vec4f normal;
    Vec4f radiance;
    Vec4f debugColor;
    Vec4f area;
};

struct BBox2D
{
    Vec2f mins;
    Vec2f maxs;
    Vec2f dimension;
};

struct Triangle
{
    uint32_t a, b, c, dummy;
};

struct LevelMesh
{
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;

    std::vector<Vertex>& GetVertices()
    {
        return vertices;
    }

    std::vector<uint16_t>& GetIndices()
    {
        return indices;
    }

    size_t NumVertices()
    {
        return vertices.size();
    }

    size_t NumIndices()
    {
        return indices.size();
    }
};

BBox2D CreateBoundingBox2D(const Vec3f& a, const Vec3f& b, const Vec3f& c)
{
    BBox2D bbox{};
    bbox.mins.x      = std::min(std::min(a.x, b.x), c.x);
    bbox.mins.y      = std::min(std::min(a.y, b.y), c.y);
    bbox.maxs.x      = std::max(std::max(a.x, b.x), c.x);
    bbox.maxs.y      = std::max(std::max(a.y, b.y), c.y);
    bbox.dimension.x = abs(bbox.maxs.x - bbox.mins.x);
    bbox.dimension.y = abs(bbox.maxs.y - bbox.mins.y);

    return bbox;
}

BBox2D ScaleBoundingBox2D(const BBox2D& bbox2d, float scale)
{
    BBox2D scaledBB = bbox2d;
    scaledBB.mins *= scale;
    scaledBB.maxs *= scale;
    scaledBB.dimension *= scale;
    return scaledBB;
}

bool IsUV_OnTri(const Vec2f& uv, const Vertex& v0, const Vertex& v1, const Vertex& v2, Patch* outPatch)
{
    float area = Cross(Vec2f{ v1.uv - v0.uv }, Vec2f{ v2.uv - v0.uv });

    if ( fabsf(area) < 1e-20f ) return false;

    float invArea = 1.0f / area;

    float w0 = Cross(Vec2f{ v1.uv } - uv, Vec2f{ v2.uv } - uv) * invArea;
    float w1 = Cross(Vec2f{ v2.uv } - uv, Vec2f{ v0.uv } - uv) * invArea;
    float w2 = 1.0f - w0 - w1; //Cross(Vec2f{ v0.uv } - uv, Vec2f{ v1.uv } - uv) * invArea;
        // positiver Wert = Punkt liegt links von Strecke v0-v1
        // negativer Wert = Punkt liegt rechts von Strecke v0-v1

    const float eps = -1e-6f;

    bool inside = w0 >= eps && w1 >= eps && w2 >= eps; // liegt der Punkt uv innerhalb des Dreiecks?

    if ( inside )
    {
        Patch patch{};
        patch.worldpos = Vec4f{ w0 * v0.position + w1 * v1.position + w2 * v2.position, 1.0f };
        patch.normal   = Vec4f{ Normalize(w0 * v0.normal + w1 * v1.normal + w2 * v2.normal), 0.0f };
        *outPatch      = patch;
    }

    return inside;
}

struct ComputeShaderVertex
{
    Vec4f position;
    Vec4f normal;
    Vec4f color;
    Vec4f uv;
    Vec4f tangent;
    Vec4f bitangent;
};

int main(int argc, char** argv)
{
    Filesystem* pFS = Filesystem::Init(argc, argv, "assets");

    Ramen* pRamen = Ramen::Instance();
    pRamen->Init("Computeshader", 800, 600);

    /* Load shaders. */
    Shader modelShader{};
    if ( !modelShader.Load("shaders/computeshader.vert", "shaders/computeshader.frag") )
    {
        fprintf(stderr, "Could not load vert/frag shaders\n");
        exit(1);
    }

    Shader computeShader{};
    if ( !computeShader.Load("shaders/computeshader.comp", GL_COMPUTE_SHADER) )
    {
        fprintf(stderr, "Could not load compute shader.\n");
        exit(1);
    }

    /* Load model */
    Model model{};
    if ( !model.Load("models/cornell.obj", true) )
    {
        fprintf(stderr, "Could not load model file.\n");
        exit(1);
    }

    /* Create lightmap atlas */
    xatlas::Atlas*   atlas = xatlas::Create();
    xatlas::MeshDecl sceneDecl{};
    sceneDecl.vertexCount              = model.NumVertices();
    sceneDecl.vertexPositionData       = model.GetVertices().data();
    sceneDecl.vertexPositionStride     = sizeof(Vertex);
    sceneDecl.indexData                = model.GetIndices().data();
    sceneDecl.indexCount               = model.NumIndices();
    xatlas::AddMeshError addMeshResult = xatlas::AddMesh(atlas, sceneDecl);
    LevelMesh            levelMesh{};      /* New mesh with xatlas UVs */
    Image                xatlasUV_Image{}; /* Debug lightmap UV texture from xatlas */
    GLuint               xatlasUV_Texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &xatlasUV_Texture);
    if ( xatlas::AddMeshError::Success == addMeshResult )
    {
        xatlas::ChartOptions chartOptions{};
        chartOptions.fixWinding = true;
        xatlas::PackOptions packOptions{};
        packOptions.resolution  = LIGHT_MAP_SIZE;
        packOptions.padding     = 0;
        packOptions.bruteForce  = false;
        packOptions.createImage = true;
        xatlas::Generate(atlas, chartOptions, packOptions);
        printf("Generated texture atlas for scene geometry successfully.\n");
        printf("XAtlas Info:\n");
        printf("  width / height: %d / %d\n", atlas->width, atlas->height);
        printf("  meshCount: %d\n", atlas->meshCount);

        /* Create the debug texture of generated UV map */
        assert(atlas->image && "Atlas image is NULL! This must not happen.");
        xatlasUV_Image.Init(atlas->width, atlas->height, 4); /* xatlas images are 4byte/pixel. */
        memcpy(xatlasUV_Image.Data(), atlas->image, atlas->width * atlas->height * sizeof(uint32_t));
        glTextureParameteri(xatlasUV_Texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(xatlasUV_Texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(xatlasUV_Texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(xatlasUV_Texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(xatlasUV_Texture, 1, GL_RGBA8, xatlasUV_Image.GetWidth(), xatlasUV_Image.GetHeight());
        glTextureSubImage2D(xatlasUV_Texture,
                            0,
                            0,
                            0,
                            xatlasUV_Image.GetWidth(),
                            xatlasUV_Image.GetHeight(),
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            xatlasUV_Image.Data());

        /* We should have exactly *one* mesh as we
         * called xatlas::AddMesh() *once*.
         */
        assert(atlas->meshCount == 1 && "atlas produces more/less meshes than 1. That must not happen!");
        xatlas::Mesh mesh = atlas->meshes[ 0 ];

        levelMesh.indices.resize(mesh.indexCount);
        levelMesh.vertices.resize(mesh.vertexCount);

        for ( size_t j = 0; j < mesh.indexCount; j++ )
        {

            levelMesh.indices[ j ] = (uint16_t)(mesh.indexArray[ j ]);
        }

        for ( size_t j = 0; j < levelMesh.indices.size(); j++ )
        {
            uint16_t levelMeshIdx = levelMesh.indices[ j ]; /* new index */

            xatlas::Vertex v = mesh.vertexArray[ levelMeshIdx ];

            uint32_t origMeshIdx = v.xref;
            Vertex   originalV   = model.GetVertices()[ origMeshIdx ];
            originalV.uv         = Vec3f{ v.uv[ 0 ], v.uv[ 1 ], 0.0f };
            originalV.uv.x /= (float)atlas->width;
            originalV.uv.y /= (float)atlas->height;
            levelMesh.vertices[ levelMeshIdx ] = originalV;
        }
        // TODO: Part of xatlas::Mesh. What do they do?
        // Chart *chartArray;
        // uint32_t chartCount;
    }
    else
    {
        fprintf(stderr, "Could not create UV texture atlas for lightmap :(\n");
        exit(1);
    }

    /* Create patches */
    printf("...................\n");
    printf("Create Patches...\n");
    printf("...................\n");

    std::vector<Patch> patches{};
    Image              patchDebugImage{};
    patchDebugImage.Init(LIGHT_MAP_SIZE, LIGHT_MAP_SIZE, 4);
    patches.resize(NUM_PATCHES);
    memset(patches.data(), 0, patches.size() * sizeof(Patch)); // NOTE: I am a bit paranoid.
    const std::vector<uint16_t>& levelGeomIndices  = levelMesh.GetIndices();
    const std::vector<Vertex>&   levelGeomVertices = levelMesh.GetVertices();

    /* Create individual colors for each triangle for debug purposes */
    size_t             numLevelGeomTris = levelGeomIndices.size() / 3;
    float              hsvAngleStep     = (2.0f * RAMEN_PI) / (float)numLevelGeomTris;
    float              redStep          = 1.0f / (float)numLevelGeomTris;
    std::vector<Vec3f> levelGeomTriColors{};
    levelGeomTriColors.resize(numLevelGeomTris);
    for ( int i = 0; i < numLevelGeomTris; ++i )
    {
        float currentAngle = i * hsvAngleStep;
        Vec3f hsv          = Vec3f{ currentAngle, 1.0f, 1.0f };
        Vec3f rgb          = gui_hsv_to_rgb(hsv);

        levelGeomTriColors[ i ] = Vec3f{ rgb };
    }

    /* Compute how large a texel is in the lightmap to iterate
     * through lightmap.
     */
    Vec2f lightmapTexelSize = Vec2f{ 1.0f } / (float)LIGHT_MAP_SIZE;
    /* Go through all indices of level geom, form a triangle (i+=3)
     * and find its pixel coord in lightmap (patches-array) through
     * interpolated UV coords.
     */
    for ( size_t i = 0; i < levelGeomIndices.size(); i += 3 )
    {
        /* Get triangle */
        uint16_t     i0 = levelGeomIndices[ i + 0 ];
        uint16_t     i1 = levelGeomIndices[ i + 1 ];
        uint16_t     i2 = levelGeomIndices[ i + 2 ];
        const Vertex v0 = levelGeomVertices[ i0 ];
        const Vertex v1 = levelGeomVertices[ i1 ];
        const Vertex v2 = levelGeomVertices[ i2 ];

        /* Copy of verts with normalized UVs */
        const Vec3f& levelGeomTriColor = levelGeomTriColors[ i / 3 ];
        uint32_t     color             = Vec3f_to_32ABGR(levelGeomTriColor);
        float        area              = Cross(Vec2f{ v2.uv - v0.uv }, Vec2f{ v1.uv - v0.uv });
        float        area_tri          = area / 2.0f;
        int          numPatchesCovered = (int)(area_tri * (float)(NUM_PATCHES));
        float        patchSize         = (float)numPatchesCovered / (float)(NUM_PATCHES);
        for ( size_t y = 0; y < LIGHT_MAP_SIZE; y++ )
        {
            for ( size_t x = 0; x < LIGHT_MAP_SIZE; x++ )
            {
                /* Convert pixel coord to lightmap UV coord */
                /* 0.5f = Use center of texel. */
                const Vec2f uv = { ((float)x + 0.5f) * lightmapTexelSize.x, ((float)y + 0.5f) * lightmapTexelSize.y };
                Patch       patch{};
                if ( IsUV_OnTri(uv, v0, v1, v2, &patch) )
                {
                    patchDebugImage.SetPixel32ABGR(x, y, color);
                    patch.area                        = Vec4f{ patchSize, 0.0f, 0.0f, 0.0f };
                    patch.debugColor                  = Vec4f{ levelGeomTriColor, 1.0f };
                    patches[ y * LIGHT_MAP_SIZE + x ] = patch;
                }
            }
        }
    }
    /* Make patch debug image a GL Texture so we can draw it with dear imgui. */
    GLuint patchDebugTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &patchDebugTexture);
    glTextureParameteri(patchDebugTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(patchDebugTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(patchDebugTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(patchDebugTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureStorage2D(patchDebugTexture, 1, GL_RGBA8, patchDebugImage.GetWidth(), patchDebugImage.GetHeight());
    glTextureSubImage2D(patchDebugTexture,
                        0,
                        0,
                        0,
                        patchDebugImage.GetWidth(),
                        patchDebugImage.GetHeight(),
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        patchDebugImage.Data());

    /* Create the final lightmap texture that the Compute Shader writes into.
     * We then sample from it in the fragment shader
     */
    GLuint lightmapTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &lightmapTexture);
    glTextureParameteri(lightmapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(lightmapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(lightmapTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(lightmapTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(lightmapTexture, 1, GL_RGBA32F, LIGHT_MAP_SIZE, LIGHT_MAP_SIZE);

    printf("...................\n");
    printf("Done.\n");
    printf("...................\n");

    /* Model */
    GLuint EBO;
    glCreateBuffers(1, &EBO);
    glNamedBufferData(EBO, levelMesh.NumIndices() * sizeof(uint16_t), levelMesh.GetIndices().data(), GL_STATIC_DRAW);
    GLuint levelGeometryVBO;
    glCreateBuffers(1, &levelGeometryVBO);
    glNamedBufferData(
        levelGeometryVBO, levelMesh.NumVertices() * sizeof(Vertex), levelMesh.GetVertices().data(), GL_STATIC_DRAW);

    /* Read only SSBO for Triangles */
    std::vector<Triangle> levelIndicesAsU32(levelMesh.NumIndices() / 3);
    for ( int i = 0; i < levelMesh.NumIndices() / 3; i++ )
    {
        uint32_t i0            = (uint32_t)levelMesh.GetIndices()[ i * 3 + 0 ];
        uint32_t i1            = (uint32_t)levelMesh.GetIndices()[ i * 3 + 1 ];
        uint32_t i2            = (uint32_t)levelMesh.GetIndices()[ i * 3 + 2 ];
        levelIndicesAsU32[ i ] = Triangle{ .a = i0, .b = i1, .c = i2, .dummy = 0 };
    }

    /* Read only SSBO for Vertices in CS layout */
    std::vector<ComputeShaderVertex> levelVerticesForCS(levelMesh.NumVertices());
    for ( int i = 0; i < levelMesh.NumVertices(); i++ )
    {
        const Vertex& v = levelMesh.GetVertices()[ i ];
        levelVerticesForCS[ i ]
            = ComputeShaderVertex{ Vec4f{ v.position, 1.0f }, Vec4f{ v.normal, 0.0f },  Vec4f{ v.color, 1.0f },
                                   Vec4f{ v.uv, 0.0f },       Vec4f{ v.tangent, 0.0f }, Vec4f{ v.bitangent, 0.0f } };
    }
    GLuint TriangleSSBO;
    glCreateBuffers(1, &TriangleSSBO);
    /* 0 = Upload data and never change it on host. */
    glNamedBufferStorage(TriangleSSBO, levelIndicesAsU32.size() * sizeof(Triangle), levelIndicesAsU32.data(), 0);
    /* Read only SSBO for Vertices */
    GLuint VerticesSSBO;
    glCreateBuffers(1, &VerticesSSBO);
    glNamedBufferStorage(
        VerticesSSBO, levelVerticesForCS.size() * sizeof(ComputeShaderVertex), levelVerticesForCS.data(), 0);

    /* Two SSBOs for Patch-Data. One for reading, one for writing. They get swapped after
     * each iteration, so read-buffer becomes write-buffer and vice-versa.
     */
    GLuint patchesSSBO0;
    glCreateBuffers(1, &patchesSSBO0);
    glNamedBufferStorage(patchesSSBO0, NUM_PATCHES * sizeof(Patch), patches.data(), GL_DYNAMIC_STORAGE_BIT);
    GLuint patchesSSBO1;
    glCreateBuffers(1, &patchesSSBO1);
    glNamedBufferStorage(patchesSSBO1, NUM_PATCHES * sizeof(Patch), patches.data(), GL_DYNAMIC_STORAGE_BIT);

    /* Create camera */
    Camera camera(Vec3f{ 0.0f, 10.0f, 10.0f });

    /* Model matrices */
    Mat4f modelMat = Mat4f::Identity();

    /* VAO. */
    GLuint VAO, levelGeometryVAO;
    glCreateVertexArrays(1, &VAO);
    glCreateVertexArrays(1, &levelGeometryVAO);

    glVertexArrayElementBuffer(levelGeometryVAO, EBO);
    glVertexArrayVertexBuffer(levelGeometryVAO, 1, levelGeometryVBO, 0, sizeof(Vertex));

    GLuint VAOs[ 2 ] = { VAO, levelGeometryVAO };
    int    numVaos   = sizeof(VAOs) / sizeof(*VAOs);
    for ( int i = 0; i < numVaos; i++ )
    {
        GLuint currentVAO = VAOs[ i ];
        /* Position */
        glVertexArrayAttribFormat(currentVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(currentVAO, 0);
        glVertexArrayAttribBinding(currentVAO, 0, 0);
        /* Normal */
        glVertexArrayAttribFormat(currentVAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
        glEnableVertexArrayAttrib(currentVAO, 1);
        glVertexArrayAttribBinding(currentVAO, 1, 0);
        /* Color */
        glVertexArrayAttribFormat(currentVAO, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
        glEnableVertexArrayAttrib(currentVAO, 2);
        glVertexArrayAttribBinding(currentVAO, 2, 0);
        /* UV */
        glVertexArrayAttribFormat(currentVAO, 3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
        glEnableVertexArrayAttrib(currentVAO, 3);
        glVertexArrayAttribBinding(currentVAO, 3, 0);
        /* Tangent */
        glVertexArrayAttribFormat(currentVAO, 4, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float));
        glEnableVertexArrayAttrib(currentVAO, 4);
        glVertexArrayAttribBinding(currentVAO, 4, 0);
        /* Bitangent */
        glVertexArrayAttribFormat(currentVAO, 5, 3, GL_FLOAT, GL_FALSE, 15 * sizeof(float));
        glEnableVertexArrayAttrib(currentVAO, 5);
        glVertexArrayAttribBinding(currentVAO, 5, 0);
    }

    /* Some global GL states */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* Main loop */
    bool isRunning = true;
    SDL_GL_SetSwapInterval(1); /* 1 = VSync enabled; 0 = VSync disabled */
    Uint64   ticksPerSecond      = SDL_GetPerformanceFrequency();
    Uint64   startCounter        = SDL_GetPerformanceCounter();
    Uint64   endCounter          = SDL_GetPerformanceCounter();
    GLuint   patcheSSBOs[]       = { patchesSSBO0, patchesSSBO1 };
    uint32_t frameNum            = 0;
    uint32_t numBounces          = 1;
    uint32_t numIterations       = 1;
    uint32_t iterationCounter    = 0;
    size_t   patches_ssbo_toggle = 0;
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
        if ( pRamen->KeyPressed(SDLK_DOWN) ) camera.RotateAroundSide(-100.0f * msPerFrame / 1000.0f);
        if ( pRamen->KeyPressed(SDLK_UP) ) camera.RotateAroundSide(100.0f * msPerFrame / 1000.0f);
        if ( pRamen->KeyPressed(SDLK_LEFT) ) camera.RotateAroundWorldUp(100.0f * msPerFrame / 1000.0f);
        if ( pRamen->KeyPressed(SDLK_RIGHT) ) camera.RotateAroundWorldUp(-100.0f * msPerFrame / 1000.0f);
        if ( pRamen->KeyPressed(SDLK_W) ) camera.DollyForward(DOLLY_SPEED * (float)msPerFrame / 1000.0f);
        if ( pRamen->KeyPressed(SDLK_S) ) camera.DollyForward(-DOLLY_SPEED * (float)msPerFrame / 1000.0f);
        if ( pRamen->KeyPressed(SDLK_A) ) camera.DollySide(-DOLLY_SPEED * msPerFrame / 1000.0f);
        if ( pRamen->KeyPressed(SDLK_D) ) camera.DollySide(DOLLY_SPEED * msPerFrame / 1000.0f);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        static float modelScale        = 1.0f;
        static bool  vsyncOn           = true;
        static bool  recomputeLightmap = true;
        ImGuiIO&     io                = ImGui::GetIO();
        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        if ( ImGui::Checkbox("Enable VSync", &vsyncOn) )
        {
            SDL_GL_SetSwapInterval((int)vsyncOn); /* 1 = VSync enabled; 0 = VSync disabled */
        }
        ImGui::End();

        ImGui::Begin("Lightmap Debugger");
        {
            ImGui::SliderInt("# Bunces", (int*)&numBounces, 0, 50);
            ImGui::SliderInt("# Iterations", (int*)&numIterations, 0, 5000); // 500
            if ( ImGui::Button("Recompute Lightmap") )
            {
                recomputeLightmap   = true;
                iterationCounter    = 0;
                patches_ssbo_toggle = 0;
                frameNum            = 0;
            }
            ImGui::Text("xAtlas UV Map");
            ImTextureID imguiXatlasTexture = (ImTextureID)(intptr_t)xatlasUV_Texture;
            ImVec2      xatlasTextureSize
                = ImVec2{ (float)xatlasUV_Image.GetWidth() / 3.0f, (float)xatlasUV_Image.GetHeight() / 3.0f };
            ImGui::ImageWithBg((void*)(intptr_t)imguiXatlasTexture,
                               xatlasTextureSize,
                               ImVec2(0.0f, 0.0f),
                               ImVec2(1.0f, 1.0f),
                               ImVec4(0.5f, 0.3f, 0.9f, 1.0f),  /* background color */
                               ImVec4(1.0f, 0.0f, 1.0f, 1.0f)); /* tint color */
        }
        {
            ImGui::Text("Patches (Lightmap UV)");
            ImTextureID imguiPatchDebugTexture = (ImTextureID)(intptr_t)patchDebugTexture;
            ImVec2      patchDebugTextureSize
                = ImVec2{ (float)patchDebugImage.GetWidth() / 3.0f, (float)patchDebugImage.GetHeight() / 3.0f };
            ImGui::Image(
                (void*)(intptr_t)imguiPatchDebugTexture, patchDebugTextureSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        }
        {
            ImGui::Text("Lightmap");
            ImTextureID imguiLightmapTexture = (ImTextureID)(intptr_t)lightmapTexture;
            ImVec2      lightmapTextureSize  = ImVec2{ (float)LIGHT_MAP_SIZE / 3.0f, (float)LIGHT_MAP_SIZE / 3.0f };
            ImGui::Image(
                (void*)(intptr_t)imguiLightmapTexture, lightmapTextureSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        }

        ImGui::End();

        /* Query new frame dimensions */
        int windowWidth, windowHeight;
        SDL_GetWindowSize(pRamen->GetWindow(), &windowWidth, &windowHeight);

        /* View mat */
        Mat4f viewMat = LookAt(camera.GetPosition(), camera.GetPosition() + camera.GetForward(), camera.GetUp());
        /* Projection mat */
        float aspect  = (float)windowWidth / (float)windowHeight;
        Mat4f projMat = PerspectiveProjection(TO_RAD(60.0f), aspect, 0.01f, 10000.0f);

        modelMat = Mat4f::Identity();
        Scale(modelMat, modelScale);

        /* Rendering */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

        /* Adjust viewport and perspective projection accordingly. */
        glViewport(0, 0, windowWidth, windowHeight);

        /* Run compute shader */
        if ( recomputeLightmap )
        {
            computeShader.Use();
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, TriangleSSBO);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, VerticesSSBO);
            glBindImageTexture(0, lightmapTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glUniform1ui(2, levelIndicesAsU32.size());
            glUniform1ui(3, numBounces);
            glUniform1ui(4, numIterations);
            {
                glUniform1ui(5, 1); /* Tell shader to reset patches */
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, patchesSSBO0);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, patchesSSBO1);
                glDispatchCompute(LIGHT_MAP_SIZE / 4, LIGHT_MAP_SIZE / 4, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, patchesSSBO1);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, patchesSSBO0);
                glDispatchCompute(LIGHT_MAP_SIZE / 4, LIGHT_MAP_SIZE / 4, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
            }
            recomputeLightmap = false;
        }

        if ( iterationCounter < numIterations )
        {
            computeShader.Use();
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, TriangleSSBO);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, VerticesSSBO);
            glBindImageTexture(0, lightmapTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glUniform1ui(2, levelIndicesAsU32.size());
            glUniform1ui(3, numBounces);
            glUniform1ui(4, numIterations);
            glUniform1ui(5, 0); /* Don't reset patches */
            {
                glUniform1ui(1, frameNum);
                frameNum++;
                GLuint patchesReadSSBO  = patcheSSBOs[ patches_ssbo_toggle ^ 1 ];
                GLuint patchesWriteSSBO = patcheSSBOs[ patches_ssbo_toggle ];
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, patchesReadSSBO);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, patchesWriteSSBO);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
                glDispatchCompute(LIGHT_MAP_SIZE / 4, LIGHT_MAP_SIZE / 4, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
                glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
                /* Switch SSBOs for next iteration */
                patches_ssbo_toggle ^= 1;
            }
            iterationCounter++;
        }

        /* The following calls operate on indexed geometry, so use this VAO. */
        glBindVertexArray(levelGeometryVAO);

        /* ImGUI Rendering */
        ImGui::Render();

        /* Draw models from viewer's perspective */
        modelShader.Use();
        glUniformMatrix4fv(0, 1, GL_FALSE, modelMat.Data());
        glUniformMatrix4fv(4, 1, GL_FALSE, viewMat.Data());
        glUniformMatrix4fv(8, 1, GL_FALSE, projMat.Data());
        glUniform3fv(12, 1, camera.GetPosition().Data());
        glBindVertexBuffer(0, levelGeometryVBO, 0, sizeof(Vertex));
        glBindTextureUnit(5, patchDebugTexture);
        glBindTextureUnit(6, lightmapTexture);
        glDrawElementsBaseVertex(GL_TRIANGLES, levelMesh.NumIndices(), GL_UNSIGNED_SHORT, 0, 0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        pRamen->EndFrame();

        SDL_GL_SwapWindow(pRamen->GetWindow());

        endCounter = SDL_GetPerformanceCounter();
    }

    /* GL Resources shutdown. */
    modelShader.Delete();
    glDeleteBuffers(1, &levelGeometryVBO);
    glDeleteVertexArrays(1, &VAO);

    /* Ramen Shutdown */
    pRamen->Shutdown();

    /* Filesystem deinit */
    PHYSFS_deinit();

    return 0;
}
