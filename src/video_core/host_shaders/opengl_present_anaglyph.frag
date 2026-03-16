// Copyright 2023 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

//? #version 430 core

// Anaglyph Red-Cyan shader based on Dubois algorithm
// Constants taken from the paper:
// "Conversion of a Stereo Pair to Anaglyph with
// the Least-Squares Projection Method"
// Eric Dubois, March 2009
const mat3 l = mat3( 0.437, 0.449, 0.164,
              -0.062,-0.062,-0.024,
              -0.048,-0.050,-0.017);
const mat3 r = mat3(-0.011,-0.032,-0.007,
               0.377, 0.761, 0.009,
              -0.026,-0.093, 1.234);

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D color_texture;
layout(binding = 1) uniform sampler2D color_texture_r;

uniform vec4 resolution;
uniform int layer;
uniform int linear_filter;

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

// Bilinear filtering with gamma-correct interpolation (sRGB -> linear -> blend -> sRGB)
vec4 GammaCorrectSample(sampler2D tex, vec2 uv) {
    ivec2 size = textureSize(tex, 0);
    vec2 texel_pos = uv * vec2(size) - 0.5;
    vec2 frac_part = fract(texel_pos);
    ivec2 base = ivec2(floor(texel_pos));
    ivec2 s = size - ivec2(1);

    vec4 c00 = texelFetch(tex, clamp(base,                ivec2(0), s), 0);
    vec4 c10 = texelFetch(tex, clamp(base + ivec2(1, 0),  ivec2(0), s), 0);
    vec4 c01 = texelFetch(tex, clamp(base + ivec2(0, 1),  ivec2(0), s), 0);
    vec4 c11 = texelFetch(tex, clamp(base + ivec2(1, 1),  ivec2(0), s), 0);

    c00.rgb = srgbToLinear(c00.rgb);
    c10.rgb = srgbToLinear(c10.rgb);
    c01.rgb = srgbToLinear(c01.rgb);
    c11.rgb = srgbToLinear(c11.rgb);

    vec4 result = mix(mix(c00, c10, frac_part.x), mix(c01, c11, frac_part.x), frac_part.y);
    result.rgb = linearToSrgb(result.rgb);
    return result;
}

void main() {
    vec4 color_tex_l, color_tex_r;
    if (linear_filter != 0) {
        color_tex_l = GammaCorrectSample(color_texture, frag_tex_coord);
        color_tex_r = GammaCorrectSample(color_texture_r, frag_tex_coord);
    } else {
        color_tex_l = texture(color_texture, frag_tex_coord);
        color_tex_r = texture(color_texture_r, frag_tex_coord);
    }
    color = vec4(color_tex_l.rgb*l+color_tex_r.rgb*r, color_tex_l.a);
}
