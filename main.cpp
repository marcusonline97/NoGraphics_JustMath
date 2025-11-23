#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cmath>
#include <algorithm>

struct vec4 {
    float x, y, z, w;
    vec4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {}
};

struct vec2 {
    float x, y;
    vec2(float x = 0, float y = 0) : x(x), y(y) {}
    vec2 yx() const { return vec2(y, x); }
    vec4 xyyx() const { return vec4(x, y, y, x); }
};

vec2 operator *(const vec2& a, float s) { return vec2(a.x * s, a.y * s); }
vec2 operator +(const vec2& a, float s) { return vec2(a.x + s, a.y + s); }
vec2 operator *(float s, const vec2& a) { return vec2(a.x * s, a.y * s); }
vec2 operator -(const vec2& a, const vec2& b) { return vec2(a.x - b.x, a.y - b.y); }
vec2 operator +(const vec2& a, const vec2& b) { return vec2(a.x + b.x, a.y + b.y); }
vec2 operator *(const vec2& a, const vec2& b) { return vec2(a.x * b.x, a.y * b.y); }
vec2 operator /(const vec2& a, float s) { return vec2(a.x / s, a.y / s); }
float dot(const vec2& a, const vec2& b) { return a.x * b.x + a.y * b.y; }
vec2 abs(const vec2& a) { return vec2(std::fabs(a.x), std::fabs(a.y)); }
vec2& operator +=(vec2& a, const vec2& b) { a = a + b; return a; }
vec2& operator +=(vec2& a, float s) { a = a + s; return a; }
vec2 cos(const vec2& a) { return vec2(std::cos(a.x), std::cos(a.y)); }

vec4 sin(const vec4& a) { return vec4(std::sin(a.x), std::sin(a.y), std::sin(a.z), std::sin(a.w)); }
vec4 exp(const vec4& a) { return vec4(std::exp(a.x), std::exp(a.y), std::exp(a.z), std::exp(a.w)); }
vec4 tanh(const vec4& a) { return vec4(std::tanh(a.x), std::tanh(a.y), std::tanh(a.z), std::tanh(a.w)); }
vec4 operator +(const vec4& a, float s) { return vec4(a.x + s, a.y + s, a.z + s, a.w + s); }
vec4 operator *(const vec4& a, float s) { return vec4(a.x * s, a.y * s, a.z * s, a.w * s); }
vec4 operator *(float s, const vec4& a) { return vec4(a.x * s, a.y * s, a.z * s, a.w * s); }
vec4 operator +(const vec4& a, const vec4& b) { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
vec4& operator +=(vec4& a, const vec4& b) { a = a + b; return a; }
vec4 operator -(float s, const vec4& a) { return vec4(s - a.x, s - a.y, s - a.z, s - a.w); }
vec4 operator /(const vec4& a, const vec4& b) { return vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

static inline unsigned char to_u8(float v) {
    v = std::max(0.0f, std::min(1.0f, v));
    return static_cast<unsigned char>(v * 255.0f);
}

int main()
{
    char buf[256];
    const int w = 16 * 60;
    const int h = 9 * 60;
    vec2 r = { static_cast<float>(w), static_cast<float>(h) };

    for (int frame = 0; frame < 240; ++frame) {
        std::snprintf(buf, sizeof(buf), "output-%03d.ppm", frame);
        const char* output_path = buf;
        FILE* f = std::fopen(output_path, "wb");
        if (!f) {
            std::perror("fopen");
            return 1;
        }
        std::fprintf(f, "P6\n");
        std::fprintf(f, "%d %d\n", w, h);
        std::fprintf(f, "255\n");

        float t = static_cast<float>(frame) / 60.0f; // time

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                vec2 FC = { static_cast<float>(x), static_cast<float>(y) };
                vec2 p = (FC * 2.0f - r) / r.y;

                vec2 l = vec2(0.0f, 0.0f);
                vec2 ii = vec2(0.0f, 0.0f);
                vec2 v = p * (l += (4.0f - 4.0f * std::fabs(0.7f - dot(p, p))));

                vec4 o = vec4(0, 0, 0, 0);
                for (; ii.y < 8.0f; ii.y += 1.0f) {
                    // o += (sin(v.xyyx()) + 1.) * abs(v.x - v.y);
                    vec4 sx = sin(v.xyyx());
                    vec4 term = sx + 1.0f;
                    float a = std::fabs(v.x - v.y);
                    o += vec4(term.x * a, term.y * a, term.z * a, term.w * a);

                    // v += cos(v.yx() * ii.y + ii + t) / ii.y + .7;
                    vec2 add = cos(v.yx() * ii.y + ii + t);
                    if (ii.y == 0.0f) {
                        // avoid divide-by-zero on first iteration: treat as large contribution
                        v += add + 0.7f;
                    }
                    else {
                        v += (add / ii.y) + 0.7f;
                    }
                }

                // o = tanh(5.*exp(l.x - 4. - p.y * vec4(-1, 1, 2, 0)) / o);
                vec4 A = vec4(
                    l.x - 4.0f - p.y * (-1.0f),
                    l.x - 4.0f - p.y * (1.0f),
                    l.x - 4.0f - p.y * (2.0f),
                    l.x - 4.0f - p.y * (0.0f)
                );
                vec4 E = exp(A);
                vec4 o_safe = o + 1e-6f; // avoid div-by-zero
                vec4 res = tanh(5.0f * (E / o_safe));

                std::fputc(to_u8(res.x), f);
                std::fputc(to_u8(res.y), f);
                std::fputc(to_u8(res.z), f);
            }
        }
        std::fclose(f);
        std::printf("Generated %s (%3d/%3d)\n", output_path, frame + 1, 240);
    }
    return 0;
}
