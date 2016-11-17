//-----------------------------------------------------------------------------
// File: maths.cpp
//
// Maths library implementation
//-----------------------------------------------------------------------------

#include "maths.h"
#include "util.h"

#include "mem.h"
#define new mem_new

const color_t color_t::identity= 0xffffffff;
const color_t color_t::black   = 0x00000000;
const color_t color_t::white   = 0x00ffffff;
const color_t color_t::red     = 0x00ff0000;
const color_t color_t::green   = 0x0000ff00;
const color_t color_t::blue    = 0x000000ff;
const color_t color_t::magenta = 0x00ff00ff;
const color_t color_t::yellow  = 0x00ffff00;
const color_t color_t::cyan    = 0x0000ffff;

const vec2_t vec2_t::origin(0.0f, 0.0f);
const vec3_t vec3_t::origin(0.0f, 0.0f, 0.0f);
const vec4_t vec4_t::origin(0.0f, 0.0f, 0.0f, 0.0f);

const matrix_t matrix_t::identity(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f);

matrix_t&
matrix_t::operator=(const matrix_t& m)
	// Matrix assignment to self
{
	_00 = m._00; _01 = m._01; _02 = m._02; _03 = m._03;
	_10 = m._10; _11 = m._11; _12 = m._12; _13 = m._13;
	_20 = m._20; _21 = m._21; _22 = m._22; _23 = m._23;
	_30 = m._30; _31 = m._31; _32 = m._32; _33 = m._33;
	return *this;
}

bool
matrix_t::operator==(const matrix_t& m) const
	// Matrix equality with self
{
	return (
		_00 == m._00 && _01 == m._01 && _02 == m._02 && _03 == m._03
	 && _10 == m._10 && _11 == m._11 && _12 == m._12 && _13 == m._13
	 && _20 == m._20 && _21 == m._21 && _22 == m._22 && _23 == m._23
	 && _30 == m._30 && _31 == m._31 && _32 == m._32 && _33 == m._33);
}

bool
matrix_t::operator!=(const matrix_t& m) const
	// Matrix inequality with self
{
	return (
		_00 != m._00 || _01 != m._01 || _02 != m._02 || _03 != m._03
	 || _10 != m._10 || _11 != m._11 || _12 != m._12 || _13 != m._13
	 || _20 != m._20 || _21 != m._21 || _22 != m._22 || _23 != m._23
	 || _30 != m._30 || _31 != m._31 || _32 != m._32 || _33 != m._33);
}

matrix_t&
matrix_t::operator*=(const float f)
	// Multiply self by scaler
{
	_00 *= f; _01 *= f; _02 *= f; _03 *= f;
	_10 *= f; _11 *= f; _12 *= f; _13 *= f;
	_20 *= f; _21 *= f; _22 *= f; _23 *= f;
	_30 *= f; _31 *= f; _32 *= f; _33 *= f;
	return *this;
}

matrix_t&
matrix_t::operator+=(const matrix_t& m)
	// Add another matrix to self
{
	_00 += m._00; _01 += m._01; _02 += m._02; _03 += m._03;
	_10 += m._10; _11 += m._11; _12 += m._12; _13 += m._13;
	_20 += m._20; _21 += m._21; _22 += m._22; _23 += m._23;
	_30 += m._30; _31 += m._31; _32 += m._32; _33 += m._33;
	return *this;
}

matrix_t&
matrix_t::operator-=(const matrix_t& m)
	// Subtract another matrix from self
{
	_00 -= m._00; _01 -= m._01; _02 -= m._02; _03 -= m._03;
	_10 -= m._10; _11 -= m._11; _12 -= m._12; _13 -= m._13;
	_20 -= m._20; _21 -= m._21; _22 -= m._22; _23 -= m._23;
	_30 -= m._30; _31 -= m._31; _32 -= m._32; _33 -= m._33;
	return *this;
}

void
matrix_t::look_at_rh(const vec3_t& eye, const vec3_t& at, const vec3_t& up)
{
	vec3_t zaxis = normalize(eye - at);
	vec3_t xaxis = normalize(cross(up, zaxis));
	vec3_t yaxis = cross(zaxis, xaxis);

	_00 = xaxis.x;          _01 = yaxis.x;          _02 = zaxis.x;          _03 = 0.0f; 
	_10 = xaxis.y;          _11 = yaxis.y;          _12 = zaxis.y;          _13 = 0.0f; 
	_20 = xaxis.z;          _21 = yaxis.z;          _22 = zaxis.z;          _23 = 0.0f; 
	_30 = -dot(xaxis, eye); _31 = -dot(yaxis, eye); _32 = -dot(zaxis, eye); _33 = 1.0f; 
}

void
matrix_t::ortho_rh(float l, float r, float b, float t, float n, float f)
	// Assign self to the left handed orthographic projection matrix
{
	float irml = 1.0f / (r - l);	// inverse right minus left
	float itmb = 1.0f / (t - b);	// inverse top minus bottom
	float inmf = 1.0f / (n - f);	// inverse near minus far

	_00 =     2.0f * irml; _01 =            0.0f; _02 =     0.0f; _03 = 0.0f;
	_10 =            0.0f; _11 =     2.0f * itmb; _12 =     0.0f; _13 = 0.0f;
	_20 =            0.0f; _21 =            0.0f; _22 =     inmf; _23 = 0.0f;
	_30 = -(r + l) * irml; _31 = -(t + b) * itmb; _32 = n * inmf; _33 = 1.0f;
}

void
matrix_t::perspective_fov_rh(const float fov, const float aspect, const float n, const float f)
	// Assign self to an arbitrary right handed perspective projection matrix
{
//	float sin, cos;
//	m_sincos(fovy / 2.0f, sin, cos);
//	float h = cos / sin;
//	float w = h / aspect;
//	float npn = n + n;			// near plus near
//	float finmf = f / (n - f);	// far * inverse near minus far
//
//	_00 = npn / w; _01 =    0.0f; _02 =      0.0f; _03 =  0.0f;
//	_10 =    0.0f; _11 = npn / h; _12 =      0.0f; _13 =  0.0f;
//	_20 =    0.0f; _21 =    0.0f; _22 =     finmf; _23 = -1.0f;
//	_30 =    0.0f; _31 =    0.0f; _32 = n * finmf; _33 =  0.0f;

	float r = n * m_tan(fov / 2.0f);
	float l = -r;
	float t = aspect * r;
	float b = -t;

	//perspective_rh(-tan, tan, -tan * aspect, tan * aspect, n, f);
	perspective_rh(l, r, b, t, n, f);
}

void
matrix_t::perspective_rh(const float l, const float r, const float b, const float t, const float n, const float f)
	// Assign self to an arbitrary right handed perspective projection matrix
{
	float npn = n + n;			// near plus near
	float tmb = t - b;			// top minus bottom
	float irml = 1.0f / (r - l);// inverse right minus left
	float finmf = f / (n - f);	// far inverse near minus far

	_00 =     npn * irml; _01 =          0.0f; _02 =      0.0f; _03 =  0.0f;
	_10 =           0.0f; _11 =     npn / tmb; _12 =      0.0f; _13 =  0.0f;
	_20 = (r + l) * irml; _21 = (t + b) / tmb; _22 =     finmf; _23 = -1.0f;
	_30 =           0.0f; _31 =          0.0f; _32 = n * finmf; _33 =  0.0f;
}

void
matrix_t::rotation(const vec3_t& axis, float angle)
	// Set to the matrix representing roating angle radians around the axis
	// positive angle rotations rotate counter-clockwise along the axis when
	// the axis is pointing towards you
{
	float sin, cos, omcos;
	m_sincos(angle, sin, cos);
	omcos = 1 - cos;	// one minus cos

	_00 = cos + axis.x * axis.x * omcos;
	_01 = axis.x * axis.y * omcos + axis.z * sin;
	_02 = axis.x * axis.z * omcos - axis.y * sin;
	_03 = 0.0f;
	_10 = axis.x * axis.y * omcos - axis.z * sin;
	_11 = cos + axis.y * axis.y * omcos;
	_12 = axis.y * axis.z * omcos + axis.x * sin;
	_13 = 0.0f;
	_20 = axis.x * axis.z * omcos + axis.y * sin;
	_21 = axis.y * axis.z * omcos - axis.x * sin;
	_22 = cos + axis.z * axis.z * omcos;
	_23 = 0.0f;
	_30 = 0.0f;
	_31 = 0.0f;
	_32 = 0.0f;
	_33 = 1.0f;
}

void
matrix_t::rotation_x(float angle)
	// Assign self to rotate angle radians around the x axis
	// positive angle rotations rotate counter-clockwise along the axis when
	// the axis is pointing towards you
{
	float sin, cos;
	m_sincos(angle, sin, cos);
	_00 = 1.0f; _01 = 0.0f; _02 = 0.0f; _03 = 0.0f;
	_10 = 0.0f; _11 =  cos; _12 =  sin; _13 = 0.0f;
	_20 = 0.0f; _21 = -sin; _22 =  cos; _23 = 0.0f;
	_30 = 0.0f; _31 = 0.0f; _32 = 0.0f; _33 = 1.0f;
}

void
matrix_t::rotation_y(float angle)
	// Assign self to rotate angle radians around the y axis
	// positive angle rotations rotate counter-clockwise along the axis when
	// the axis is pointing towards you
{
	float sin, cos;
	m_sincos(angle, sin, cos);
	_00 =  cos; _01 = 0.0f; _02 = -sin; _03 = 0.0f;
	_10 = 0.0f; _11 = 1.0f; _12 = 0.0f; _13 = 0.0f;
	_20 =  sin; _21 = 0.0f; _22 =  cos; _23 = 0.0f;
	_30 = 0.0f; _31 = 0.0f; _32 = 0.0f; _33 = 1.0f;
}

void
matrix_t::rotation_z(float angle)
	// Assign self to rotate angle radians around the z axis
	// positive angle rotations rotate counter-clockwise along the axis when
	// the axis is pointing towards you
{
	float sin, cos;
	m_sincos(angle, sin, cos);
	_00 =  cos; _01 =  sin; _02 = 0.0f; _03 = 0.0f;
	_10 = -sin; _11 =  cos; _12 = 0.0f; _13 = 0.0f;
	_20 = 0.0f; _21 = 0.0f; _22 = 1.0f; _23 = 0.0f;
	_30 = 0.0f; _31 = 0.0f; _32 = 0.0f; _33 = 1.0f;
}

void
matrix_t::scale(const float x, const float y, const float z)
	// Assign self to an arbitrary scale matrix
{
	_00 =    x; _01 = 0.0f; _02 = 0.0f; _03 = 0.0f;
	_10 = 0.0f; _11 =    y; _12 = 0.0f; _13 = 0.0f;
	_20 = 0.0f; _21 = 0.0f; _22 =    z; _23 = 0.0f;
	_30 = 0.0f; _31 = 0.0f; _32 = 0.0f; _33 = 1.0f;
}

void
matrix_t::translation(const float x, const float y, const float z)
	// Assign self to the translation matrix for x, y and z
{
	_00 = 1.0f; _01 = 0.0f; _02 = 0.0f; _03 = 0.0f;
	_10 = 0.0f; _11 = 1.0f; _12 = 0.0f; _13 = 0.0f;
	_20 = 0.0f; _21 = 0.0f; _22 = 1.0f; _23 = 0.0f;
	_30 =    x; _31 =    y; _32 =    z; _33 = 1.0f;
}

void
matrix_t::transpose() 
	// Transpose rows and columns of self
{
	u_swap(_01, _10);
	u_swap(_02, _20);
	u_swap(_03, _30);
	u_swap(_12, _21);
	u_swap(_13, _31);
	u_swap(_23, _32);
}

float
matrix_t::determinant() const
	// Returns the determinant of the matrix, definitely not an optimal
	// implementation
{
	matrix_t tmp(*this);
	float det = 1.0f;

	if (tmp._00 == 0.0f) {
		if (tmp._10 != 0.0f)
			u_swap(tmp[0], tmp[1]);
		else if (tmp._20 != 0.0f)
			u_swap(tmp[0], tmp[2]);
		else if (tmp._30 != 0.0f)
			u_swap(tmp[0], tmp[3]);
		else
			return 0.0f;
		det = -det;
	}
	det *= tmp._00;
	tmp[0] /= tmp._00;
	tmp[1] -= tmp._10 * tmp[0];
	tmp[2] -= tmp._20 * tmp[0];
	tmp[3] -= tmp._30 * tmp[0];
	if (tmp._11 == 0.0f) {
		if (tmp._21 != 0.0f)
			u_swap(tmp[1], tmp[2]);
		else if (tmp._31 != 0.0f)
			u_swap(tmp[1], tmp[3]);
		else
			return 0.0f;
		det = -det;
	}
	det *= tmp._11;
	tmp[1] /= tmp._11;
	tmp[2] -= tmp._21 * tmp[1];
	tmp[3] -= tmp._31 * tmp[1];
	if (tmp._22 == 0.0f) {
		if (tmp._33 != 0.0f)
			u_swap(tmp[2], tmp[3]);
		else
			return 0.0f;
		det = -det;
	}
	det *= tmp._22;
	tmp[2] /= tmp._22;
	tmp[3] -= tmp._32 * tmp[2];
	return det * tmp._33;
}

vec3_t
matrix_t::transform(const vec3_t& v) const
	// Transform row vector v through the arbitrary matrix, using 1.0f for the w value
{
	float wscale = 1.0f / (_30 + _31 + _32 + _33);
	return vec3_t(
		(_00 * v.x + _10 * v.y + _20 * v.z + _30) * wscale,
		(_01 * v.x + _11 * v.y + _21 * v.z + _31) * wscale,
		(_02 * v.x + _12 * v.y + _22 * v.z + _32) * wscale
	);
}

vec4_t
matrix_t::transform(const vec4_t& v) const
	// Transform row vector v through the arbitrary matrix, using 1.0f for the w value
{
	return vec4_t(
		(_00 * v.x + _10 * v.y + _20 * v.z + v.w * _30),
		(_01 * v.x + _11 * v.y + _21 * v.z + v.w * _31),
		(_02 * v.x + _12 * v.y + _22 * v.z + v.w * _32),
		(_03 * v.x + _13 * v.y + _23 * v.z + v.w * _33)
	);
}

vec3_t
matrix_t::transform_point(const vec3_t& v) const
	// Transform a row vector, assumes that the matrix is of the form:
	// [ m00, m01, m02, 0 ]
	// [ m10, m11, m12, 0 ]
	// [ m20, m21, m22, 0 ]
	// [ m30, m31, m32, 1 ]
	// This method uses 1.0f for the w value of v
{
	return vec3_t(
		_00 * v.x + _10 * v.y + _20 * v.z + _30,
		_01 * v.x + _11 * v.y + _21 * v.z + _31,
		_02 * v.x + _12 * v.y + _22 * v.z + _32
	);
}

vec3_t
matrix_t::transform_dir(const vec3_t& v) const
	// Transform a row vector, assumes that the matrix is of the form:
	// [ m00, m01, m02, 0 ]
	// [ m10, m11, m12, 0 ]
	// [ m20, m21, m22, 0 ]
	// [ m30, m31, m32, 1 ]
	// This method uses 0.0f for the w value of v
{
	return vec3_t(
		_00 * v.x + _10 * v.y + _20 * v.z,
		_01 * v.x + _11 * v.y + _21 * v.z,
		_02 * v.x + _12 * v.y + _22 * v.z
	);
}

bool
invert(matrix_t src, matrix_t& dst)
	// Invert src into dest, returns true if src is invertible, if
	// false is returned then the contents of dest are undefined
{
	dst = matrix_t::identity;

	if (src._00 == 0.0f) {
		if (src._10 != 0.0f) {
			u_swap(src[0], src[1]);
			u_swap(dst[0], dst[1]);
		} else if (src._20 != 0.0f) {
			u_swap(src[0], src[2]);
			u_swap(dst[0], dst[2]);
		} else if (src._30 != 0.0f) {
			u_swap(src[0], src[3]);
			u_swap(dst[0], dst[3]);
		}
	}
	if (src._00 != 0.0f) {
		float div = 1.0f / src._00;
		src[0] *= div;
		dst[0] *= div;
		dst[1] -= dst[0] * src._10;
		src[1] -= src[0] * src._10;
		dst[2] -= dst[0] * src._20;
		src[2] -= src[0] * src._20;
		dst[3] -= dst[0] * src._30;
		src[3] -= src[0] * src._30;
		if (src._11 == 0.0f) {
			if (src._21 != 0.0f) {
				u_swap(src[1], src[2]);
				u_swap(dst[1], dst[2]);
			} else if (src._31 != 0.0f) {
				u_swap(src[1], src[3]);
				u_swap(dst[1], dst[3]);
			}
		}
		if (src._11 != 0.0f) {
			float div = 1.0f / src._11;
			src[1] *= div;
			dst[1] *= div;
			dst[0] -= dst[1] * src._01;
			src[0] -= src[1] * src._01;
			dst[2] -= dst[1] * src._21;
			src[2] -= src[1] * src._21;
			dst[3] -= dst[1] * src._31;
			src[3] -= src[1] * src._31;
			if (src._22 == 0.0f) {
				if (src._32 != 0.0f) {
					u_swap(src[2], src[3]);
					u_swap(dst[2], dst[3]);
				}
			}
			if (src._22 != 0.0f) {
				float div = 1.0f / src._22;
				src[2] *= div;
				dst[2] *= div;
				dst[0] -= dst[2] * src._02;
				src[0] -= src[2] * src._02;
				dst[1] -= dst[2] * src._12;
				src[1] -= src[2] * src._12;
				dst[3] -= dst[2] * src._32;
				src[3] -= src[2] * src._32;
				if (src._33 != 0.0f) {
					float div = 1.0f / src._33;
					src[3] *= div;
					dst[3] *= div;
					dst[0] -= dst[3] * src._03;
					src[0] -= src[3] * src._03;
					dst[1] -= dst[3] * src._13;
					src[1] -= src[3] * src._13;
					dst[2] -= dst[3] * src._23;
					src[2] -= src[3] * src._23;
					return true;
				}
			}
		}
	}
	return false;
}

matrix_t
operator-(const matrix_t& m)
	// Negate a matrix
{
	return matrix_t(
		-m._00, -m._01, -m._02, -m._03,
		-m._10, -m._11, -m._12, -m._13,
		-m._20, -m._21, -m._22, -m._23,
		-m._30, -m._31, -m._32, -m._33);
}

matrix_t
operator*(const matrix_t& m, const float f)
	// Multiply a matrix by a scaler
{
	return matrix_t(
		m._00 * f, m._01 * f, m._02 * f, m._03 * f,
		m._10 * f, m._11 * f, m._12 * f, m._13 * f,
		m._20 * f, m._21 * f, m._22 * f, m._23 * f,
		m._30 * f, m._31 * f, m._32 * f, m._33 * f);
}

matrix_t 
operator+(const matrix_t& a, const matrix_t& b)
	// Add two matrices
{
	return matrix_t(
		a._00 + b._00, a._01 + b._01, a._02 + b._02, a._03 + b._03,
		a._10 + b._10, a._11 + b._11, a._12 + b._12, a._13 + b._13,
		a._20 + b._20, a._21 + b._21, a._22 + b._22, a._23 + b._23,
		a._30 + b._30, a._31 + b._31, a._32 + b._32, a._33 + b._33);
}

matrix_t 
operator-(const matrix_t& a, const matrix_t& b)
	// Subtract two matrices
{
	return matrix_t(
		a._00 - b._00, a._01 - b._01, a._02 - b._02, a._03 - b._03,
		a._10 - b._10, a._11 - b._11, a._12 - b._12, a._13 - b._13,
		a._20 - b._20, a._21 - b._21, a._22 - b._22, a._23 - b._23,
		a._30 - b._30, a._31 - b._31, a._32 - b._32, a._33 - b._33);
}

matrix_t 
operator*(const matrix_t& a, const matrix_t& b)
	// Multiply one matrix by another
{
	return matrix_t(
		a._00 * b._00 + a._01 * b._10 + a._02 * b._20 + a._03 * b._30,
		a._00 * b._01 + a._01 * b._11 + a._02 * b._21 + a._03 * b._31,
		a._00 * b._02 + a._01 * b._12 + a._02 * b._22 + a._03 * b._32,
		a._00 * b._03 + a._01 * b._13 + a._02 * b._23 + a._03 * b._33,
		a._10 * b._00 + a._11 * b._10 + a._12 * b._20 + a._13 * b._30,
		a._10 * b._01 + a._11 * b._11 + a._12 * b._21 + a._13 * b._31,
		a._10 * b._02 + a._11 * b._12 + a._12 * b._22 + a._13 * b._32,
		a._10 * b._03 + a._11 * b._13 + a._12 * b._23 + a._13 * b._33,
		a._20 * b._00 + a._21 * b._10 + a._22 * b._20 + a._23 * b._30,
		a._20 * b._01 + a._21 * b._11 + a._22 * b._21 + a._23 * b._31,
		a._20 * b._02 + a._21 * b._12 + a._22 * b._22 + a._23 * b._32,
		a._20 * b._03 + a._21 * b._13 + a._22 * b._23 + a._23 * b._33,
		a._30 * b._00 + a._31 * b._10 + a._32 * b._20 + a._33 * b._30,
		a._30 * b._01 + a._31 * b._11 + a._32 * b._21 + a._33 * b._31,
		a._30 * b._02 + a._31 * b._12 + a._32 * b._22 + a._33 * b._32,
		a._30 * b._03 + a._31 * b._13 + a._32 * b._23 + a._33 * b._33);
}
