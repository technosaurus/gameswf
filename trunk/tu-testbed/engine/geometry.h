// geometry.hpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some basic geometric types.


#ifndef GEOMETRY_H
#define GEOMETRY_H


class	vec3
// 3-element vector class, for 3D math.
{
public:
	vec3() {}
	vec3(float X, float Y, float Z) { m[0] = X; m[1] = Y; m[2] = Z; }
	vec3(const vec3& v) { m[0] = v.m[0]; m[1] = v.m[1]; m[2] = v.m[2]; }

	operator	const float*() const { return &m[0]; }
			
	float	get(int element) const { return m[element]; }
	void	set(int element, float NewValue) { m[element] = NewValue; }
	float	get_x() const { return m[0]; }
	float	get_y() const { return m[1]; }
	float	get_z() const { return m[2]; }
	float&	x() { return m[0]; }
	float&	y() { return m[1]; }
	float&	z() { return m[2]; }
	void	set_xyz(float newx, float newy, float newz) { m[0] = newx; m[1] = newy; m[2] = newz; }
	
	vec3	operator+(const vec3& v) const;
	vec3	operator-(const vec3& v) const;
	vec3	operator-() const;
	float	operator*(const vec3& v) const;
	vec3	operator*(float f) const;
	vec3	operator/(float f) const { return this->operator*(1.0 / f); }
	vec3	cross(const vec3& v) const;

	vec3&	normalize();
	vec3&	operator=(const vec3& v) { m[0] = v.m[0]; m[1] = v.m[1]; m[2] = v.m[2]; return *this; }
	vec3&	operator+=(const vec3& v);
	vec3& operator-=(const vec3& v);
	vec3&	operator*=(float f);
	vec3&	operator/=(float f) { return this->operator*=(1.0 / f); }

	float	magnitude() const;
	float	sqrmag() const;
//	float	min() const;
//	float	max() const;
//	float	minabs() const;
//	float	maxabs() const;

	bool	checknan() const;	// Returns true if any component is nan.
private:
	float	m[3];
};


#define INLINE_VEC3


#ifdef INLINE_VEC3


inline float	vec3::operator*(const vec3& v) const
// Dot product.
{
	float	result;
	result = m[0] * v.m[0];
	result += m[1] * v.m[1];
	result += m[2] * v.m[2];
	return result;
}


inline vec3&	vec3::operator+=(const vec3& v)
// Adds a vec3 to *this.
{
	m[0] += v.m[0];
	m[1] += v.m[1];
	m[2] += v.m[2];
	return *this;
}


inline vec3&	vec3::operator-=(const vec3& v)
// Subtracts a vec3 from *this.
{
	m[0] -= v.m[0];
	m[1] -= v.m[1];
	m[2] -= v.m[2];
	return *this;
}


#endif // INLINE_VEC3




extern vec3	ZeroVector, XAxis, YAxis, ZAxis;


class	quaternion;


class	matrix
// 3x4 matrix class, for 3D transformations.
{
public:
	matrix() { Identity(); }

	void	Identity();
	void	View(const vec3& ViewNormal, const vec3& ViewUp, const vec3& ViewLocation);
	void	Orient(const vec3& ObjectDirection, const vec3& ObjectUp, const vec3& ObjectLocation);

	static void	Compose(matrix* dest, const matrix& left, const matrix& right);
	vec3	operator*(const vec3& v) const;
	matrix	operator*(const matrix& m) const;
//	operator*=(const quaternion& q);

	matrix&	operator*=(float f);
	matrix&	operator+=(const matrix& m);
	
	void	Invert();
	void	InvertRotation();
	void	NormalizeRotation();
	void	Apply(vec3* result, const vec3& v) const;
	void	ApplyRotation(vec3* result, const vec3& v) const;
	void	ApplyInverse(vec3* result, const vec3& v) const;
	void	ApplyInverseRotation(vec3* result, const vec3& v) const;
	void	Translate(const vec3& v);
	void	SetOrientation(const quaternion& q);
	quaternion	GetOrientation() const;
	
	void	SetColumn(int column, const vec3& v) { m[column] = v; }
	const vec3&	GetColumn(int column) const { return m[column]; }
private:
	vec3	m[4];
};


// class quaternion -- handy for representing rotations.

class quaternion {
public:
	quaternion() : S(1), V(ZeroVector) {}
	quaternion(const quaternion& q) : S(q.S), V(q.V) {}
	quaternion(float s, const vec3& v) : S(s), V(v) {}

	quaternion(const vec3& Axis, float Angle);	// Slightly dubious: semantics varies from other constructor depending on order of arg types.

	float	GetS() const { return S; }
	const vec3&	GetV() const { return V; }
	void	SetS(float s) { S = s; }
	void	SetV(const vec3& v) { V = v; }

	float	get(int i) const { if (i==0) return GetS(); else return V.get(i-1); }
	void	set(int i, float f) { if (i==0) S = f; else V.set(i-1, f); }

	quaternion	operator*(const quaternion& q) const;
	quaternion&	operator*=(float f) { S *= f; V *= f; return *this; }
	quaternion&	operator+=(const quaternion& q) { S += q.S; V += q.V; return *this; }

	quaternion&	operator=(const quaternion& q) { S = q.S; V = q.V; return *this; }
	quaternion&	normalize();
	quaternion&	operator*=(const quaternion& q);
	void	ApplyRotation(vec3* result, const vec3& v);
	
	quaternion	lerp(const quaternion& q, float f) const;
private:
	float	S;
	vec3	V;
};


namespace Geometry {
	vec3	Rotate(float Angle, const vec3& Axis, const vec3& Point);
};



#endif // GEOMETRY_H
