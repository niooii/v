#version 450

const uint BLOCK_FACE_TOP = 0;    // Positive Y
const uint BLOCK_FACE_BOT = 1;    // Negative Y
const uint BLOCK_FACE_LEFT = 2;   // Positive X
const uint BLOCK_FACE_RIGHT = 3;  // Negative X
const uint BLOCK_FACE_FRONT = 4;  // Positive Z
const uint BLOCK_FACE_BACK = 5;   // Negative Z

struct StaticBlockData {
    uint textures[6];
};

layout(set = 1, binding = 0) uniform sampler2DArray block_textures;
layout(set = 1, binding = 1) readonly buffer StaticBlockDataBuffer {
    StaticBlockData table[];
} static_block_data;

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) flat in uint block_type;
layout(location = 3) flat in uint face_dir;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(block_textures, vec3(in_tex_coord, static_block_data.table[block_type].textures[face_dir]));
    // out_color = vec4(0.8);
}