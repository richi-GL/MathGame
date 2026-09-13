#pragma once
#include "gfx.h"

namespace math
{
    class Map
    {
    private:
        int* data;
        int width;
        int height;
        float cellSize;
        Texture2D wallTexture;
        Model wallModel;
        Texture2D floorTexture;
        Model floorTileModel;
        Texture2D ceilingTexture;
        Model ceilingTileModel;
        Texture2D notebookTexture;

    public:
        Map(int* data, int width, int height, float cellSize, const char* texturePath = "../assets/wall.png");

        bool HasNearbyNotebook(const Vector3& cameraPosition, float detectionDistance = 2.5f) const;
        void Draw(const Camera3D& camera);
    };

}

