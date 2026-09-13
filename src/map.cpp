#include "map.h"
#include "gfx.h"
#include <cmath>

namespace math
{
    namespace
    {
        void StretchMeshUVs(Mesh& mesh, float uScale, float vScale)
        {
            if (mesh.texcoords == nullptr || mesh.vertexCount <= 0)
            {
                return;
            }

            for (int i = 0; i < mesh.vertexCount * 2; i += 2)
            {
                mesh.texcoords[i] *= uScale * 2;
                mesh.texcoords[i + 1] *= vScale;
            }
        }
    }

    Map::Map(int* data, int width, int height, float cellSize, const char* texturePath)
        : data(data),
        width(width),
        height(height),
        cellSize(cellSize),
        wallTexture({ 0 }),
        wallModel({ 0 }),
        floorTexture({ 0 }),
        floorTileModel({ 0 }),
        ceilingTexture({ 0 }),
        ceilingTileModel({ 0 }),
        notebookTexture({ 0 })
    {
#if defined(PLATFORM_WEB)
        const char* wallPath = "assets/wall.png";
        const char* floorPath = "assets/floor.png";
        const char* ceilingPath = "assets/ceiling.png";
        const char* notebookPath = "assets/notebook.png";
#else
        const char* wallPath = "../assets/wall.png";
        const char* floorPath = "../assets/floor.png";
        const char* ceilingPath = "../assets/ceiling.png";
        const char* notebookPath = "../assets/notebook.png";
#endif

        wallTexture = LoadTexture(texturePath);
        if (wallTexture.id == 0)
        {
            wallTexture = LoadTexture(wallPath);
        }

        if (wallTexture.id == 0)
        {
            TraceLog(LOG_WARNING, "No se pudo cargar la textura del muro: %s", texturePath);
        }
        else
        {
            SetTextureWrap(wallTexture, TEXTURE_WRAP_CLAMP);
            SetTextureFilter(wallTexture, TEXTURE_FILTER_BILINEAR);

            Mesh wallMesh = GenMeshCube(cellSize, 6.0f, cellSize);
            StretchMeshUVs(wallMesh, 1.35f, 1.35f);
            wallModel = LoadModelFromMesh(wallMesh);
            wallModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = wallTexture;
        }

        floorTexture = LoadTexture(floorPath);
        if (floorTexture.id != 0)
        {
            SetTextureWrap(floorTexture, TEXTURE_WRAP_REPEAT);
            SetTextureFilter(floorTexture, TEXTURE_FILTER_BILINEAR);

            Mesh floorMesh = GenMeshCube(cellSize, 0.5f, cellSize);
            StretchMeshUVs(floorMesh, 1.35f, 1.35f);
            floorTileModel = LoadModelFromMesh(floorMesh);
            floorTileModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = floorTexture;
        }

        ceilingTexture = LoadTexture(ceilingPath);
        if (ceilingTexture.id != 0)
        {
            SetTextureWrap(ceilingTexture, TEXTURE_WRAP_REPEAT);
            SetTextureFilter(ceilingTexture, TEXTURE_FILTER_BILINEAR);

            Mesh ceilingMesh = GenMeshCube(cellSize, 0.5f, cellSize);
            StretchMeshUVs(ceilingMesh, 1.35f, 1.35f);
            ceilingTileModel = LoadModelFromMesh(ceilingMesh);
            ceilingTileModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = ceilingTexture;
        }

        notebookTexture = LoadTexture(notebookPath);
        if (notebookTexture.id != 0)
        {
            SetTextureWrap(notebookTexture, TEXTURE_WRAP_CLAMP);
            SetTextureFilter(notebookTexture, TEXTURE_FILTER_BILINEAR);
        }
    }

    bool Map::HasNearbyNotebook(const Vector3& cameraPosition, float detectionDistance) const
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (data[y * width + x] != 3)
                {
                    continue;
                }

                Vector3 position = {
                    (x + 0.5f) * cellSize,
                    1.5f,
                    (y + 0.5f) * cellSize
                };

                float dx = cameraPosition.x - position.x;
                float dy = cameraPosition.y - position.y;
                float dz = cameraPosition.z - position.z;
                float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

                if (distance < detectionDistance)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void Map::Draw(const Camera3D& camera)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if (floorTileModel.meshCount > 0)
                {
                    DrawModel(
                        floorTileModel,
                        { (x + 0.5f) * cellSize, -0.25f, (y + 0.5f) * cellSize },
                        1.0f,
                        WHITE
                    );
                }

                if (ceilingTileModel.meshCount > 0)
                {
                    DrawModel(
                        ceilingTileModel,
                        { (x + 0.5f) * cellSize, 6.25f, (y + 0.5f) * cellSize },
                        1.0f,
                        WHITE
                    );
                }

                int cell = data[y * width + x];

                if (cell == 1)
                {
                    Vector3 position = {
                        (x + 0.5f) * cellSize,
                        3.0f,
                        (y + 0.5f) * cellSize
                    };

                    if (wallModel.meshCount > 0)
                    {
                        DrawModel(wallModel, position, 1.0f, WHITE);
                    }
                    else
                    {
                        DrawCube(
                            { x * cellSize, 1.0f, y * cellSize },
                            cellSize,
                            6.0f,
                            cellSize,
                            GRAY
                        );
                    }
                }
                else if (cell == 3)
                {
                    Vector3 position = {
                        (x + 0.5f) * cellSize,
                        1.5f,
                        (y + 0.5f) * cellSize
                    };

                    float dx = camera.position.x - position.x;
                    float dy = camera.position.y - position.y;
                    float dz = camera.position.z - position.z;
                    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

                    if (distance < 2.5f)
                    {
                        continue;
                    }

                    if (notebookTexture.id != 0)
                    {
                        DrawBillboard(camera, notebookTexture, position, 1.8f, WHITE);
                    }
                    else
                    {
                        DrawCube(
                            { (x + 0.5f) * cellSize, 1.0f, (y + 0.5f) * cellSize },
                            cellSize * 0.7f,
                            0.9f,
                            0.15f,
                            BEIGE
                        );
                    }
                }
            }
        }
    }

}

