// Basic cube renderer

#version 450
#extension GL_ARB_gpu_shader_int64: enable

#define CHUNK_SIZE 32
#define CHUNK_SIZE 32

const uint BLOCK_FACE_TOP = 0;    // Positive Y
const uint BLOCK_FACE_BOT = 1;    // Negative Y
const uint BLOCK_FACE_LEFT = 2;   // Positive X
const uint BLOCK_FACE_RIGHT = 3;  // Negative X
const uint BLOCK_FACE_FRONT = 4;  // Positive Z
const uint BLOCK_FACE_BACK = 5;   // Negative Z

layout (set = 0, binding = 0) uniform VertexUniformBuffer {
    mat4 view_projection;
} ubo;

layout (push_constant) uniform PushConstants {
    ivec3 chunk_coord;
} pc;

layout (location = 0) in uint x;
layout (location = 1) in uint y;
layout (location = 2) in uint z;
layout (location = 3) in uint face_dir;
layout (location = 4) in uint block_type;
layout (location = 5) in uint u;
layout (location = 6) in uint v;

layout (location = 0) out vec4 out_position;
layout (location = 1) out vec2 out_tex_coord;
layout (location = 2) out uint out_block_type;
layout (location = 3) out uint out_face_dir;

void main() {
    vec3 in_pos = vec3(float(x), float(y), float(z));
    vec3 transformed_pos = in_pos;
    transformed_pos.xz += (pc.chunk_coord.xz * CHUNK_SIZE);
    transformed_pos.y += (pc.chunk_coord.y * CHUNK_SIZE);

    out_tex_coord = vec2(float(u), float(v));

    out_block_type = block_type;
    out_face_dir = face_dir;

    gl_Position = ubo.view_projection * vec4(transformed_pos, 1);
}