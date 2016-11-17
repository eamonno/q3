//-----------------------------------------------------------------------------
// File: maths.h
//
// Maths library, contains vector, matrix and general purpose maths functions
// and classes
//-----------------------------------------------------------------------------

#ifndef MATHS_H
#define MATHS_H

#include "types.h"
#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846f
#endif
#ifndef M_2PI
#define M_2PI		(M_PI * 2.0f)
#endif

#define M_DEG2RAD	(M_PI / 180.0f)
#define M_RAD2DEG	(180.0f / M_PI)

inline float m_deg2rad(float f)
	{ return f * M_DEG2RAD; }

inline float m_rad2deg(float f)
	{ return f * M_RAD2DEG; }

inline float m_sqrt(float f)
	{ return (float)sqrt(f); }

inline int m_ftoi(float f)
	{ return (int)f; }

inline float m_itof(int i)
	{ return (float)i; }

inline float m_lltof(__int64 ll)
	{ return (float)ll; }

inline float m_floor(float f)
	{ return (float)floor(f); }

inline void m_sincos(const float angle, float& s, float& c)
	{ s = (float)sin(angle); c = (float)cos(angle); }

inline float m_tan(const float angle)
	{ return (float)tan(angle); }

inline float m_sin(const float angle)
	{ return (float)sin(angle); }

inline float m_cos(const float angle)
	{ return (float)cos(angle); }

inline float m_clamp(const float value)
	{ return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); }

inline float m_sin_wave(const float pos)
	{ return m_sin(pos * M_2PI); }

inline float m_triangle_wave(const float pos)
	{ return ((pos - m_floor(pos)) < 0.5f) ? pos * 2.0f : 2.0f - pos * 2.0f; }

inline float m_square_wave(const float pos)
	{ return ((pos - m_floor(pos)) < 0.5f) ? -1.0f : 1.0f; }

inline float m_sawtooth_wave(const float pos)
	{ return (pos - m_floor(pos)); }

inline float m_inversesawtooth_wave(const float pos)
	{ return 1.0f - (pos - m_floor(pos)); }


class color_t {
	// ARGB color packed into 4 bytes
public:
	static const color_t identity;
	static const color_t black;
	static const color_t white;
	static const color_t red;
	static const color_t green;
	static const color_t blue;
	static const color_t magenta;
	static const color_t yellow;
	static const color_t cyan;

	union {
		struct {
			ubyte b;
			ubyte g;
			ubyte r;
			ubyte a;
		};
		uint c;
	};
	color_t(uint color = 0) : c(color) {}
	operator uint() const { return c; }
	color_t& operator=(uint color) { c = color; return *this; }
	color_t& operator=(const color_t& color) { c = color.c; return *this; }

	void set_a(ubyte aa)	{ a = aa; }
	void set_r(ubyte rr)	{ r = rr; }
	void set_g(ubyte gg)	{ g = gg; }
	void set_b(ubyte bb)	{ b = bb; }

	void set_a(float aa)	{ a = m_ftoi(m_clamp(aa) * 255.0f); }
	void set_r(float rr)	{ r = m_ftoi(m_clamp(rr) * 255.0f); }
	void set_g(float gg)	{ g = m_ftoi(m_clamp(gg) * 255.0f); }
	void set_b(float bb)	{ b = m_ftoi(m_clamp(bb) * 255.0f); }
};



class vec2_t {
	// 2 dimensional vector
public:
	static const vec2_t origin;

	union {
		struct {
			float x;
			float y;
		};
		struct {
			float u;
			float v;
		};
		float array[2];
	};

	// Constructors
	vec2_t() {}
	vec2_t(const float xx, const float yy) : x(xx), y(yy) {}
	vec2_t(const vec2_t& v) : x(v.x), y(v.y) {}

	// Assignment
	vec2_t& operator=(const vec2_t& v)   { x = v.x; y = v.y; return *this; }

	// Array subscripting
	float& operator[](const int pos)             { return array[pos]; }
	const float& operator[](const int pos) const { return array[pos]; }
	// Cast to an array
	operator float*()             { return array; };
	operator const float*() const { return array; };

	// Comparison
	bool operator==(const vec2_t& v) const { return (x == v.x && y == v.y); }
	bool operator!=(const vec2_t& v) const { return (x != v.x || y != v.y); }

	// Scaler multiplication/addition
	vec2_t& operator*=(const float f) { x *= f; y *= f; return *this; }
	vec2_t& operator/=(const float f) { return operator*=(1.0f / f); }

	// 2d vector maths
	vec2_t& operator+=(const vec2_t& v) { x += v.x; y += v.y; return *this; }
	vec2_t& operator-=(const vec2_t& v) { x -= v.x; y -= v.y; return *this; }

	// Length and normalization
	float length() const { return m_sqrt(x * x + y * y); }
	vec2_t& normalize() { return operator/=(length()); }

	// Set the position
	vec2_t& set(const float xx, const float yy) { x = xx; y = yy; return *this; }
};

// 2d vector operators
inline vec2_t operator-(const vec2_t& v)
	{ return vec2_t(-v.x, -v.y); }
inline vec2_t operator*(const vec2_t& v, const float f)
	{ return vec2_t(v.x * f, v.y * f); }
inline vec2_t operator*(const float f, const vec2_t& v)
	{ return v * f; }
inline vec2_t operator/(const vec2_t& v, const float f)
	{ return v * (1.0f / f); }
inline vec2_t operator+(const vec2_t& a, const vec2_t& b)
	{ return vec2_t(a.x + b.x, a.y + b.y); }
inline vec2_t operator-(const vec2_t& a, const vec2_t& b)
	{ return vec2_t(a.x - b.x, a.y - b.y); }

// 2d vector functions
inline float dot(const vec2_t& a, const vec2_t& b)
	{ return a.x * b.x + a.y * b.y; }
inline vec2_t normalize(const vec2_t& v)
	{ return v / v.length(); }




class vec3_t {
	// 3 dimensional vector
public:
	static const vec3_t origin;

	union {
		struct {
			float x;
			float y;
			float z;
		};
		float array[3];
	};

	// Constructors
	vec3_t() {}
	vec3_t(const float xx, const float yy, const float zz) : x(xx), y(yy), z(zz) {}
	vec3_t(const vec3_t& v) : x(v.x), y(v.y), z(v.z) {}

	// Assignment
	vec3_t& operator=(const vec3_t& v)   { x = v.x; y = v.y; z = v.z; return *this; }

	// Array subscripting
	float& operator[](const int pos)             { return array[pos]; }
	const float& operator[](const int pos) const { return array[pos]; }

	// Cast to an array
	operator float*()             { return array; };
	operator const float*() const { return array; };

	// Comparison
	bool operator==(const vec3_t& v) const { return (x == v.x && y == v.y && z == v.z); }
	bool operator!=(const vec3_t& v) const { return (x != v.x || y != v.y || z != v.z); }

	// Scaler multiplication/addition
	vec3_t& operator*=(const float f)  { x *= f; y *= f; z *= f; return *this; }
	vec3_t& operator/=(const float f)  { return operator*=(1.0f / f); }

	// 3d vector maths
	vec3_t& operator+=(const vec3_t& v)  { x += v.x; y += v.y; z += v.z; return *this; }
	vec3_t& operator-=(const vec3_t& v)  { x -= v.x; y -= v.y; z -= v.z; return *this; }

	// Length and normalization
	float length() const { return m_sqrt(x * x + y * y + z * z); }
	vec3_t& normalize() { return operator/=(length()); }

	// Set the position
	vec3_t& set(const float xx, const float yy, const float zz) { x = xx; y = yy; z = zz; return *this; }
};

// 3d vector operators
inline vec3_t operator-(const vec3_t& v)
	{ return vec3_t(-v.x, -v.y, -v.z); }
inline vec3_t operator*(const vec3_t& v, const float f)
	{ return vec3_t(v.x * f, v.y * f, v.z * f); }
inline vec3_t operator*(const float f, const vec3_t& v)
	{ return v * f; }
inline vec3_t operator/(const vec3_t& v, const float f)
	{ return v * (1.0f / f); }
inline vec3_t operator+(const vec3_t& a, const vec3_t& b)
	{ return vec3_t(a.x + b.x, a.y + b.y, a.z + b.z); }
inline vec3_t operator-(const vec3_t& a, const vec3_t& b)
	{ return vec3_t(a.x - b.x, a.y - b.y, a.z - b.z); }

// 3d vector functions
inline float dot(const vec3_t& a, const vec3_t& b)
	{ return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3_t normalize(const vec3_t& v)
	{ return v / v.length(); }
inline vec3_t cross(const vec3_t& a, const vec3_t& b)
	{ return vec3_t(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }




class vec4_t {
	// 4 dimensional vector
public:
	static const vec4_t origin;

	union {
		struct { 
			float x;
			float y;
			float z;
			float w;
		};
		float array[4];
	};

	// Constructors
	vec4_t() {}
	vec4_t(const float xx, const float yy, const float zz, const float ww) : x(xx), y(yy), z(zz), w(ww) {}
	vec4_t(const vec4_t& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

	// Assignment
	vec4_t& operator=(const vec4_t& v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }

	// Array subscripting
	float& operator[](const int pos)             { return array[pos]; }
	const float& operator[](const int pos) const { return array[pos]; }

	// Cast to an array
	operator float*() { return array; };
	operator const float*() const { return array; };

	// Comparison
	bool operator==(const vec4_t& v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); }
	bool operator!=(const vec4_t& v) const { return (x != v.x || y != v.y || z != v.z || w != v.w); }

	// Scaler multiplication/addition
	vec4_t& operator*=(const float f) { x *= f; y *= f; z *= f; w *= f; return *this; }
	vec4_t& operator/=(const float f) { return operator*=(1.0f / f); }

	// 4d vector maths
	vec4_t& operator+=(const vec4_t& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	vec4_t& operator-=(const vec4_t& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }

	// Length and normalization
	float length() const { return m_sqrt(x * x + y * y + z * z + w * w); }
	vec4_t& normalize() { return operator/=(length()); }

	// Set the position
	vec4_t& set(const float xx, const float yy, const float zz, const float ww) { x = xx; y = yy; z = zz; w = ww; return *this; }
};

// 4d vector operators
inline vec4_t operator-(const vec4_t& v)
	{ return vec4_t(-v.x, -v.y, -v.z, -v.w); }
inline vec4_t operator*(const vec4_t& v, const float f)
	{ return vec4_t(v.x * f, v.y * f, v.z * f, v.w * f); }
inline vec4_t operator*(const float f, const vec4_t& v)
	{ return v * f; }
inline vec4_t operator/(const vec4_t& v, const float f)
	{ return v * (1.0f / f); }
inline vec4_t operator+(const vec4_t& a, const vec4_t& b)
	{ return vec4_t(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
inline vec4_t operator-(const vec4_t& a, const vec4_t& b)
	{ return vec4_t(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }

// 4d vector functions
inline float dot(const vec4_t& a, const vec4_t& b)
	{ return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline vec4_t normalize(const vec4_t& v)
	{ return v / v.length(); }



class matrix_t {
	// A 4x4 matrix
public:
	static const matrix_t identity;

	union {
		struct {
			float _00, _01, _02, _03;
			float _10, _11, _12, _13;
			float _20, _21, _22, _23;
			float _30, _31, _32, _33;
		};
		float array[16];
	};

	// Constructors
	matrix_t() {}
	matrix_t(const float f00, const float f01, const float f02, const float f03,
			 const float f10, const float f11, const float f12, const float f13,
			 const float f20, const float f21, const float f22, const float f23,
			 const float f30, const float f31, const float f32, const float f33) :
		_00(f00), _01(f01), _02(f02), _03(f03), _10(f10), _11(f11), _12(f12), _13(f13),
		_20(f20), _21(f21), _22(f22), _23(f23), _30(f30), _31(f31), _32(f32), _33(f33) {}

	// Assignment
	matrix_t& operator=(const matrix_t& m);

	// Array subscripting
	vec4_t& operator[](const int row)             { return *(reinterpret_cast<vec4_t*>(array + row * 4)); }
	const vec4_t& operator[](const int row) const { return *(reinterpret_cast<const vec4_t*>(array + row * 4)); }

	// Cast to an array
	operator float*()             { return array; };
	operator const float*() const { return array; };

	// Comparison
	bool operator==(const matrix_t& m) const;
	bool operator!=(const matrix_t& m) const;

	// Scaler multiplication, division
	matrix_t& operator*=(const float f);
	matrix_t& operator/=(const float f)  { return operator/=(1.0f / f); }
	
	// Matrix addition and subtraction
	matrix_t& operator+=(const matrix_t& m);
	matrix_t& operator-=(const matrix_t& m);
	matrix_t& operator*=(const matrix_t& m);

	// Assign the matrix to perform a given transform
	void look_at_rh(const vec3_t& eye, const vec3_t& at, const vec3_t& up);
	void ortho_rh(float l, float r, float b, float t, float zn, float zf);
	void perspective_fov_rh(const float fov, const float aspect, const float n, const float f);
	void perspective_rh(const float l, const float r, const float b, const float t, const float n, const float f);
	void rotation(const vec3_t& axis, float angle);
	void rotation_x(const float x);
	void rotation_y(const float y);
	void rotation_z(const float z);
	void scale(const float x, const float y, const float z);
	void scale(const vec3_t& v) { scale(v.x, v.y, v.z); }
	void scale_x(const float x) { scale(x, 1.0f, 1.0f); }
	void scale_y(const float y) { scale(1.0f, y, 1.0f); }
	void scale_z(const float z) { scale(1.0f, 1.0f, z); }
	void translation(const float x, const float y, const float z);
	void translation(const vec3_t& dir) { translation(dir.x, dir.y, dir.z); }
	void translation_x(const float x) { translation(x, 0.0f, 0.0f); }
	void translation_y(const float y) { translation(0.0f, y, 0.0f); }
	void translation_z(const float z) { translation(0.0f, 0.0f, z); }
	void transpose();

	// Get the determinant
	float determinant() const;

	// Transforms using the matrix
	vec3_t transform(const vec3_t& v) const;
	vec4_t transform(const vec4_t& v) const;
	vec3_t transform_point(const vec3_t& v) const;
	vec3_t transform_dir(const vec3_t& v) const;
};

// Additional matrix operations
matrix_t operator-(const matrix_t& m);
matrix_t operator*(const matrix_t& m, const float f);
matrix_t operator+(const matrix_t& a, const matrix_t& b);
matrix_t operator-(const matrix_t& a, const matrix_t& b);
matrix_t operator*(const matrix_t& a, const matrix_t& b);

inline vec3_t operator*(const matrix_t& m, const vec3_t& v) { return m.transform(v); }
inline vec4_t operator*(const matrix_t& m, const vec4_t& v) { return m.transform(v); }
inline matrix_t operator*(const float f, const matrix_t& m) { return m * f; }
inline matrix_t operator/(const matrix_t& m, const float f) { return m * (1.0f / f); }
inline matrix_t& matrix_t::operator*=(const matrix_t& m)
	{	return (*this = *this * m);	}

bool invert(matrix_t src, matrix_t& dst);

class plane_t {
	// 3 dimensional plane
public:
	plane_t() {}
	plane_t(const vec3_t& n, const float d) : normal(n), distance(d) {}
	plane_t(const vec4_t& v) : normal(v.x, v.y, v.z), distance(v.w) {}

	plane_t& operator=(const vec4_t& v) { normal.set(v.x, v.y, v.z); distance = v.w; return *this; }

	vec3_t normal;
	float distance;

	void set(const vec3_t& n, const float d) { normal = n; distance = d; }
	void set(const float x, const float y, const float z, const float d) { normal.set(x, y, z); distance = d; }
	void rescale() { float length = normal.length(); normal /= length; distance /= length; }
	operator vec4_t&() { return *(reinterpret_cast<vec4_t*>(this)); }
	operator const vec4_t&() const { return *(reinterpret_cast<const vec4_t*>(this)); }
};

inline bool
is_before(const vec3_t& point, const plane_t& plane)
{
	return dot(point, plane.normal) > plane.distance;
}

inline bool
is_behind(const vec3_t& point, const plane_t& plane)
{
	return dot(point, plane.normal) < plane.distance;
}

class bbox_t {
	// bounding box
public:
	bbox_t() {}
	bbox_t(const vec3_t& min, const vec3_t& max) : mins(min), maxs(max) {}

	vec3_t mins;
	vec3_t maxs;

	float  diagonal_length()   const { return (maxs - mins).length(); }
	vec3_t midpoint() const { return (mins + maxs) * 0.5f; }
};

class bsphere_t {
	// bounding sphere
public:
	bsphere_t() {}
	bsphere_t(const vec3_t& mid, const float rad) : 
		midpoint(mid), 
		radius(rad)
	{}

	vec3_t midpoint;
	float radius;
};

class frustum_t {
public:
	plane_t znear;
	plane_t zfar;
	plane_t left;
	plane_t right;
	plane_t bottom;
	plane_t top;

	frustum_t() {}
	frustum_t(const matrix_t& m) { set_from_projection(m); }

	void set_from_projection(const matrix_t& mt)
	{
		matrix_t m(mt);
		m.transpose();

		znear  = m[2];
		zfar   = m[3] - m[2];
		left   = m[3] + m[0];
		right  = m[3] - m[0];
		bottom = m[3] + m[1];
		top    = m[3] - m[1];

		znear.rescale();
		zfar.rescale();
		left.rescale();
		right.rescale();
		bottom.rescale();
		top.rescale();
	}

	plane_t& operator[](int n) { return *(reinterpret_cast<plane_t*>(this) + n); }
	const plane_t& operator[](int n) const { return *(reinterpret_cast<const plane_t*>(this) + n); }

	bool intersect_sphere(const bsphere_t& sphere) {
		float mradius = -sphere.radius;
		if (dot(sphere.midpoint, znear.normal ) + znear.distance  <= mradius) return false;
		if (dot(sphere.midpoint, zfar.normal  ) + zfar.distance   <= mradius) return false;
		if (dot(sphere.midpoint, left.normal  ) + left.distance   <= mradius) return false;
		if (dot(sphere.midpoint, right.normal ) + right.distance  <= mradius) return false;
		if (dot(sphere.midpoint, bottom.normal) + bottom.distance <= mradius) return false;
		if (dot(sphere.midpoint, top.normal   ) + top.distance    <= mradius) return false;
		return true;
	}

	bool intersect_bounds(const bbox_t& bounds) {
		return intersect_sphere(bsphere_t(bounds.midpoint(), bounds.diagonal_length() * 0.5f));
	}
};

//if (dot(plane.normal, (pos - plane.normal * plane.distance) >= 0)
//	point is in front of the plane

#endif
