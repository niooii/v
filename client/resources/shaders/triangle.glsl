#include <shared.inl>

#if defined(VERTEX_SHADER)

layout(location = 0) out vec3 v_color;

struct PushConstants {
    vec2 mouse_pos;
};

DAXA_DECL_PUSH_CONSTANT(PushConstants, pc);

void main() {
    // Hardcoded triangle vertices (NDC space)
    const vec2 positions[3] = vec2[](
        vec2( 0.0, -0.5),  // Bottom
        vec2( 0.5,  0.5),  // Top right
        vec2(-0.5,  0.5)   // Top left
    );

    // Rainbow colors
    const vec3 colors[3] = vec3[](
        vec3(1.0, 0.0, 0.0),  // Red
        vec3(0.0, 1.0, 0.0),  // Green
        vec3(0.0, 0.0, 1.0)   // Blue
    );

    gl_Position = vec4(positions[gl_VertexIndex] + sin(pc.mouse_pos.x + pc.mouse_pos.y)/10.f, 0.0, 1.0);
    v_color = colors[gl_VertexIndex];
}

#elif defined(FRAGMENT_SHADER)

layout(location = 0) in vec3 v_color;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(0.0);
}

#endif
