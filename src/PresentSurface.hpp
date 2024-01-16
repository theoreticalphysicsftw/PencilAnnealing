// MIT License
// 
// Copyright (c) 2024 Mihail Mladenov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#pragma once

#include "Types.hpp"
#include "Time.hpp"
#include "Algebra.hpp"
#include "Error.hpp"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>


namespace PA
{
    struct LockedTexture
    {
        U32 width;
        U32 height;
        I32 stride;
        Byte* data;
    };

    struct PresentSurface
    {
        static auto Init(U32 height, U32 width, const C* appName = "PencilAnnealing") -> B;
        static auto Destroy() -> V;
        static auto PresentLoop() -> V;

        using RenderFunction = Function<V()>;

        static auto AddRenderingCode(const RenderFunction& func) -> V;
        static auto GetDimensions() -> Vec2;

        static auto LockScreenTarget() -> LockedTexture;
        static auto UnlockScreenTarget() -> V;

    private:
        static auto PresentLoopIteration() -> V;
        static auto ProcessInput() -> V;

        inline static SDL_Window* window = nullptr;
        inline static SDL_Renderer* renderer = nullptr;
        inline static SDL_Texture* screenTarget = nullptr;
        inline static RenderFunction renderFunction = [](){};


        inline static U32 width = 0;
        inline static U32 height = 0;
        inline static B isWindowClosed = false;
    };
}


namespace PA
{
    auto PresentSurface::Init(U32 width, U32 height, const C* appName) -> B
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            isWindowClosed = true;
            return false;
        }

        PresentSurface::width = width;
        PresentSurface::height = height;

        window = SDL_CreateWindow(
            appName,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_SHOWN
        );

        if (window == nullptr)
        {
            isWindowClosed = true;
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        screenTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        return renderer != nullptr && screenTarget != nullptr;
    }


    auto PresentSurface::Destroy() -> V
    {
        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
            SDL_Quit();
        }
        isWindowClosed = true;
    }


    auto PresentSurface::ProcessInput() -> V
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {

            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                isWindowClosed = true;
            }

            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                //width = event.window.data1;
                //height = event.window.data2;
            }

            if (event.type == SDL_KEYDOWN)
            {
                //switch (event.key.keysym.sym)
            }
        }
    }


    auto PresentSurface::PresentLoopIteration() -> V
    {
        ProcessInput();
        renderFunction();
        SDL_Rect copyRect = { 0, 0, (I32)width, (I32)height };
        SDL_RenderCopy(renderer, screenTarget, &copyRect, &copyRect);
        SDL_RenderPresent(renderer);
    }


    auto PresentSurface::PresentLoop() -> V
    {
        while (!isWindowClosed)
        {
            PresentLoopIteration();
        }
    }


    auto PresentSurface::AddRenderingCode(const RenderFunction& func) -> V
    {
        renderFunction = func;
    }


    inline auto PresentSurface::GetDimensions() -> Vec2
    {
        return Vec2(width, height);
    }


    inline auto PresentSurface::LockScreenTarget() -> LockedTexture
    {
        LockedTexture result;
        result.width = width;
        result.height = height;
        if (SDL_LockTexture(screenTarget, nullptr, (V**)&result.data, &result.stride))
        {
            LogError(SDL_GetError());
            Terminate();
        }
        return result;
    }


    inline auto PresentSurface::UnlockScreenTarget() -> V
    {
        SDL_UnlockTexture(screenTarget);
    }
}