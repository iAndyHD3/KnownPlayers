#pragma once
#include <Geode/platform/cplatform.h>
#include <Geode/platform/platform.hpp>

namespace geode::platform
{
    #ifdef GEODE_IS_WINDOWS
        constexpr bool win = true;
    #else
        constexpr bool win = false;
    #endif

    #ifdef GEODE_IS_ANDROID64
        constexpr bool android64 = true;
    #else
        constexpr bool android64 = false;
    #endif

    #ifdef GEODE_IS_ANDROID32
        constexpr bool android32 = true;
    #else
        constexpr bool android32 = false;
    #endif

    #ifdef GEODE_IS_MACOS
        constexpr bool mac = true;
    #else
        constexpr bool mac = false;
    #endif

    #ifdef GEODE_IS_IOS
        constexpr bool ios = true;
    #else
        constexpr bool ios = false;
    #endif

    #ifdef GEODE_IS_DESKTOP
        constexpr bool desktop = true; //win + mac
    #else
        constexpr bool desktop = false; //win + mac
    #endif

    #ifdef GEODE_IS_ANDROID
        constexpr bool android = true; //android32 + android64
    #else
        constexpr bool android = false; //android32 + android64
    #endif

    #ifdef GEODE_IS_MOBILE
        constexpr bool mobile = true; //android32 + android64 + ios
    #else
        constexpr bool mobile = false; //android32 + android64 + ios
    #endif
}