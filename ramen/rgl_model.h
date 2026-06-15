#ifndef RGL_MODEL_H
#define RGL_MODEL_H

#include <map>
#include <string>

#include <glad/glad.h>

#include <tiny_obj_loader.h>

#include "rgl_filesystem.h"
#include "rgl_math.h"
#include "rgl_platform.h"

struct Vertex
{
    Vec3f position;
    Vec3f normal;
    Vec3f color;
    Vec3f uv;
    Vec3f tangent;
    Vec3f bitangent;

    // Padding to full up to multiples of vec4 for compute-shader.
    float padding0;
    float padding1;

    // Total size: (6 * 3 * 4) + (2*4) = 80 bytes;
};

static void ComputeTangents(Vertex& v0, Vertex& v1, Vertex& v2)
{
    const Vec3f& p0 = v0.position;
    const Vec3f& p1 = v1.position;
    const Vec3f& p2 = v2.position;
    const Vec3f  e1 = p1 - p0;
    const Vec3f  e2 = p2 - p0;
    const Vec3f& t0 = v0.uv;
    const Vec3f& t1 = v1.uv;
    const Vec3f& t2 = v2.uv;

    const float x1 = t1.x - t0.x;
    const float y1 = t1.y - t0.y;
    const float x2 = t2.x - t0.x;
    const float y2 = t2.y - t0.y;

    const Mat2f uvFrame    = Mat2f{ x1, y1, x2, y2 };
    const float uvFrameDet = uvFrame.Det();

    Vec3f tangent   = Vec3f{ e1.x * y2 + e2.x * -y1, e1.y * y2 + e2.y * -y1, e1.z * y2 + e2.z * -y1 };
    Vec3f bitangent = Vec3f{ e1.x * -x2 + e2.x * x1, e1.y * -x2 + e2.y * x1, e1.z * -x2 + e2.z * x1 };
    tangent *= uvFrameDet;
    bitangent *= uvFrameDet;

    v0.tangent   = tangent;
    v1.tangent   = tangent;
    v2.tangent   = tangent;
    v0.bitangent = bitangent;
    v1.bitangent = bitangent;
    v2.bitangent = bitangent;
}

class Model
{
  public:
    bool Load(const char* file, bool generateIndexed = false)
    {
        File modelFile = Filesystem::Instance()->Read(file);
        if ( !modelFile.data )
        {
            return false;
        }

        tinyobj::ObjReader       reader;
        tinyobj::ObjReaderConfig reader_config;
        reader_config.vertex_color    = true;
        reader_config.mtl_search_path = ""; // not needed if no MTL
        if ( !reader.ParseFromString(std::string(modelFile.data), std::string("")) )
        {
            if ( !reader.Error().empty() )
            {
                fprintf(stderr, "tinyobjloader: %s\n", reader.Error().c_str());
            }
            return false;
            modelFile.Destroy();
        }

        const tinyobj::attrib_t& attrib = reader.GetAttrib();
        m_Shapes                        = reader.GetShapes();

        printf("loaded: '%s'\n", file);
        printf("# of vertices: %d\n", (int)(attrib.vertices.size()) / 3);
        printf("# of normals/vertex: %d\n", (int)(attrib.normals.size() / 3));
        printf("# of texcoords: %d\n", (int)attrib.texcoords.size() / 2);
        printf("# of shapes: %u\n", (int)m_Shapes.size());

        if ( generateIndexed )
        {
            // We don't know the total size yet because 1 position might become 3 vertices
            // due to different normals/UVs. So we use a vector and push_back.
            m_Vertices.clear();
            m_Indices.clear();

            // This map stores: <triple of indices, index in our m_Vertices array>
            // We use a tuple of the 3 indices from tinyobj to identify a "unique" vertex
            std::map<std::tuple<int, int, int>, uint32_t> uniqueVertices;

            for ( size_t i = 0; i < m_Shapes.size(); i++ )
            {
                const tinyobj::shape_t& shape      = m_Shapes[ i ];
                const tinyobj::mesh_t&  mesh       = shape.mesh;
                size_t                  numIndices = mesh.indices.size();

                for ( size_t j = 0; j < numIndices; j++ )
                {
                    tinyobj::index_t idx = mesh.indices[ j ];

                    // Create a unique key for this specific combination of attributes
                    std::tuple<int, int, int> vertexKey
                        = std::make_tuple(idx.vertex_index, idx.texcoord_index, idx.normal_index);

                    uint32_t existingVertexIndex;

                    // Check if we have already processed this exact combination
                    if ( uniqueVertices.count(vertexKey) == 0 )
                    {
                        // NEW VERTEX: Create it and add to m_Vertices
                        Vertex v{};

                        // Position
                        v.position.x = attrib.vertices[ 3 * idx.vertex_index + 0 ];
                        v.position.y = attrib.vertices[ 3 * idx.vertex_index + 1 ];
                        v.position.z = attrib.vertices[ 3 * idx.vertex_index + 2 ];

                        // Color
                        v.color.x = attrib.colors[ 3 * idx.vertex_index + 0 ];
                        v.color.y = attrib.colors[ 3 * idx.vertex_index + 1 ];
                        v.color.z = attrib.colors[ 3 * idx.vertex_index + 2 ];

                        // UV
                        if ( idx.texcoord_index >= 0 )
                        {
                            v.uv.x = attrib.texcoords[ 2 * idx.texcoord_index + 0 ];
                            v.uv.y = attrib.texcoords[ 2 * idx.texcoord_index + 1 ];
                        }

                        // Normal
                        if ( idx.normal_index >= 0 )
                        {
                            v.normal.x = attrib.normals[ 3 * idx.normal_index + 0 ];
                            v.normal.y = attrib.normals[ 3 * idx.normal_index + 1 ];
                            v.normal.z = attrib.normals[ 3 * idx.normal_index + 2 ];
                        }

                        // Add to our buffers
                        existingVertexIndex = static_cast<uint32_t>(m_Vertices.size());
                        m_Vertices.push_back(v);

                        // Record this key in the map
                        uniqueVertices[ vertexKey ] = existingVertexIndex;
                    }
                    else
                    {
                        // REUSE VERTEX: Get the index of the vertex we already created
                        existingVertexIndex = uniqueVertices[ vertexKey ];
                    }

                    // Add the index to our index buffer
                    m_Indices.push_back(existingVertexIndex);
                }
            }
        }
        else
        {
            for ( size_t i = 0; i < m_Shapes.size(); i++ )
            {
                for ( size_t j = 0; j < m_Shapes[ i ].mesh.indices.size(); j++ )
                {
                    tinyobj::index_t idx = m_Shapes[ i ].mesh.indices[ j ];
                    Vertex           v{};
                    /* UV */
                    if ( idx.texcoord_index >= 0 )
                    {
                        v.uv.x = attrib.texcoords[ 2 * idx.texcoord_index ];
                        v.uv.y = attrib.texcoords[ 2 * idx.texcoord_index + 1 ];
                    }
                    for ( int k = 0; k < 3; k++ )
                    {
                        /* Position */
                        v.position[ k ] = attrib.vertices[ 3 * idx.vertex_index + k ];

                        /* Normal */
                        if ( idx.normal_index >= 0 ) v.normal[ k ] = attrib.normals[ 3 * idx.normal_index + k ];
                    }
                    m_Vertices.push_back(v);
                }
            }
        }
        printf("m_Vertices.size(): %lu\n", m_Vertices.size());
        printf("m_Indices.size(): %lu\n", m_Indices.size());

        /* Compute Tangent / Bitangent */
#if 1
        if ( generateIndexed )
        {
            for ( size_t i = 0; i < m_Indices.size(); i += 3 )
            {
                uint16_t idx0 = m_Indices[ i + 0 ];
                uint16_t idx1 = m_Indices[ i + 1 ];
                uint16_t idx2 = m_Indices[ i + 2 ];
                Vertex&  v0   = m_Vertices[ idx0 ];
                Vertex&  v1   = m_Vertices[ idx1 ];
                Vertex&  v2   = m_Vertices[ idx2 ];
                ComputeTangents(v0, v1, v2);
            }
        }
        else
        {
            for ( size_t i = 0; i < m_Vertices.size(); i += 3 )
            {
                Vertex& v0 = m_Vertices[ i + 0 ];
                Vertex& v1 = m_Vertices[ i + 1 ];
                Vertex& v2 = m_Vertices[ i + 2 ];
                ComputeTangents(v0, v1, v2);
            }
        }

        /* Normalize tangents / bitangents */
        //  TODO: Orthonormalize.
        for ( size_t i = 0; i < m_Vertices.size(); i++ )
        {
            Vertex& v = m_Vertices[ i ];
            v.tangent.Normalize();
            v.bitangent.Normalize();
        }
#endif

        modelFile.Destroy();

        return true;
    }

    void SetVertex(size_t index, const Vertex& v)
    {
        m_Vertices[ index ] = v;
    }

    const std::vector<Vertex>& GetVertices() const
    {
        return m_Vertices;
    }

    const std::vector<uint16_t>& GetIndices() const
    {
        return m_Indices;
    }

    const size_t NumVertices()
    {
        return m_Vertices.size();
    }

    const size_t NumIndices()
    {
        return m_Indices.size();
    }

  private:
  private:
    std::vector<tinyobj::shape_t> m_Shapes;
    std::vector<Vertex>           m_Vertices;
    std::vector<uint16_t>         m_Indices;
};

#endif
