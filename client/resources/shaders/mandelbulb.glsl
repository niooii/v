#include <shared.inl>

struct MandelbulbPush
{
    daxa_ImageViewId image_id;
    daxa_u32vec2 frame_dim;
    daxa_f32mat4x4 inv_view_proj;
    daxa_f32vec3 camera_pos;
    daxa_f32 time;
};

DAXA_DECL_PUSH_CONSTANT(MandelbulbPush, push)

#define MAX_STEPS 512
#define MAX_DIST 100.0
#define SURF_DIST 0.0001

// Mandelbulb distance estimator
float mandelbulb_de(vec3 pos)
{
    vec3 z = pos;
    float dr = 1.0;
    float r = 0.0;
    float POWER = sin(push.time / 10) * 4 + 6;

    for (int i = 0; i < 15; i++)
    {
        r = length(z);

        if (r > 2.0)
            break;

        // Convert to polar coordinates
        float theta = acos(z.z / r);
        float phi = atan(z.y, z.x);
        dr = pow(r, POWER - 1.0) * POWER * dr + 1.0;

        // Scale and rotate the point
        float zr = pow(r, POWER);
        theta = theta * POWER;
        phi = phi * POWER;

        // Convert back to cartesian coordinates
        z = zr * vec3(
            sin(theta) * cos(phi),
            sin(phi) * sin(theta),
            cos(theta)
        );
        z += pos;
    }

    return 0.5 * log(r) * r / dr;
}

// Raymarch the scene
float raymarch(vec3 ro, vec3 rd)
{
    float dO = 0.0;

    for (int i = 0; i < MAX_STEPS; i++)
    {
        vec3 p = ro + rd * dO;
        float dS = mandelbulb_de(p);
        dO += dS;

        if (dO > MAX_DIST || abs(dS) < SURF_DIST)
            break;
    }

    return dO;
}

// Calculate normal using gradient
vec3 get_normal(vec3 p)
{
    float d = mandelbulb_de(p);
    vec2 e = vec2(0.001, 0.0);

    vec3 n = d - vec3(
        mandelbulb_de(p - e.xyy),
        mandelbulb_de(p - e.yxy),
        mandelbulb_de(p - e.yyx)
    );

    return normalize(n);
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    uvec3 pixel_i = gl_GlobalInvocationID.xyz;
    if (pixel_i.x >= push.frame_dim.x || pixel_i.y >= push.frame_dim.y)
        return;

    // Calculate UV coordinates [-1, 1]
    vec2 uv = (vec2(pixel_i.xy) / vec2(push.frame_dim.xy)) * 2.0 - 1.0;

    // Generate ray from camera using inverse projection
    vec4 target = push.inv_view_proj * vec4(uv.x, uv.y, 1.0, 1.0);
    vec3 ray_dir = normalize(target.xyz / target.w - push.camera_pos);

    // Raymarch the scene
    float d = raymarch(push.camera_pos, ray_dir);

    vec3 col = vec3(0.0);

    if (d < MAX_DIST)
    {
        // Hit! Calculate lighting
        vec3 p = push.camera_pos + ray_dir * d;
        vec3 normal = get_normal(p);

        // Simple directional light
        vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
        float diff = max(dot(normal, light_dir), 0.0);

        // Ambient occlusion approximation
        float ao = 1.0 - float(d) / MAX_DIST;

        // Monochrome scheme - convert normal to grayscale
        float gray = length(normal * 0.5 + 0.5) / sqrt(3.0);
        gray = pow(gray, 1.5);
        vec3 base_color = vec3(gray);
        col = base_color * (diff * 0.8 + 0.2) * ao;
    }
    else
    {
        // Background - pure black
        col = vec3(0.0);
    }

    imageStore(daxa_image2D(push.image_id), ivec2(pixel_i.xy), vec4(col, 1.0));
}
