/* Just enough GLSL for the panvk BC decoder shaders to run on the host as
 * C++20 (see glsl2cpp.py). Integer semantics follow GLSL (wrapping, build
 * with -fwrapv). Not a general GLSL implementation. */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <type_traits>

typedef unsigned int uint;

template <typename T, int N> struct vec_t {
   T v[N];

   vec_t() : v{} {}
   template <typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
   explicit vec_t(S s)
   {
      for (int i = 0; i < N; i++)
         v[i] = T(s);
   }
   template <typename... S, typename = std::enable_if_t<sizeof...(S) == N && N >= 2>>
   vec_t(S... s) : v{T(s)...} {}
   template <typename U> vec_t(const vec_t<U, N> &o)
   {
      for (int i = 0; i < N; i++)
         v[i] = T(o.v[i]);
   }
   template <typename U, typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
   vec_t(const vec_t<U, N - 1> &o, S s)
   {
      for (int i = 0; i < N - 1; i++)
         v[i] = T(o.v[i]);
      v[N - 1] = T(s);
   }

   T &operator[](int i) { return v[i]; }
   const T &operator[](int i) const { return v[i]; }
   T &operator[](uint i) { return v[i]; }
   const T &operator[](uint i) const { return v[i]; }

   /* Swizzles (glsl2cpp.py rewrites .rgb to .rgb_(), .agbr to
    * .sw_(3, 1, 2, 0) and single components to [i]). */
   vec_t<T, 3> rgb_() const { return vec_t<T, 3>(v[0], v[1], v[2]); }
   vec_t<T, 2> zw_() const { return vec_t<T, 2>(v[2], v[3]); }
   vec_t<T, 4> sw_(int i, int j, int k, int l) const
   {
      return vec_t<T, 4>(v[i], v[j], v[k], v[l]);
   }
};

typedef vec_t<int, 2> ivec2;
typedef vec_t<int, 3> ivec3;
typedef vec_t<int, 4> ivec4;
typedef vec_t<uint, 2> uvec2;
typedef vec_t<uint, 3> uvec3;
typedef vec_t<uint, 4> uvec4;
typedef vec_t<float, 2> vec2;
typedef vec_t<float, 3> vec3;
typedef vec_t<float, 4> vec4;
typedef vec_t<bool, 3> bvec3;

#define VOP(op)                                                                \
   template <typename T, int N>                                               \
   vec_t<T, N> operator op(const vec_t<T, N> &a, const vec_t<T, N> &b)        \
   {                                                                          \
      vec_t<T, N> r;                                                          \
      for (int i = 0; i < N; i++)                                             \
         r.v[i] = a.v[i] op b.v[i];                                           \
      return r;                                                               \
   }                                                                          \
   template <typename T, int N, typename S,                                   \
             typename = std::enable_if_t<std::is_arithmetic_v<S>>>            \
   vec_t<T, N> operator op(const vec_t<T, N> &a, S b)                         \
   {                                                                          \
      vec_t<T, N> r;                                                          \
      for (int i = 0; i < N; i++)                                             \
         r.v[i] = a.v[i] op T(b);                                             \
      return r;                                                               \
   }                                                                          \
   template <typename T, int N, typename S,                                   \
             typename = std::enable_if_t<std::is_arithmetic_v<S>>>            \
   vec_t<T, N> operator op(S a, const vec_t<T, N> &b)                         \
   {                                                                          \
      vec_t<T, N> r;                                                          \
      for (int i = 0; i < N; i++)                                             \
         r.v[i] = T(a) op b.v[i];                                             \
      return r;                                                               \
   }
VOP(+) VOP(-) VOP(*) VOP(/) VOP(%) VOP(<<) VOP(>>) VOP(|) VOP(&) VOP(^)
#undef VOP

template <typename T, int N> vec_t<T, N> operator-(const vec_t<T, N> &a)
{
   vec_t<T, N> r;
   for (int i = 0; i < N; i++)
      r.v[i] = -a.v[i];
   return r;
}

#define VASSIGN(op)                                                            \
   template <typename T, int N, typename B>                                   \
   vec_t<T, N> &operator op##=(vec_t<T, N> &a, const B &b)                    \
   {                                                                          \
      a = a op b;                                                             \
      return a;                                                               \
   }
VASSIGN(+) VASSIGN(-) VASSIGN(*) VASSIGN(<<) VASSIGN(>>) VASSIGN(|) VASSIGN(&)
#undef VASSIGN

#define VCMP(name, op)                                                         \
   template <typename T, int N>                                               \
   vec_t<bool, N> name(const vec_t<T, N> &a, const vec_t<T, N> &b)            \
   {                                                                          \
      vec_t<bool, N> r;                                                       \
      for (int i = 0; i < N; i++)                                             \
         r.v[i] = a.v[i] op b.v[i];                                           \
      return r;                                                               \
   }
VCMP(lessThan, <) VCMP(equal, ==) VCMP(greaterThanEqual, >=)
VCMP(greaterThan, >) VCMP(notEqual, !=)
#undef VCMP

template <typename T, int N>
vec_t<T, N> mix(const vec_t<T, N> &a, const vec_t<T, N> &b, const vec_t<bool, N> &s)
{
   vec_t<T, N> r;
   for (int i = 0; i < N; i++)
      r.v[i] = s.v[i] ? b.v[i] : a.v[i];
   return r;
}
template <typename T, int N> vec_t<T, N> abs(const vec_t<T, N> &a)
{
   vec_t<T, N> r;
   for (int i = 0; i < N; i++)
      r.v[i] = a.v[i] < 0 ? -a.v[i] : a.v[i];
   return r;
}

static inline int min(int a, int b) { return a < b ? a : b; }
static inline uint min(uint a, uint b) { return a < b ? a : b; }
static inline int max(int a, int b) { return a > b ? a : b; }
static inline uint max(uint a, uint b) { return a > b ? a : b; }
static inline float min(float a, float b) { return a < b ? a : b; }
static inline float max(float a, float b) { return a > b ? a : b; }
static inline int clamp(int x, int a, int b) { return min(max(x, a), b); }

static inline uint bitfieldExtract(uint v, int off, int bits)
{
   if (bits == 0)
      return 0;
   return (v >> off) & (bits == 32 ? ~0u : ((1u << bits) - 1u));
}
static inline int bitfieldExtract(int v, int off, int bits)
{
   if (bits == 0)
      return 0;
   return int(uint(v) << (32 - off - bits)) >> (32 - bits);
}
template <typename T, int N>
vec_t<T, N> bitfieldExtract(const vec_t<T, N> &a, int off, int bits)
{
   vec_t<T, N> r;
   for (int i = 0; i < N; i++)
      r.v[i] = bitfieldExtract(a.v[i], off, bits);
   return r;
}
static inline uint bitfieldReverse(uint v)
{
   uint r = 0;
   for (int i = 0; i < 32; i++)
      r |= ((v >> i) & 1u) << (31 - i);
   return r;
}
static inline int bitfieldReverse(int v) { return int(bitfieldReverse(uint(v))); }
static inline int findLSB(uint v) { return v ? __builtin_ctz(v) : -1; }
static inline int findLSB(int v) { return findLSB(uint(v)); }
static inline int findMSB(uint v) { return v ? 31 - __builtin_clz(v) : -1; }
static inline int bitCount(uint v) { return __builtin_popcount(v); }

static inline void umulExtended(uint a, uint b, uint &hi, uint &lo)
{
   uint64_t p = (uint64_t)a * b;
   hi = uint(p >> 32);
   lo = uint(p);
}
static inline uint uaddCarry(uint a, uint b, uint &c)
{
   uint64_t s = (uint64_t)a + b;
   c = uint(s >> 32);
   return uint(s);
}

static inline uint packUnorm4x8(const vec4 &c)
{
   uint r = 0;
   for (int i = 0; i < 4; i++) {
      float f = c.v[i] < 0 ? 0 : c.v[i] > 1 ? 1 : c.v[i];
      r |= uint(std::nearbyint(f * 255.0f)) << (8 * i);
   }
   return r;
}

/* buffer_reference blocks: a 64-bit host address as uvec2. */
template <typename T> struct bufref {
   T *data;
   explicit bufref(const uvec2 &a)
      : data((T *)(uintptr_t)(((uint64_t)a.v[1] << 32) | a.v[0]))
   {
   }
};
static inline uvec2 host_addr(const void *p)
{
   uint64_t a = (uint64_t)(uintptr_t)p;
   return uvec2(uint(a), uint(a >> 32));
}

extern uvec3 gl_GlobalInvocationID;
