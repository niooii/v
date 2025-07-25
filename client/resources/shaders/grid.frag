#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_world_pos;

layout(location = 1) in vec3 camera_pos;

float ease_inout_exp(float x) {
    return x <= 0.0 ? 0.0 : pow(2.0, 10.0 * (x - 1.0));
}

const vec3 grid_col = vec3(0.9);
const float thickness = 0.02;
const float fade_distance = 32;

void main() {
    vec3 distVec = in_world_pos - camera_pos;
    float distance = sqrt(dot(distVec, distVec));
    if (distance > fade_distance) {
         out_color = vec4(0.0);
         return;
    }
    float decimal_part_x = fract(in_world_pos.x);
    float decimal_part_y = fract(in_world_pos.z);
    if ((decimal_part_x < thickness || decimal_part_x > (1-thickness))
        || (decimal_part_y < thickness || decimal_part_y > (1-thickness)))
    {
        float dist = length(in_world_pos - camera_pos);
    
        float fade_factor = max(0.0, 1.0 - dist / fade_distance);
        fade_factor = ease_inout_exp(fade_factor);
        out_color = vec4(grid_col * fade_factor, 0.5);
    }
    else
    {
        out_color = vec4(0.0);
    }
}