// Copyright 2024 Mihail Mladenov
//
// This file is part of PencilAnnealing.
//
// PencilAnnealing is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PencilAnnealing is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PencilAnnealing.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include "Types.hpp"
#include "Time.hpp"
#include "Algebra.hpp"
#include "Error.hpp"
#include "Image.hpp"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>


namespace PA
{
    struct PresentSurface
    {
        static auto Init(U32 height, U32 width, U32 internalWidth = width, U32 internalHeight = height, const C* appName = "PencilAnnealing") -> B;
        static auto Destroy() -> V;
        static auto PresentLoop() -> V;

        using RenderFunction = Function<V()>;

        static auto AddRenderingCode(const RenderFunction& func) -> V;
        static auto GetDimensions() -> Vec2;

        static auto LockScreenTarget() -> LockedTexture;
        static auto UnlockScreenTarget() -> V;
        static auto IsClosed() -> B;

        static auto GetDisplayRes() -> Pair<U32, U32>;

    private:
        static auto PresentLoopIteration() -> V;
        static auto ProcessInput() -> V;
        static auto InitVideo() -> B;

        inline static SDL_Window* window = nullptr;
        inline static SDL_Renderer* renderer = nullptr;
        inline static SDL_Texture* screenTarget = nullptr;
        inline static RenderFunction renderFunction = [](){};


        inline static U32 width = 0;
        inline static U32 height = 0;
        inline static U32 internalWidth = 0;
        inline static U32 internalHeight = 0;
        inline static Atomic<B> isWindowClosed = false;
        inline static B isVideoSystemReady = false;
    };
}


namespace PA
{
    auto PresentSurface::InitVideo() -> B
    {
        if (!isVideoSystemReady)
        {
            if (!SDL_Init(SDL_INIT_VIDEO))
            {
                isVideoSystemReady = true;
                return true;
            }

            isWindowClosed = true;
            return false;
        }

        return true;
    }

    auto PresentSurface::Init(U32 width, U32 height, U32 internalWidth, U32 internalHeight, const C* appName) -> B
    {
        InitVideo();

        PresentSurface::width = width;
        PresentSurface::height = height;
        PresentSurface::internalWidth = internalWidth;
        PresentSurface::internalHeight = internalHeight;

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
        screenTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, internalWidth, internalHeight);
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
        SDL_RenderCopy(renderer, screenTarget, nullptr, nullptr);
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

    inline auto PresentSurface::GetDisplayRes() -> Pair<U32, U32>
    {
        InitVideo();
        SDL_DisplayMode dm;
        Pair<U32, U32> result = {};
        if (!SDL_GetDesktopDisplayMode(0, &dm))
        {
            result.first = dm.w;
            result.second = dm.h;
        }
        else
        {
            Log(SDL_GetError());
        }
        return result;
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


    inline auto PresentSurface::IsClosed() -> B
    {
        return isWindowClosed;
    }
}