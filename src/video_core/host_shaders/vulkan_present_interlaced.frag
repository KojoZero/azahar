// Copyright 2022 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 color;

layout (push_constant, std140) uniform DrawInfo {
    mat4 modelview_matrix;
    vec4 i_resolution;
    vec4 o_resolution;
    int screen_id_l;
    int screen_id_r;
    int layer;
    int reverse_interlaced;
    int linear_filter;
};

layout (set = 0, binding = 0) uniform sampler2D screen_textures[3];

// sRGB <-> linear transfer functions for gamma-correct linear filtering
float srgbToLinear(float c) {
    return c <= 0.04045 ? c * (1.0 / 12.92) : pow((c + 0.055) * (1.0 / 1.055), 2.4);
}

vec3 srgbToLinear(vec3 c) {
    return vec3(srgbToLinear(c.r), srgbToLinear(c.g), srgbToLinear(c.b));
}

float linearToSrgb(float c) {
    return c <= 0.0031308 ? c * 12.92 : 1.055 * pow(c, 1.0 / 2.4) - 0.055;
}

vec3 linearToSrgb(vec3 c) {
    return vec3(linearToSrgb(c.r), linearToSrgb(c.g), linearToSrgb(c.b));
}

vec4 FetchTexel(int screen_id, ivec2 coord) {
#ifdef ARRAY_DYNAMIC_INDEX
    return texelFetch(screen_textures[screen_id], coord, 0);
#else
    switch (screen_id) {
    case 0: return texelFetch(screen_textures[0], coord, 0);
    case 1: return texelFetch(screen_textures[1], coord, 0);
    case 2: return texelFetch(screen_textures[2], coord, 0);
    }
#endif
}

ivec2 GetTexSize(int screen_id) {
#ifdef ARRAY_DYNAMIC_INDEX
    return textureSize(screen_textures[screen_id], 0);
#else
    switch (screen_id) {
    case 0: return textureSize(screen_textures[0], 0);
    case 1: return textureSize(screen_textures[1], 0);
    case 2: return textureSize(screen_textures[2], 0);
    }
#endif
}

// Bilinear filtering with gamma-correct interpolation (sRGB -> linear -> blend -> sRGB)
vec4 GammaCorrectSample(int screen_id) {
    ivec2 size = GetTexSize(screen_id);
    vec2 texel_pos = frag_tex_coord * vec2(size) - 0.5;
    vec2 frac_part = fract(texel_pos);
    ivec2 base = ivec2(floor(texel_pos));
    ivec2 s = size - ivec2(1);

    vec4 c00 = FetchTexel(screen_id, clamp(base,                ivec2(0), s));
    vec4 c10 = FetchTexel(screen_id, clamp(base + ivec2(1, 0),  ivec2(0), s));
    vec4 c01 = FetchTexel(screen_id, clamp(base + ivec2(0, 1),  ivec2(0), s));
    vec4 c11 = FetchTexel(screen_id, clamp(base + ivec2(1, 1),  ivec2(0), s));

    c00.rgb = srgbToLinear(c00.rgb);
    c10.rgb = srgbToLinear(c10.rgb);
    c01.rgb = srgbToLinear(c01.rgb);
    c11.rgb = srgbToLinear(c11.rgb);

    vec4 result = mix(mix(c00, c10, frac_part.x), mix(c01, c11, frac_part.x), frac_part.y);
    result.rgb = linearToSrgb(result.rgb);
    return result;
}

vec4 GetScreen(int screen_id) {
    if (linear_filter != 0) {
        return GammaCorrectSample(screen_id);
    }
#ifdef ARRAY_DYNAMIC_INDEX
    return texture(screen_textures[screen_id], frag_tex_coord);
#else
    switch (screen_id) {
    case 0:
        return texture(screen_textures[0], frag_tex_coord);
    case 1:
        return texture(screen_textures[1], frag_tex_coord);
    case 2:
        return texture(screen_textures[2], frag_tex_coord);
    }
#endif
}

void main() {
    float screen_row = o_resolution.x * frag_tex_coord.x;
    if (int(screen_row) % 2 == reverse_interlaced)
        color = GetScreen(screen_id_l);
    else
        color = GetScreen(screen_id_r);
}
