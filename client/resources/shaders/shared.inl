#pragma once

#include <daxa/daxa.inl>

struct MandelbulbPush
{
    daxa_ImageViewId image_id;
    daxa_u32vec2 frame_dim;
    daxa_f32mat4x4 inv_view_proj;
    daxa_f32vec3 camera_pos;
    daxa_f32 time;
};

// SVO def
//
