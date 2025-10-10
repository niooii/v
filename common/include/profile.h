//
// Created by niooi on 10/9/2025.
//

#pragma once

// just a wrapper of tracy right now, maybe i'll want to do my own later

#ifdef TRACY_ENABLE
    #include <tracy/Tracy.hpp>

    // Zone profiling - captures function/scope execution time
    #define V_PROFILE_ZONE                          ZoneScoped
    #define V_PROFILE_ZONE_NAMED(name)              ZoneScopedN(name)
    #define V_PROFILE_ZONE_COLOR(color)             ZoneScopedC(color)
    #define V_PROFILE_ZONE_NAMED_COLOR(name, color) ZoneScopedNC(name, color)

    // Frame marking - marks frame boundaries for frame-based profiling
    #define V_PROFILE_FRAME_MARK             FrameMark
    #define V_PROFILE_FRAME_MARK_NAMED(name) FrameMarkNamed(name)
    #define V_PROFILE_FRAME_MARK_START(name) FrameMarkStart(name)
    #define V_PROFILE_FRAME_MARK_END(name)   FrameMarkEnd(name)

    // Zone annotation - add text/values to current zone
    #define V_PROFILE_ZONE_TEXT(text, size) ZoneText(text, size)
    #define V_PROFILE_ZONE_VALUE(value)     ZoneValue(value)
    #define V_PROFILE_ZONE_NAME(name, size) ZoneName(name, size)

    // Memory profiling - track allocations/deallocations
    #define V_PROFILE_ALLOC(ptr, size)        TracyAlloc(ptr, size)
    #define V_PROFILE_FREE(ptr)               TracyFree(ptr)
    #define V_PROFILE_SECURE_ALLOC(ptr, size) TracySecureAlloc(ptr, size)
    #define V_PROFILE_SECURE_FREE(ptr)        TracySecureFree(ptr)

    // Plots - continuous value graphs
    #define V_PROFILE_PLOT(name, value) TracyPlot(name, value)

    // Messages - single event markers
    #define V_PROFILE_MESSAGE(text, size)          TracyMessage(text, size)
    #define V_PROFILE_MESSAGE_L(text)              TracyMessageL(text)
    #define V_PROFILE_MESSAGE_C(text, size, color) TracyMessageC(text, size, color)
    #define V_PROFILE_MESSAGE_LC(text, color)      TracyMessageLC(text, color)

#else
    // tis a no-op
    #define V_PROFILE_ZONE
    #define V_PROFILE_ZONE_NAMED(name)
    #define V_PROFILE_ZONE_COLOR(color)
    #define V_PROFILE_ZONE_NAMED_COLOR(name, color)

    #define V_PROFILE_FRAME_MARK
    #define V_PROFILE_FRAME_MARK_NAMED(name)
    #define V_PROFILE_FRAME_MARK_START(name)
    #define V_PROFILE_FRAME_MARK_END(name)

    #define V_PROFILE_ZONE_TEXT(text, size)
    #define V_PROFILE_ZONE_VALUE(value)
    #define V_PROFILE_ZONE_NAME(name, size)

    #define V_PROFILE_ALLOC(ptr, size)
    #define V_PROFILE_FREE(ptr)
    #define V_PROFILE_SECURE_ALLOC(ptr, size)
    #define V_PROFILE_SECURE_FREE(ptr)

    #define V_PROFILE_PLOT(name, value)

    #define V_PROFILE_MESSAGE(text, size)
    #define V_PROFILE_MESSAGE_L(text)
    #define V_PROFILE_MESSAGE_C(text, size, color)
    #define V_PROFILE_MESSAGE_LC(text, color)

#endif
