/*
** vectors.h
** Vector math routines.
**
**---------------------------------------------------------------------------
** Copyright 2005-2007 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** Since C++ doesn't let me add completely new operators, the following two
** are overloaded for vectors:
**
**    |  dot product
**    ^  cross product
*/

#ifndef VECTORS_H
#define VECTORS_H

#include <math.h>
#include <string.h>

template<class vec_t>
struct TVector2
{
	vec_t X, Y;

	TVector2 ()
	{
	}

	TVector2 (double a, double b)
		: X(vec_t(a)), Y(vec_t(b))
	{
	}

	TVector2 (const TVector2 &other)
		: X(other.X), Y(other.Y)
	{
	}

	TVector2 &operator= (const TVector2 &other)
	{
		// This might seem backwards, but this helps produce smaller code when a newly
		// created vector is assigned, because the components can just be popped off
		// the FPU stack in order without the need for fxch. For platforms with a
		// more sensible registered-based FPU, of course, the order doesn't matter.
		// (And, yes, I know fxch can improve performance in the right circumstances,
		// but this isn't one of those times. Here, it's little more than a no-op that
		// makes the exe 2 bytes larger whenever you assign one vector to another.)
		Y = other.Y, X = other.X;
		return *this;
	}

	// Access X and Y as an array
	vec_t &operator[] (int index)
	{
		return *(&X + index);
	}

	const vec_t &operator[] (int index) const
	{
		return *(&X + index);
	}

	// Test for equality
	bool operator== (const TVector2 &other) const
	{
		return X == other.X && Y == other.Y;
	}

	// Test for inequality
	bool operator!= (const TVector2 &other) const
	{
		return X != other.X || Y != other.Y;
	}

	// Unary negation
	TVector2 operator- () const
	{
		return TVector2(-X, -Y);
	}

	// Scalar addition
	TVector2 &operator+= (double scalar)
	{
		X += scalar, Y += scalar;
		return *this;
	}

	friend TVector2 operator+ (const TVector2 &v, double scalar)
	{
		return TVector2(v.X + scalar, v.Y + scalar);
	}

	friend TVector2 operator+ (double scalar, const TVector2 &v)
	{
		return TVector2(v.X + scalar, v.Y + scalar);
	}

	// Scalar subtraction
	TVector2 &operator-= (double scalar)
	{
		X -= scalar, Y -= scalar;
		return *this;
	}

	TVector2 operator- (double scalar) const
	{
		return TVector2(X - scalar, Y - scalar);
	}

	// Scalar multiplication
	TVector2 &operator*= (double scalar)
	{
		X *= scalar, Y *= scalar;
		return *this;
	}

	friend TVector2 operator* (const TVector2 &v, double scalar)
	{
		return TVector2(v.X * scalar, v.Y * scalar);
	}

	friend TVector2 operator* (double scalar, const TVector2 &v)
	{
		return TVector2(v.X * scalar, v.Y * scalar);
	}

	// Scalar division
	TVector2 &operator/= (double scalar)
	{
		scalar = 1 / scalar, X *= scalar, Y *= scalar;
		return *this;
	}

	TVector2 operator/ (double scalar) const
	{
		scalar = 1 / scalar;
		return TVector2(X * scalar, Y * scalar);
	}

	// Vector addition
	TVector2 &operator+= (const TVector2 &other)
	{
		X += other.X, Y += other.Y;
		return *this;
	}

	TVector2 operator+ (const TVector2 &other) const
	{
		return TVector2(X + other.X, Y + other.Y);
	}

	// Vector subtraction
	TVector2 &operator-= (const TVector2 &other)
	{
		X -= other.X, Y -= other.Y;
		return *this;
	}

	TVector2 operator- (const TVector2 &other) const
	{
		return TVector2(X - other.X, Y - other.Y);
	}

	// Dot product
	double operator | (const TVector2 &other) const
	{
		return X*other.X + Y*other.Y;
	}
};

typedef TVector2<float>		FVector2;

#endif
