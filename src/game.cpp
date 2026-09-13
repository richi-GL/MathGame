#include "game.h"
#include <raylib.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "rlgl.h"
#include "map.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

namespace math
{
    namespace
    {
        enum class ChallengeType
        {
            Sum,
            Subtraction,
            Multiplication,
            Division,
            Linear
        };

        struct ChallengeData
        {
            ChallengeType type;
            std::string title;
            std::string explanation;
            int a = 0;
            int b = 0;
            int expected = 0;
            std::string prompt;
        };

        std::vector<std::pair<int, int>> GetFreeCells(int width, int height, const int* level)
        {
            std::vector<std::pair<int, int>> freeCells;
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (level[y * width + x] == 0)
                    {
                        freeCells.emplace_back(x, y);
                    }
                }
            }
            return freeCells;
        }

        std::vector<std::pair<int, int>> GenerateNotebookPositions(int width, int height, int* level)
        {
            std::vector<std::pair<int, int>> freeCells = GetFreeCells(width, height, level);
            std::vector<std::pair<int, int>> notebookPositions;

            while (notebookPositions.size() < 7 && !freeCells.empty())
            {
                int index = rand() % static_cast<int>(freeCells.size());
                std::pair<int, int> candidate = freeCells[index];
                freeCells.erase(freeCells.begin() + index);

                bool valid = true;
                for (const auto& pos : notebookPositions)
                {
                    int dx = std::abs(candidate.first - pos.first);
                    int dy = std::abs(candidate.second - pos.second);
                    int manhattan = dx + dy;

                    if (manhattan < 4 || manhattan > 10)
                    {
                        valid = false;
                        break;
                    }
                }

                if (valid)
                {
                    notebookPositions.push_back(candidate);
                    level[candidate.second * width + candidate.first] = 3;
                }
            }

            if (notebookPositions.size() < 7)
            {
                for (const auto& cell : freeCells)
                {
                    if (notebookPositions.size() >= 7)
                    {
                        break;
                    }

                    bool alreadyPlaced = false;
                    for (const auto& pos : notebookPositions)
                    {
                        if (cell == pos)
                        {
                            alreadyPlaced = true;
                            break;
                        }
                    }

                    if (!alreadyPlaced)
                    {
                        notebookPositions.push_back(cell);
                        level[cell.second * width + cell.first] = 3;
                    }
                }
            }

            return notebookPositions;
        }

        bool IsFirstExplanationShown(const std::array<bool, 5>& shown, ChallengeType type)
        {
            return !shown[static_cast<int>(type)];
        }

        void DrawWrappedTextEveryNWords(const std::string& text, int x, int y, int fontSize, Color color, int wordsPerLine)
        {
            std::istringstream stream(text);
            std::vector<std::string> words;
            std::string word;

            while (stream >> word)
            {
                words.push_back(word);
            }

            int lineY = y;
            for (size_t i = 0; i < words.size();)
            {
                std::string line;
                int count = 0;

                while (i < words.size() && count < wordsPerLine)
                {
                    if (!line.empty())
                    {
                        line += " ";
                    }
                    line += words[i];
                    ++i;
                    ++count;
                }

                DrawText(line.c_str(), x, lineY, fontSize, color);
                lineY += fontSize + 6;
            }
        }

        bool IsWallCollision(const int* level, int width, int height, float cellSize, const Vector3& position)
        {
            int cellX = static_cast<int>(position.x / cellSize);
            int cellZ = static_cast<int>(position.z / cellSize);

            for (int z = -1; z <= 1; ++z)
            {
                for (int x = -1; x <= 1; ++x)
                {
                    int cx = cellX + x;
                    int cz = cellZ + z;

                    if (cx < 0 || cx >= width || cz < 0 || cz >= height)
                    {
                        return true;
                    }

                    if (level[cz * width + cx] != 1)
                    {
                        continue;
                    }

                    Vector3 wallCenter = {
                        (cx + 0.5f) * cellSize,
                        3.0f,
                        (cz + 0.5f) * cellSize
                    };

                    float dx = std::abs(position.x - wallCenter.x);
                    float dz = std::abs(position.z - wallCenter.z);
                    if (dx < cellSize * 0.65f && dz < cellSize * 0.65f)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        void ResetNotebookLayout(int* level, int width, int height)
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (level[y * width + x] == 3)
                    {
                        level[y * width + x] = 0;
                    }
                }
            }

            GenerateNotebookPositions(width, height, level);
        }

        ChallengeData BuildChallenge(ChallengeType type)
        {
            ChallengeData data;
            data.type = type;

            switch (type)
            {
                case ChallengeType::Sum:
                    data.title = "Suma";
                    data.explanation = "La suma combina dos cantidades para obtener un total. Solo tienes que sumar ambos valores.";
                    data.a = rand() % 9 + 1;
                    data.b = rand() % 9 + 1;
                    data.expected = data.a + data.b;
                    data.prompt = std::to_string(data.a) + " + " + std::to_string(data.b) + " = ?";
                    break;

                case ChallengeType::Subtraction:
                    data.title = "Resta";
                    data.explanation = "La resta quita una cantidad de otra. Encuentra la diferencia entre ambas.";
                    data.a = rand() % 12 + 6;
                    data.b = rand() % data.a + 1;
                    data.expected = data.a - data.b;
                    data.prompt = std::to_string(data.a) + " - " + std::to_string(data.b) + " = ?";
                    break;

                case ChallengeType::Multiplication:
                    data.title = "Multiplicacion";
                    data.explanation = "La multiplicacion repite una cantidad varias veces. Haz la cuenta como una suma repetida.";
                    data.a = rand() % 7 + 2;
                    data.b = rand() % 7 + 2;
                    data.expected = data.a * data.b;
                    data.prompt = std::to_string(data.a) + " x " + std::to_string(data.b) + " = ?";
                    break;

                case ChallengeType::Division:
                    data.title = "Division";
                    data.explanation = "La division reparte una cantidad en partes iguales. Busca cuantas veces cabe el divisor.";
                    data.b = rand() % 5 + 2;
                    data.expected = rand() % 7 + 2;
                    data.a = data.expected * data.b;
                    data.prompt = std::to_string(data.a) + " / " + std::to_string(data.b) + " = ?";
                    break;

                case ChallengeType::Linear:
                    data.title = "Ecuacion lineal";
                    data.explanation = "La ecuacion lineal tiene la forma x + b = c. Para resolverla, resta b de c y encuentra el valor de x.";
                    data.b = rand() % 8 + 2;
                    data.expected = rand() % 12 + 7;
                    data.a = data.expected - data.b;
                    data.prompt = "x + " + std::to_string(data.b) + " = " + std::to_string(data.expected);
                    break;
            }

            return data;
        }
    }

    namespace
    {
        struct GameLoopState
        {
            int level[16 * 12] = {};
            Map* map = nullptr;
            Camera3D* camera = nullptr;
            Texture2D skyboxTexture = { 0 };
            Model skyboxModel = { 0 };
            bool mathChallengeActive = false;
            bool challengeIntroVisible = false;
            bool gameWon = false;
            std::array<bool, 5> introShown = { false, false, false, false, false };
            std::string answerInput;
            ChallengeData activeChallenge;
            int solvedChallengeCount = 0;
            int currentNotebookX = -1;
            int currentNotebookY = -1;
            float mistakeTimer = 0.0f;
            std::array<ChallengeType, 7> challengeOrder = {
                ChallengeType::Sum,
                ChallengeType::Subtraction,
                ChallengeType::Subtraction,
                ChallengeType::Multiplication,
                ChallengeType::Multiplication,
                ChallengeType::Division,
                ChallengeType::Linear
            };
        };

        void GameLoopFrame(void* userData)
        {
            auto* state = static_cast<GameLoopState*>(userData);
            if (state == nullptr || state->camera == nullptr || state->map == nullptr)
            {
                return;
            }

            if (state->mistakeTimer > 0.0f)
            {
                state->mistakeTimer -= GetFrameTime();
                if (state->mistakeTimer <= 0.0f)
                {
                    state->solvedChallengeCount = 0;
                    state->introShown = { false, false, false, false, false };
                    ResetNotebookLayout(state->level, 16, 12);
                    state->mathChallengeActive = false;
                    state->challengeIntroVisible = false;
                    state->answerInput.clear();
                    state->currentNotebookX = -1;
                    state->currentNotebookY = -1;
                }
            }

            if (!state->mathChallengeActive && !state->gameWon && state->mistakeTimer <= 0.0f)
            {
                float nearestDistance = 9999.0f;
                bool foundNotebook = false;
                int notebookX = -1;
                int notebookY = -1;

                for (int y = 0; y < 12; ++y)
                {
                    for (int x = 0; x < 16; ++x)
                    {
                        if (state->level[y * 16 + x] != 3)
                        {
                            continue;
                        }

                        Vector3 position = {
                            (x + 0.5f) * 6.0f,
                            1.5f,
                            (y + 0.5f) * 6.0f
                        };

                        float dx = state->camera->position.x - position.x;
                        float dy = state->camera->position.y - position.y;
                        float dz = state->camera->position.z - position.z;
                        float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

                        if (distance < nearestDistance)
                        {
                            nearestDistance = distance;
                            notebookX = x;
                            notebookY = y;
                            foundNotebook = true;
                        }
                    }
                }

                if (foundNotebook && nearestDistance < 2.5f && state->solvedChallengeCount < static_cast<int>(state->challengeOrder.size()))
                {
                    state->currentNotebookX = notebookX;
                    state->currentNotebookY = notebookY;
                    state->mathChallengeActive = true;
                    state->challengeIntroVisible = IsFirstExplanationShown(state->introShown, state->challengeOrder[state->solvedChallengeCount]);
                    state->activeChallenge = BuildChallenge(state->challengeOrder[state->solvedChallengeCount]);
                    state->answerInput.clear();
                }
                else
                {
                    Vector3 previous = state->camera->position;
                    UpdateCamera(state->camera, CAMERA_FIRST_PERSON);
                    if (IsWallCollision(state->level, 16, 12, 6.0f, state->camera->position))
                    {
                        state->camera->position = previous;
                    }
                }
            }

            BeginDrawing();
            ClearBackground(BLACK);

            BeginMode3D(*state->camera);
                rlDisableDepthTest();
                rlDisableBackfaceCulling();
                DrawModel(state->skyboxModel, state->camera->position, 200.0f, WHITE);
                rlEnableBackfaceCulling();
                rlEnableDepthTest();

                state->map->Draw(*state->camera);
            EndMode3D();

            if (state->mathChallengeActive)
            {
                int screenW = GetScreenWidth();
                int screenH = GetScreenHeight();
                DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.72f));
                DrawRectangle(screenW / 2 - 260, screenH / 2 - 120, 520, 240, DARKGRAY);

                if (state->challengeIntroVisible)
                {
                    std::string title = state->activeChallenge.title;
                    DrawText(title.c_str(), screenW / 2 - MeasureText(title.c_str(), 26) / 2, screenH / 2 - 80, 26, WHITE);

                    DrawWrappedTextEveryNWords(state->activeChallenge.explanation, screenW / 2 - 220, screenH / 2 - 20, 20, RAYWHITE, 5);
                    DrawText("Pulsa ENTER para continuar.", screenW / 2 - 150, screenH / 2 + 70, 20, WHITE);

                    if (IsKeyPressed(KEY_ENTER))
                    {
                        state->introShown[static_cast<int>(state->activeChallenge.type)] = true;
                        state->challengeIntroVisible = false;
                        state->answerInput.clear();
                    }
                }
                else
                {
                    std::string title = state->activeChallenge.title;
                    DrawText(title.c_str(), screenW / 2 - MeasureText(title.c_str(), 26) / 2, screenH / 2 - 90, 26, WHITE);
                    DrawText(state->activeChallenge.prompt.c_str(), screenW / 2 - MeasureText(state->activeChallenge.prompt.c_str(), 28) / 2, screenH / 2 - 20, 28, RAYWHITE);

                    std::string prompt = "Respuesta: " + state->answerInput;
                    DrawText(prompt.c_str(), screenW / 2 - MeasureText(prompt.c_str(), 22) / 2, screenH / 2 + 40, 22, WHITE);

                    int key = GetKeyPressed();
                    if (key >= KEY_ZERO && key <= KEY_NINE)
                    {
                        int value = key - KEY_ZERO;
                        if (state->answerInput.size() < 6)
                        {
                            state->answerInput += std::to_string(value);
                        }
                    }
                    else if (IsKeyPressed(KEY_BACKSPACE) && !state->answerInput.empty())
                    {
                        state->answerInput.pop_back();
                    }
                    else if (IsKeyPressed(KEY_ENTER))
                    {
                        int answer = 0;
                        if (!state->answerInput.empty())
                        {
                            answer = std::stoi(state->answerInput);
                        }

                        if (answer == state->activeChallenge.expected)
                        {
                            if (state->solvedChallengeCount + 1 >= static_cast<int>(state->challengeOrder.size()))
                            {
                                state->level[state->currentNotebookY * 16 + state->currentNotebookX] = 0;
                                state->solvedChallengeCount++;
                                state->mathChallengeActive = false;
                                state->answerInput.clear();
                                state->currentNotebookX = -1;
                                state->currentNotebookY = -1;
                                state->gameWon = true;
                            }
                            else
                            {
                                state->level[state->currentNotebookY * 16 + state->currentNotebookX] = 0;
                                state->solvedChallengeCount++;
                                state->mathChallengeActive = false;
                                state->answerInput.clear();
                                state->currentNotebookX = -1;
                                state->currentNotebookY = -1;
                            }
                        }
                        else
                        {
                            state->mistakeTimer = 2.0f;
                            state->mathChallengeActive = false;
                            state->challengeIntroVisible = false;
                            state->answerInput.clear();
                            state->currentNotebookX = -1;
                            state->currentNotebookY = -1;
                            DrawText("Te equivocaste!", screenW / 2 - MeasureText("Te equivocaste!", 30) / 2, screenH / 2 + 120, 30, RED);
                        }
                    }
                }
            }

            if (state->mistakeTimer > 0.0f)
            {
                DrawText("Te equivocaste!", GetScreenWidth() / 2 - MeasureText("Te equivocaste!", 30) / 2, GetScreenHeight() / 2 + 120, 30, RED);
            }

            if (state->gameWon)
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));
                DrawText("YOU WON", GetScreenWidth() / 2 - MeasureText("YOU WON", 40) / 2, GetScreenHeight() / 2 - 20, 40, GREEN);
            }

            EndDrawing();
        }
    }

    int App::running()
    {
        InitWindow(1000, 600, "Math Game");

        static GameLoopState state;
#if defined(PLATFORM_WEB)
        const char* skyboxPath = "assets/skybox/skybox.png";
#else
        const char* skyboxPath = "../assets/skybox/skybox.png";
#endif
        state.skyboxTexture = LoadTexture(skyboxPath);
        Mesh skyboxMesh = GenMeshSphere(1.0f, 16, 16);
        state.skyboxModel = LoadModelFromMesh(skyboxMesh);
        state.skyboxModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = state.skyboxTexture;

        static const int initialLevel[16 * 12] = {
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
            1,0,1,1,1,0,1,0,1,1,1,1,1,0,0,1,
            1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1,
            1,0,1,0,1,1,1,1,1,1,1,0,1,0,0,1,
            1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,
            1,1,1,1,1,1,1,1,0,0,1,1,1,1,0,1,
            1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,
            1,0,1,1,1,1,0,1,1,1,1,1,1,0,1,1,
            1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,
            1,0,0,0,0,0,0,1,1,1,1,0,1,0,0,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        };
        std::copy(std::begin(initialLevel), std::end(initialLevel), std::begin(state.level));

        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        GenerateNotebookPositions(16, 12, state.level);

        static Map map(state.level, 16, 12, 6.0f);
        state.map = &map;

        static Camera3D camera = { 0 };
        camera.position = { 1.5f * 6.0f, 2.0f, 1.5f * 6.0f };
        camera.target = { camera.position.x, camera.position.y, camera.position.z + 2.0f };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        state.camera = &camera;

        DisableCursor();

#if defined(PLATFORM_WEB)
        emscripten_set_main_loop_arg(GameLoopFrame, &state, 0, 0);
#else
        SetTargetFPS(60);
        while (!WindowShouldClose())
        {
            GameLoopFrame(&state);
        }

        EnableCursor();
        UnloadModel(state.skyboxModel);
        UnloadTexture(state.skyboxTexture);
        CloseWindow();
        return 1;
#endif

        return 0;
    }
}
