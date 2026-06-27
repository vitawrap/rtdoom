#include <SDL_render.h>
#include "glad/glad.h"

#include "GameLoop.h"

#include <filesystem>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define MAIN_RETURN(x) return
#else
#define MAIN_RETURN(x) return (x)
#endif

#define ENABLE_GL 1

using namespace rtdoom;
using namespace std::string_literals;
using std::cout, std::endl, std::string;

void                       InitSDL(SDL_Renderer*& sdlRenderer, SDL_Window*& sdlWindow);
void                       DestroySDL(SDL_Renderer* sdlRenderer, SDL_Window* sdlWindow);
std::optional<std::string> FindWAD();

#include "WADFile.h"

bool platformInitialized = false;

int main(int /*argc*/, char** /*argv*/)
{
    static std::optional<std::string> wadFileName;
    static WADFile* wadFile = nullptr;
    static decltype(WADFile::m_maps)::iterator mapIter;

    static SDL_Renderer* sdlRenderer;
    static SDL_Window*   sdlWindow;

    static GameLoop* _gameLoop = nullptr;

    static uint64_t tickFrequency;
    static uint64_t tickCounter;
    static uint64_t frametimeCounter;

#ifdef __EMSCRIPTEN__
    static auto main_loop_wrap = []() -> void
#else
    while(!platformInitialized || _gameLoop->isRunning())
#endif
    {
        if (!platformInitialized) {
            wadFileName = FindWAD();
            if(!wadFileName.has_value())
            {
                std::cerr << "No WAD file found! Drop off a .wad from Doom or Freedoom to .exe directory.\n";
                MAIN_RETURN(EXIT_FAILURE);
            }
            wadFile = new WADFile(wadFileName.value());
            mapIter = wadFile->m_maps.begin();

            InitSDL(sdlRenderer, sdlWindow);
            _gameLoop = new GameLoop(sdlRenderer, sdlWindow, *wadFile);
            _gameLoop->Start(mapIter->second);

            tickFrequency = SDL_GetPerformanceFrequency();
            tickCounter   = SDL_GetPerformanceCounter();
            platformInitialized = true;
        }
        GameLoop& gameLoop = *_gameLoop;

        frametimeCounter = SDL_GetPerformanceCounter();

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
#ifndef __EMSCRIPTEN__ /* web app shouldn't be exit-able */
            if((SDL_QUIT == event.type) || (SDL_KEYDOWN == event.type && SDL_SCANCODE_ESCAPE == event.key.keysym.scancode))
            {
                gameLoop.Stop();
                break;
            }
#endif

            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                gameLoop.ResizeWindow(event.window.data1, event.window.data2);
            }

            if((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && event.key.repeat == 0)
            {
                auto k = event.key;
                auto p = event.type == SDL_KEYDOWN;
                switch(k.keysym.sym)
                {
                case SDLK_UP:
                    gameLoop.Move(p ? 1 : 0);
                    break;
                case SDLK_DOWN:
                    gameLoop.Move(p ? -1 : 0);
                    break;
                case SDLK_LEFT:
                    gameLoop.Rotate(p ? -1 : 0);
                    break;
                case SDLK_RIGHT:
                    gameLoop.Rotate(p ? 1 : 0);
                    break;
                case SDLK_TAB:
                    gameLoop.SetRenderMap(p);
                    break;
#ifndef _RT_PUBLIC
                case SDLK_1:
                    gameLoop.SetRenderingMode(Renderer::RenderingMode::Wireframe);
                    SDL_SetWindowTitle(sdlWindow, "rtdoom (Wireframe)");
                    break;
                case SDLK_2:
                    gameLoop.SetRenderingMode(Renderer::RenderingMode::Solid);
                    SDL_SetWindowTitle(sdlWindow, "rtdoom (Solid)");
                    break;
                case SDLK_3:
                    gameLoop.SetRenderingMode(Renderer::RenderingMode::Textured);
                    SDL_SetWindowTitle(sdlWindow, "rtdoom (Textured)");
                    break;
#   if(ENABLE_GL)
                case SDLK_4:
                    gameLoop.SetRenderingMode(Renderer::RenderingMode::OpenGL);
                    SDL_SetWindowTitle(sdlWindow, "rtdoom (OpenGL)");
                    break;
#   else
                case SDLK_4:
                    std::cout << "OpenGL not enabled, recompile with ENABLE_GL = 1" << std::endl;
                    break;
#   endif
                case SDLK_s:
                    if(p)
                    {
                        gameLoop.StepFrame();
                    }
                    break;
                case SDLK_m:
                    // next map
                    if(p)
                    {
                        mapIter++;
                        if(mapIter == wadFile->m_maps.end())
                        {
                            mapIter = wadFile->m_maps.begin();
                        }
                        gameLoop.Start(mapIter->second);
                    }
                    break;
                case SDLK_d:
                    cout << "Player position: (" << gameLoop.Player().x << ", " << gameLoop.Player().y << ", " << gameLoop.Player().a
                            << ")" << endl;
                    break;
#endif
                default:
                    break;
                }
            }
        }

        const Frame* frame = gameLoop.RenderFrame();

        const auto nextCounter = SDL_GetPerformanceCounter();
        const auto seconds     = (nextCounter - tickCounter) / static_cast<float>(tickFrequency);
        tickCounter            = nextCounter;
        gameLoop.Tick(seconds);

#ifndef __EMSCRIPTEN__
        // keep a consistent speed of *about* 60 fps to be on par with browser animation performance
        const auto frameSeconds = (nextCounter - frametimeCounter) / static_cast<float>(tickFrequency);
        if (frameSeconds < 0.016) {
            SDL_Delay(16 - (frameSeconds * 1000.f));
        }
#endif

#ifndef NDEBUG
        if(frame != NULL)
        {
            cout << "Frame time: " << seconds * 1000.0 << "ms: " << frame->m_numSegments << " segs, " << frame->m_numFloorPlanes << "+"
                    << frame->m_numCeilingPlanes << " planes, " << frame->m_sprites.size() << " sprites" << endl;
        }
        else
        {
            cout << "Frame time: " << seconds * 1000.0 << "ms" << endl;
        }
#endif
    };
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop_wrap, 0, 1);
    emscripten_exit_with_live_runtime();
#else
    delete _gameLoop;
    delete wadFile;

    DestroySDL(sdlRenderer, sdlWindow);
#endif
    return EXIT_SUCCESS;
}

void InitSDL(SDL_Renderer*& sdlRenderer, SDL_Window*& sdlWindow)
{
    if(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO))
    {
        throw std::runtime_error("Unable to initialize SDL: " + std::string(SDL_GetError()));
    }

    atexit(SDL_Quit);

#if(ENABLE_GL)
    SDL_GL_LoadLibrary(NULL);
#endif

    if constexpr(s_multisamplingLevel > 1.0f)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    }

    auto flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

#if(ENABLE_GL)
    flags |= SDL_WINDOW_OPENGL;
#endif

    sdlWindow = SDL_CreateWindow("rtdoom",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 s_displayX,
                                 s_displayY,
                                 flags);

    if(!sdlWindow)
    {
        throw std::runtime_error("Unable to create SDL window: " + std::string(SDL_GetError()));
    }

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);

    if(!sdlRenderer)
    {
        throw std::runtime_error("Unable to create SDL renderer: " + std::string(SDL_GetError()));    
    }
}

void DestroySDL(SDL_Renderer* sdlRenderer, SDL_Window* sdlWindow)
{
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
}

bool iequals(const string& a, const string& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) { return tolower(a) == tolower(b); });
}

std::optional<std::string> FindWAD()
{
    for(const auto& entry : std::filesystem::directory_iterator("."))
    {
        if(entry.is_regular_file() && iequals(entry.path().extension().string(), ".wad"))
        {
            return entry.path().generic_string();
        }
    }
    return std::nullopt;
}
