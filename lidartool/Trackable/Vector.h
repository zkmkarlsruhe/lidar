// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _VECTOR_H_
#define _VECTOR_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include <math.h>
//#include <malloc.h>
#include <stdio.h>
#include <memory.h>


/***************************************************************************
*** 
*** TYPEDEFS
***
****************************************************************************/

/*!
  \defgroup Vector Vector and Matrix Operations
  @{
*/

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

#ifndef X_INDEX

#define X_INDEX (0);
#define Y_INDEX (1);
#define Z_INDEX (2);
#define W_INDEX (3);

#endif

#ifdef min
#undef min
#undef max
#endif

/***************************************************************************
*** 
*** CLASS DECLARATIONS
***
****************************************************************************/

class Vector2D;
class Vector3D;
class Vector4D;
class Matrix4D;
class Matrix3H;
class Matrix3D;

/*!
  @}
*/

/***************************************************************************
*** 
*** Vector2D
***
****************************************************************************/

class Vector2D
{
public:

  float x; //!< x component of the vector
  float y; //!< y component of the vector
  float z; //!< z component of the vector

/*!
  Constructs a Vector2D with components x=0, y=0
*/
  Vector2D() :  x(0), y(0){};
/*!
  Constructs a Vector2D with components x=\em X, y=0
*/
  Vector2D( float  X ) :  x(X), y(X){};
/*!
  Constructs a Vector2D with components x=\em v[0], y=\em v[1]
*/
  Vector2D( float  v[2] ) :  x(v[0]), y(v[1]){};
/*!
  Constructs a Vector2D with components x=\em X, y=\em Y
*/
  Vector2D( float  X, float  Y ) : x(X), y(Y){};

/*!
  Returns the n'th component of the vector, where component is x with \em index = 0 and y with \em index = 1
*/
  float &operator [] ( int index ) const;
/*!
  Returns a pointer to the x component
*/
  operator float*() const;
/*!
  Returns a const pointer to the x component
*/
  operator const float*() const;

/*!
  Assigns components x and y of vector \em v to the components of this vector.
*/
  Vector2D &operator =( const Vector4D &v );
/*!
  Assigns components x and y of vector \em v to the components of this vector.
*/
  Vector2D &operator =( const Vector3D &v );
/*!
  Assigns components x and y of vector \em v to the components of this vector.
*/
  Vector2D &operator =( const Vector2D &v );

/*!
  Sets the components x and y of this vector to zero.
*/
  Vector2D   &null();
/*!
  Returns true, if all the components of this vector are zero.
*/
  bool 		isNull() 			const;
/*!
  Returns the length of this vector. The length is computed by sqrt(x*x+y*y)
*/
  double 	length() 			const;
/*!
  Normalizes this vector to be of length 1.

  If the length of this vector is zero, the vector is not changed.
*/
  const Vector2D   &normalize();
/*!
  Returns the euclidian distance of this vector to vector \em v.
*/
  double 	distance( const Vector2D &v ) const;
/*!
  Sets the x component of this vector to \em X and the y component to \em Y.
*/
  void 		set     ( float X, float Y );
/*!
  Returns the vector product of this vector and vector v (this X v).
*/
  Vector2D    product ( const Vector2D &v ) const;
/*!
  Prints the components of this vector to the standart output.
*/
  void		print( const char *text="" )   const;
/*!
  Returns the angle between this vector and the vector \em other in radians.
*/
  double        angle( const Vector2D &other ) const;
/*!
  Returns the orientation of this vector as angle in radians.

  The vector (0,1) returns angle 0, a vector pointing to the negative x axis -M_PI/2, a vector pointing to the positive x axis M_PI/2. Angle is in the range of -M_PI to M_PI.
*/
  double        angle() const;
/*!
  Rotates this vector by M_PI/2 counter clockwise.
*/
  Vector2D    rotateLeft();
/*!
  Rotates this vector by M_PI/2 clockwise.
*/
  Vector2D    rotateRight();

/*!
  Adds vector \em v to this vector.
*/
  Vector2D &operator +=( const Vector2D &v );
/*!
  Subtracts vector \em v from this vector.
*/
  Vector2D &operator -=( const Vector2D &v );
/*!
  Returns the sum of this vector and vector \em v.
*/
  Vector2D  operator + ( const Vector2D &v ) const;
/*!
  Returns this vector with all components multiplied with -1.
*/
  Vector2D  operator - () 			 const;
/*!
  Returns the difference between this vector and vector \em v (this-v).
*/
  Vector2D  operator - ( const Vector2D &v ) const;
/*!
  Returns this vector with all components multiplied with \em c.
*/
  Vector2D  operator * ( double c      ) const;
/*!
  Returns this vector with all components divided by \em c.
*/
  Vector2D  operator / ( double c      ) const;
/*!
  Multiplies all components of this vector with \em c.
*/
  Vector2D &operator *=( double c      );
/*!
  Devides all components of this vector by \em c.
*/
  Vector2D &operator /=( double c      );
/*!
  Devides component x of this vector by component x of vector \em v and y of this vector by component y of vector \em v.
*/
  Vector2D &operator /=( const Vector2D &v );
/*!
  This vector becomes the result of the matrix/vector multiplication if matrix \em m and this vector.
*/
  Vector2D &operator *=( const Matrix3D &m );
/*!
  This vector becomes the result of the matrix/vector multiplication if matrix \em m and this vector.
*/
  Vector2D &operator *=( const Matrix3H &m );
/*!
  Returns the scalar product of this vector with vector \em v.
*/
  double     operator  * ( const Vector2D &v ) const;
/*!
  Returns true if all components of vector \em v and this vector are equal, otherwise returns false.
*/
  bool       operator  ==( const Vector2D &v ) const;
/*!
  Returns true if one of the components of vector \em v and this vector are not equal, otherwise returns false.
*/
  bool       operator  !=( const Vector2D &v ) const;
  
};

/***************************************************************************
*** 
*** Vector2D inlines
***
****************************************************************************/


inline Vector2D &
Vector2D::operator =( const Vector2D &v )
{ x = v.x; y = v.y;
  return( *this );
}

inline bool 
Vector2D::operator ==( const Vector2D &v ) const
{ return( v.x==x && v.y==y );
}

inline bool 
Vector2D::operator !=( const Vector2D &v ) const
{ return( v.x!=x || v.y!=y );
}

inline void 	
Vector2D::set( float X, float Y )
{ x=X; y=Y; 
}

inline float &
Vector2D::operator [] ( int index ) const 
{ return( ((float*)this)[index] ); 
}

inline 
Vector2D::operator const float*()  const
{ return( (float*)this );
}

inline 
Vector2D::operator float*()  const
{ return( (float*)this );
}

inline Vector2D &
Vector2D::operator +=( const Vector2D &v )
{ x += v.x;
  y += v.y;
  return( *this );
}

inline Vector2D &
Vector2D::operator -=( const Vector2D &v )
{ x -= v.x;
  y -= v.y;
  return( *this );
}

inline Vector2D &
Vector2D::operator *=( double c )
{ x *= (float)c;
  y *= (float)c;
  return( *this );
}

inline Vector2D &
Vector2D::operator /=( double c )
{ x /= (float)c;
  y /= (float)c;
  return( *this );
}

inline Vector2D &
Vector2D::operator /=( const Vector2D &v )
{ x /= v.x;
  y /= v.y;
  return( *this );
}

inline Vector2D 
Vector2D::operator +( const Vector2D &v ) const
{ return( Vector2D( v.x+x, v.y+y ) );
}

inline Vector2D 
Vector2D::operator -( const Vector2D &v ) const
{ return( Vector2D( x-v.x, y-v.y ) );
}

inline Vector2D 
Vector2D::operator -() const
{ return( Vector2D( -x, -y ) );
}

inline Vector2D 
Vector2D::operator *( double c ) const
{ return( Vector2D( (float)x*c, (float)y*c ) );
}

inline Vector2D 
Vector2D::operator /( double c ) const
{ return( Vector2D( (float)x/c, (float)y/c ) );
}

inline double 
Vector2D::operator *( const Vector2D &v ) const
{ return( v.x*x + v.y*y  );
}

inline bool 
Vector2D::isNull() const
{ return( x==0 && y==0 );
}
 
inline Vector2D &
Vector2D::null()
{ x = 0;
  y = 0;
  return( *this );
}

inline double 
Vector2D::length() const
{
  return( sqrt( x*x + y*y ) );
}
 
inline const Vector2D &
Vector2D::normalize()
{ double len = length();
  if ( len > 1e-6 && len != 1.0 ) 
    *this /= len;
  return( *this );
}
 
inline double 
Vector2D::distance( const Vector2D &v ) const
{ Vector2D diff( v.x-x, v.y-y );
  return( diff.length() );
}
 
inline Vector2D 
Vector2D::product( const Vector2D &v ) const
{ return( Vector2D( y*v.z-z*v.y, z*v.x-x*v.z ) );
}

inline Vector2D 
Vector2D::rotateLeft()
{ return( Vector2D( y, -x ) );
}

inline Vector2D 
Vector2D::rotateRight()
{ return( Vector2D( -y, x ) );
}


/***************************************************************************
*** 
*** Vector3D
***
****************************************************************************/

class Vector3D
{
public:

  float x; //!< x component of the vector
  float y; //!< y component of the vector
  float z; //!< z component of the vector

/*!
  Constructs a Vector3D with components x=0, y=0, z=0
*/
  Vector3D() :  x(0), y(0), z(0){};
/*!
  Constructs a Vector3D with components x=\em X, y=\em X, z=\em X
*/
  Vector3D( float  X ) :  x(X), y(X), z(X){};
/*!
  Constructs a Vector3D with components x=\em X, y=\em Y, z=0
*/
  Vector3D( float  X, float  Y ) :  x(X), y(Y), z(0){};
/*!
  Constructs a Vector3D with components x=\em X, y=\em Y, z=\em Z
*/
  Vector3D( float  X, float  Y, float  Z ) : x(X), y(Y), z(Z){};
/*!
  Constructs a Vector3D with components x=\em v[0], y=\em v[1], z=\em v[2]
*/
  Vector3D( float  v[3] ) :  x(v[0]), y(v[1]), z(v[2]){};
/*!
  Constructs a Vector3D from a Vector2D
*/
  Vector3D( Vector2D &v ) :  x(v.x), y(v.y), z(0){};

/*!
  Returns the n'th component of the vector, where component is x with \em index = 0, y with \em index = 1 and z with \em index = 2
*/
  float &operator [] ( int index ) const;
/*!
  Returns a pointer to the x component
*/
  operator float*() const;
/*!
  Returns a const pointer to the x, y, z components
*/
  operator const float*() const;

/*!
  Assigns components x, y and z of vector \em v to the components of this vector.
*/
  Vector3D &operator =( const Vector4D &v );
/*!
  Assigns components x, y and z of vector \em v to the components of this vector.
*/
  Vector3D &operator =( const Vector3D &v );
/*!
  Assigns components x and y of vector \em v to the components of this vector.
*/
  Vector3D &operator =( const Vector2D &v );

/*!
  Sets the components x, y and z of this vector to zero.
*/
  Vector3D   &null();
/*!
  Returns true, if all the components of this vector are zero.
*/
  bool 		isNull() 			const;
/*!
  Returns the length of this vector. The length is computed by sqrt(x*x+y*y+z*z)
*/
  double 	length() 			const;
/*!
  Normalizes this vector to be of length 1.

  If the length of this vector is zero, the vector is not changed.
*/
  const Vector3D   &normalize();
/*!
  Returns the euclidian distance of this vector to vector \em v.
*/
  double 	distance( const Vector3D &v ) const;
/*!
  Sets the x component of this vector to \em X, the y component to \em Y and the z component to \em Z.
*/
  void 		set     ( float X, float Y, float Z );
/*!
  Returns the vector product of this vector and vector v (this X v).
*/
  Vector3D    product ( const Vector3D &v ) const;
/*!
  Prints the components of this vector to the standart output.
*/
  void		print( const char *text="" )   const;
/*!
  Returns the angle between this vector and the vector \em other in radians. The angle is in the range [0..M_PI].
*/
  double        angle( const Vector3D &other ) const;

/*!
  Adds vector \em v to this vector.
*/
  Vector3D &operator +=( const Vector3D &v );
/*!
  Subtracts vector \em v from this vector.
*/
  Vector3D &operator -=( const Vector3D &v );
/*!
  Returns the vector sum of this vector and vector \em v.
*/
  Vector3D  operator + ( const Vector3D &v ) const;
/*!
  Returns this vector with all components multiplied with -1.
*/
  Vector3D  operator - () 			 const;
/*!
  Returns the difference between this vector and vector \em v (this-v).
*/
  Vector3D  operator - ( const Vector3D &v ) const;
/*!
  Returns this vector with all components multiplied with \em c.
*/
  Vector3D  operator * ( double c      ) const;
/*!
  Returns this vector with all components divided by \em c.
*/
  Vector3D  operator / ( double c      ) const;
/*!
  Multiplies all components of this vector with \em c.
*/
  Vector3D &operator *=( double c      );
/*!
  This vector becomes the result of the matrix/vector multiplication if matrix \em m and this vector.
*/
  Vector3D &operator *=( const Matrix3D &m );
/*!
  This vector becomes the result of the matrix/vector multiplication if matrix \em m and this vector.
*/
  Vector3D &operator *=( const Matrix3H &m );
/*!
  This vector becomes the result of the matrix/vector multiplication if matrix \em m and this vector.
*/
  Vector3D &operator *=( const Matrix4D &m );
/*!
  Devides all components of this vector by \em c.
*/
  Vector3D &operator /=( double c      );
/*!
  Devides component x of this vector by component x of vector \em v , y of this vector by component y of vector \em v and z of this vector by component z of vector \em v.
*/
  Vector3D &operator /=( const Vector3D &v );
/*!
  Returns the scalar product of this vector with vector \em v.
*/
  double     operator  * ( const Vector3D &v ) const;
/*!
  Returns true if all components of vector \em v and this vector are equal, otherwise returns false.
*/
  bool       operator  ==( const Vector3D &v ) const;
/*!
  Returns true if one of the components of vector \em v and this vector are not equal, otherwise returns false.
*/
  bool       operator  !=( const Vector3D &v ) const;
/*!
  Clamps all components of this vector to fit into the range [min..max].
*/
  void 	     clamp( float min=0.0f, float max=1.0f );

/*!
  Returns a vector with the minimum of the given vectors per component.
*/
  static Vector3D min( const Vector3D &v0, const Vector3D &v1 );
/*!
  Returns a vector with the maximum of the given vectors per component.
*/
  static Vector3D max( const Vector3D &v0, const Vector3D &v1 );
/*!
  Returns a vector with the minimum of the given vectors per component.
*/
  static Vector3D min( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2 );
/*!
  Returns a vector with the maximum of the given vectors per component.
*/
  static Vector3D max( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2 );
/*!
  Returns a vector with the minimum of the given vectors per component.
*/
  static Vector3D min( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2, const Vector3D &v3 );
/*!
  Returns a vector with the maximum of the given vectors per component.
*/
  static Vector3D max( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2, const Vector3D &v3 );
};

/***************************************************************************
*** 
*** Vector3D inlines
***
****************************************************************************/

inline Vector3D &
Vector3D::operator =( const Vector2D &v )
{ x = v.x; y = v.y; z = 0.0;
  return( *this );
}

inline Vector2D &
Vector2D::operator =( const Vector3D &v )
{ x = v.x; y = v.y;
  return( *this );
}

inline Vector3D &
Vector3D::operator =( const Vector3D &v )
{ x = v.x; y = v.y; z = v.z;
  return( *this );
}

inline bool 
Vector3D::operator ==( const Vector3D &v ) const
{ return( v.x==x && v.y==y && v.z==z );
}

inline bool 
Vector3D::operator !=( const Vector3D &v ) const
{ return( v.x!=x || v.y!=y || v.z!=z );
}

inline void 	
Vector3D::set( float X, float Y, float Z )
{ x=X; y=Y; z=Z; 
}

inline float &
Vector3D::operator [] ( int index ) const 
{ return( ((float*)this)[index] ); 
}

inline 
Vector3D::operator const float*()  const
{ return( (float*)this );
}

inline 
Vector3D::operator float*()  const
{ return( (float*)this );
}

inline Vector3D &
Vector3D::operator +=( const Vector3D &v )
{ x += v.x;
  y += v.y;
  z += v.z;
  return( *this );
}

inline Vector3D &
Vector3D::operator -=( const Vector3D &v )
{ x -= v.x;
  y -= v.y;
  z -= v.z;
  return( *this );
}

inline Vector3D &
Vector3D::operator *=( double c )
{ x *= (float)c;
  y *= (float)c;
  z *= (float)c;
  return( *this );
}

inline Vector3D &
Vector3D::operator /=( double c )
{ x /= (float)c;
  y /= (float)c;
  z /= (float)c;
  return( *this );
}

inline Vector3D &
Vector3D::operator /=( const Vector3D &v )
{ x /= v.x;
  y /= v.y;
  z /= v.z;
  return( *this );
}

inline Vector3D 
Vector3D::operator +( const Vector3D &v ) const
{ return( Vector3D( x+v.x, y+v.y, z+v.z ) );
}

inline Vector3D 
Vector3D::operator -( const Vector3D &v ) const
{ return( Vector3D( x-v.x, y-v.y, z-v.z ) );
}

inline Vector3D 
Vector3D::operator -() const
{ return( Vector3D( -x, -y, -z ) );
}

inline Vector3D 
Vector3D::operator *( double c ) const
{ return( Vector3D( (float)x*c, (float)y*c, (float)z*c ) );
}

inline Vector3D 
Vector3D::operator /( double c ) const
{ return( Vector3D( (float)x/c, (float)y/c, (float)z/c ) );
}

inline double 
Vector3D::operator *( const Vector3D &v ) const
{ return( v.x*x + v.y*y + v.z*z );
}

inline bool 
Vector3D::isNull() const
{ return( x==0 && y==0 && z==0 );
}
 
inline Vector3D &
Vector3D::null()
{ x = 0;
  y = 0;
  z = 0;
  return( *this );
}

inline double 
Vector3D::length() const
{
  return( sqrt( x*x + y*y + z*z ) );
}
 
inline const Vector3D &
Vector3D::normalize()
{ double len = length();
  if ( len > 1e-6 && len != 1.0 ) 
    *this /= len;
  return( *this );
}
 
inline double 
Vector3D::distance( const Vector3D &v ) const
{ Vector3D diff( x-v.x, y-v.y, z-v.z );
  return( diff.length() );
}
 
inline Vector3D 
Vector3D::product( const Vector3D &v ) const
{ return( Vector3D( y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x ) );
}

inline void
Vector3D::clamp( float min, float max )
{ if ( x < min ) x = min;
  else  if ( x > max ) x = max;
  if ( y < min ) y = min;
  else  if ( y > max ) y = max;
  if ( z < min ) z = min;
  else  if ( z > max ) z = max;
}

/***************************************************************************
*** 
*** Vector4D
***
****************************************************************************/

class Vector4D
{
public:

  float x; //!< x component of the vector
  float y; //!< y component of the vector
  float z; //!< z component of the vector
  float w; //!< w component of the vector

/*!
  Constructs a Vector3D with components x=0, y=0, z=0, w=0
*/
  Vector4D() 						:  x(0),    y(0),    z(0),    w(0)   {};
/*!
  Constructs a Vector3D with components x=\em X, y=\em X, z=\em X, w=1
*/
  Vector4D( float  X ) 				:  x(X),    y(X),    z(X),    w(1)   {};
/*!
  Constructs a Vector3D with components x=\em X, y=\em X, z=\em X, w=\em W
*/
  Vector4D( float  X, float  W ) 			:  x(X),    y(X),    z(X),    w(W)   {};
/*!
  Constructs a Vector3D with components x=\em v[0], y=\em v[1], z=\em v[2], w=1
*/
  Vector4D( float  v[3] )				:  x(v[0]), y(v[1]), z(v[2]), w(1.0) {};
/*!
  Constructs a Vector3D with components x=\em v[0], y=\em v[1], z=\em v[2], w=\em W
*/
  Vector4D( float  v[3], float W ) 			:  x(v[0]), y(v[1]), z(v[2]), w(W)   {};
/*!
  Constructs a Vector3D with components x=\em X, y=\em Y, z=\em Z, w=\em W
*/
  Vector4D( float  X, float  Y, float  Z, float W )  	:  x(X),    y(Y),    z(Z),    w(W)   {};
/*!
  Constructs a Vector3D with components x=\em X, y=\em Y, z=\em Z, w=1
*/
  Vector4D( float  X, float  Y, float  Z ) 	    	:  x(X),    y(Y),    z(Z),    w(1.0) {};
/*!
  Constructs a Vector3D with components x=\em v.x, y=\em v.y, z=\em v.z, w=1
*/
  Vector4D( const Vector3D &v ) 			:  x(v.x),  y(v.y),  z(v.z),  w(1.0) {};
/*!
  Constructs a Vector3D with components x=\em v.x, y=\em v.y, z=\em v.z, w=\em W
*/
  Vector4D( const Vector3D &v, float W ) 		:  x(v.x),  y(v.y),  z(v.z),  w(W)   {};
 
/*!
  Assigns components x and y of vector \em v to the components of this vector.
*/
  Vector4D &operator =( const Vector2D &v );
/*!
  Assigns components x, y and z of vector \em v to the components of this vector.
*/
  Vector4D &operator =( const Vector3D &v );
/*!
  Assigns components x, y, z and w of vector \em v to the components of this vector.
*/
  Vector4D &operator =( const Vector4D &v );

/*!
  Returns the n'th component of the vector, where component is x with \em index = 0, y with \em index = 1, z with \em index = 2 and w with \em index = 3
*/
  float &operator [] ( int index ) const;
/*!
  Returns a pointer to the x component
*/
  operator float*() const;
/*!
  Returns a const pointer to the x component
*/
  operator const float*() const;
/*!
  Returns a reference to this vector as Vector3D.
*/
  operator Vector3D&() const { return( (Vector3D&)*(char*)this ); };
  
/*!
  Sets the components x, y and z of this vector to zero.
*/
  Vector4D   &null();
/*!
  Returns true, if all the components of this vector are zero.
*/
  bool 		isNull() 			const;
/*!
  Returns the length of this vector. The length is computed by sqrt(x*x+y*y+z*z+w*w)
*/
  double 	length() 			const;
/*!
  Normalizes this vector to be of length 1.

  If the length of this vector is zero, the vector is not changed.
*/
  void 		normalize();
/*!
  Sets the x component of this vector to \em X, the y component to \em Y and the z component to \em Z.
*/
  void 		set     ( float X, float Y, float Z );
/*!
  Sets the x component of this vector to \em X, the y component to \em Y, the z component to \em Z and the w component to \em W.
*/
  void 		set     ( float X, float Y, float Z, float W );
/*!
  Returns the euclidian distance of this vector to vector \em v.
*/
  double 	distance( const Vector4D &v ) const;
/*!
  Prints the components of this vector to the standart output.
*/
  void 		print( const char *text="" )    const;

/*!
  Componentwise adds vector \em v to this vector.
*/
  Vector4D &operator +=( const Vector4D &v );
/*!
  Componentwise adds vector \em v to this vector.
*/
  Vector4D &operator +=( const Vector3D &v );
/*!
  Componentwise subtracts vector \em v from this vector.
*/
  Vector4D &operator -=( const Vector4D &v );
/*!
  Componentwise subtracts vector \em v from this vector.
*/
  Vector4D &operator -=( const Vector3D &v );
/*!
  Returns the vector sum of this vector and vector \em v.
*/
  Vector4D  operator + ( const Vector4D &v ) const;
/*!
  Returns this vector with all components multiplied with -1.
*/
  Vector4D  operator - ()			 const;
/*!
  Returns the difference between this vector and vector \em v (this-v).
*/
  Vector4D  operator - ( const Vector4D &v ) const;
/*!
  Returns this vector with all components multiplied with \em c.
*/
  Vector4D  operator * ( double c      )	 const;
/*!
  Returns this vector with all components divided by \em c.
*/
  Vector4D  operator / ( double c      )	 const;
/*!
  Multiplies all components of this vector with \em c.
*/
  Vector4D &operator *=( double c      );
/*!
  Devides all components of this vector by \em c.
*/
  Vector4D &operator /=( double c      );
/*!
  Devides component x of this vector by component x of vector \em v , y of this vector by component y of vector \em v and z of this vector by component z of vector \em v.
*/
  Vector4D &operator /=( const Vector3D &v );
/*!
  Devides component x of this vector by component x of vector \em v , y of this vector by component y of vector \em v, z of this vector by component z of vector \em v and w of this vector by component w of vector \em v.
*/
  Vector4D &operator /=( const Vector4D &v );
/*!
  Returns the scalar product of this vector with vector \em v.
*/
  double     operator  * ( const Vector4D &v ) const;
/*!
  Returns true if all components of vector \em v and this vector are equal, otherwise returns false.
*/
  bool       operator  ==( const Vector4D &v ) const;
/*!
  Returns true if one of the components of vector \em v and this vector are not equal, otherwise returns false.
*/
  bool       operator  !=( const Vector4D &v ) const;
/*!
  Clamps all components of this vector to fit into the range [min..max].
*/
  void 	     clamp( float min=0.0f, float max=1.0f );
  

};

/***************************************************************************
*** 
*** Vector4D inlines
***
****************************************************************************/

inline Vector4D &
Vector4D::operator =( const Vector2D &v )
{ x = v.x; y = v.y; z = 0.0; w = 1.0;
  return( *this );
}

inline Vector4D &
Vector4D::operator =( const Vector3D &v )
{ x = v.x; y = v.y; z = v.z; w = 1.0;
  return( *this );
}

inline Vector4D &
Vector4D::operator =( const Vector4D &v )
{ x = v.x; y = v.y; z = v.z; w = v.w;
  return( *this );
}

inline Vector2D &
Vector2D::operator =( const Vector4D &v )
{ x = v.x; y = v.y;
  return( *this );
}

inline Vector3D &
Vector3D::operator =( const Vector4D &v )
{ x = v.x; y = v.y; z = v.z;
  return( *this );
}

inline bool 
Vector4D::operator ==( const Vector4D &v ) const
{ return( v.x==x && v.y==y && v.z==z && v.w==w );
}

inline bool 
Vector4D::operator !=( const Vector4D &v ) const
{ return( v.x!=x || v.y!=y || v.z!=z || v.w!=w );
}

inline void 	
Vector4D::set( float X, float Y, float Z, float W )
{ x=X; y=Y; z=Z; w=W; 
}

inline void 	
Vector4D::set( float X, float Y, float Z )
{ x=X; y=Y; z=Z; w=1.0; 
}

inline float &
Vector4D::operator [] ( int index ) const 
{ return( ((float*)this)[index] ); 
}

inline 
Vector4D::operator const float*()  const
{ return( (float*)this );
}

inline 
Vector4D::operator float*()  const
{ return( (float*)this );
}

inline Vector4D &
Vector4D::operator +=( const Vector4D &v )
{ x += v.x;
  y += v.y;
  z += v.z;
  w += v.w;
  return( *this );
}

inline Vector4D &
Vector4D::operator +=( const Vector3D &v )
{ x += v.x;
  y += v.y;
  z += v.z;
  return( *this );
}

inline Vector4D &
Vector4D::operator -=( const Vector4D &v )
{ x -= v.x;
  y -= v.y;
  z -= v.z;
  w -= v.w;
  return( *this );
}

inline Vector4D &
Vector4D::operator -=( const Vector3D &v )
{ x -= v.x;
  y -= v.y;
  z -= v.z;
  return( *this );
}

inline Vector4D &
Vector4D::operator *=( double c )
{ x *= (float)c;
  y *= (float)c;
  z *= (float)c;
  w *= (float)c;
  return( *this );
}

inline Vector4D &
Vector4D::operator /=( double c )
{ x /= (float)c;
  y /= (float)c;
  z /= (float)c;
  w /= (float)c;
  return( *this );
}

inline Vector4D &
Vector4D::operator /=( const Vector3D &v )
{ x /= v.x;
  y /= v.y;
  z /= v.z;
  return( *this );
}

inline Vector4D &
Vector4D::operator /=( const Vector4D &v )
{ x /= v.x;
  y /= v.y;
  z /= v.z;
  w /= v.w;
  return( *this );
}

inline Vector4D 
Vector4D::operator +( const Vector4D &v ) const
{ return( Vector4D( v.x+x, v.y+y, v.z+z, v.w+w ) );
}

inline Vector4D 
Vector4D::operator -( const Vector4D &v ) const
{ return( Vector4D( x-v.x, y-v.y, z-v.z, w-v.w ) );
}

inline Vector4D 
Vector4D::operator -() const
{ return( Vector4D( -x, -y, -z, -w ) );
}

inline Vector4D 
Vector4D::operator *( double c ) const
{ return( Vector4D( (float)x*c, (float)y*c, (float)z*c, (float)w*c ) );
}

inline Vector4D 
Vector4D::operator /( double c ) const
{ return( Vector4D( (float)x/c, (float)y/c, (float)z/c, (float)w/c ) );
}

inline double 
Vector4D::operator *( const Vector4D &v ) const
{ return( v.x*x + v.y*y + v.z*z + v.w*w );
}

inline bool 
Vector4D::isNull() const
{ return( x==0 && y==0 && z==0 && w==0 );
}
 
inline Vector4D &
Vector4D::null()
{ x = 0;
  y = 0;
  z = 0;
  w = 0;
  return( *this );
}

inline double 
Vector4D::length() const
{
  return( sqrt( x*x + y*y + z*z + w*w ) );
}
 
inline double 
Vector4D::distance( const Vector4D &v ) const
{ Vector4D diff( x-v.x, y-v.y, z-v.z, w-v.w );
  return( diff.length() );
}
 
inline void
Vector4D::normalize()
{ double len = length();
  if ( len > 1e-6 && len != 1.0 ) 
    *this /= len;
}
 
inline void
Vector4D::clamp( float min, float max )
{ if ( x < min ) x = min;
  else  if ( x > max ) x = max;
  if ( y < min ) y = min;
  else  if ( y > max ) y = max;
  if ( z < min ) z = min;
  else  if ( z > max ) z = max;
  if ( w < min ) w = min;
  else  if ( w > max ) w = max;
}

/***************************************************************************
*** 
*** Matrix4D
***
****************************************************************************/

class Matrix3H;
class Matrix3D;

class Matrix4D
{
public:

  Vector4D x; //!< x component row vector of the matrix
  Vector4D y; //!< y component row vector of the matrix
  Vector4D z; //!< z component row vector of the matrix
  Vector4D w; //!< w component row vector of the matrix


/*!
  Constructs a identity Matrix4D.
  \code
    Matrix4D matrix; // constructs an identity matrix
  \endcode
*/
  Matrix4D();
/*!
  Constructs a Matrix4D as a copy of \em m with x.w=0, y.w=0, z.w=0 and w.w=1.
  \code
    Matrix3H matrix1; // constructs an identity matrix
    Matrix4D matrix2( matrix1 ); // constructs a matrix from the given matrix
  \endcode
*/
  Matrix4D( const Matrix3H &m );
/*!
  Constructs a Matrix4D as a copy of \em m with x.w=0, y.w=0, z.w=0 and w set to (0,0,0,1).
  \code
    Matrix4D matrix1; // constructs an identity matrix
    Matrix4D matrix2( matrix1 ); // constructs a matrix from the given matrix
  \endcode
*/
  Matrix4D( const Matrix3D &m );
/*!
  Constructs a Matrix4D as a copy of \em m with x.w=\em v.x, y.w=\em v.y, z.w=\em v.z and w=(0,0,0,1).
  \code
    Matrix3D matrix1;     // constructs an identity matrix
    Vector3D translation( 1.0f, 0.0f, 0.0f ); // constructs an translational vector
    Matrix4D matrix2( matrix1, translation ); // constructs a matrix from the given matrix
  \endcode
*/
  Matrix4D( const Matrix3D &m, const Vector3D &v );
/*!
  Constructs a Matrix4D with x=(\em vx,0), y=(\em vy,0), z=(\em v,0) and w=(0,0,0,1).
*/
  Matrix4D( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz  );
/*!
  Constructs a Matrix4D with x=(\em vx,0), y=(\em vy,0), z=(\em vz,0) and w=(\em vw,1).
*/
  Matrix4D( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz, const Vector3D &vw  );
/*!
  Constructs a Matrix4D with x=\em vx, y=\em vy, z=\em vz and w=vw.
*/
  Matrix4D( const Vector4D &vx, const Vector4D &vy, const Vector4D &vz, const Vector4D &vw  );
/*!
  Constructs a Matrix4D from a float matrix.
*/
  Matrix4D( const float m[4][4]  );
/*!
  Constructs a Matrix4D with x=(1,0,0,\em tx), y=(0,1,0,\em ty),  x=(0,0,1,\em tz),  x=(0,0,0,\em tw)
  \code
    Matrix4D matrix( 1.0f, 0.0f, 0.0f ); // constructs a translational matrix with x.w=1 y.w=0 z.w=0 w.w=1
  \endcode
*/
  Matrix4D( float tx, float ty, float tz, float tw=1.0f );
/*!
  Constructs a Matrix4D from float values in row order.
  \code
    Matrix4D matrix( 1.0f, 0.0f, 0.0f, 1.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f );
  // constructs a translational matrix with x=(1,0,0,1), y=(0,1,0,0), z=(0,0,1,0), w=(0,0,0,1)
  \endcode
*/
  Matrix4D( float xx, float xy, float xz, float xw,
	      float yx, float yy, float yz, float yw,
	      float zx, float zy, float zz, float zw,
	      float wx, float wy, float wz, float ww );

/*!
  Returns the n'th component of the matrix, where component is x with \em index = 0, y with \em index = 1, z with \em index = 2 and w with \em index = 3. The returned component is a Vector4D.
*/
  Vector4D &operator [] ( int index ) const;
/*!
  Assigns a Matrix3H to a Matrix4D.
*/
  Matrix4D &operator = ( const Matrix3H &m );
/*!
  Assigns a Matrix3D to a Matrix4D.
*/
  Matrix4D &operator = ( const Matrix3D &m );
/*!
  Returns a pointer to the first float of the x component
*/
  operator float*();
/*!
  Returns const a pointer to the first float of the x component
*/
  operator const float*();
/*!
  Converts the Matrix4D to a Matrix3H.
*/
  operator Matrix3H ();
/*!
  Converts the Matrix4D to a Matrix3D.
*/
  operator Matrix3D ();
  
/*!
  Returns true, if all the components of this matrix are zero.
*/
  bool 		isNull() 			const;
/*!
  Returns true, if the matrix is the identity matrix, otherwise returns false.
*/
  bool 		isId()				const;
/*!
  Sets all values to make a identity matrix.
*/
  void 		id();
/*!
  Prints the components of this matrix to the standart output. The dump is preceeded by the text \em text
*/
  void 		print( const char *text="" )   	const;
/*!
  Returns the inverse Matrix4D.
  \code
  Matrix4D m; 
  Matrix4D inverse = m.inverse();

  // inverse * m = m * inverse = id
  \endcode
*/
  Matrix4D    inverse() 	const;
/*!
  Returns the z component of the inverse Matrix4D.
  \code
  Matrix4D m; 
  Matrix4D inverse = m.inverse(); // inverse.z equals m.inverseZ()
  \endcode
*/
  Vector4D    inverseZ() 	const;
/*!
  Returns a transposed Matrix4D.


  Here is how its computed:
  \code
  return(
    Matrix4D(
    x.x, y.x, z.x, x.w,
    x.y, y.y, z.y, y.w,
    x.z, y.z, z.z, z.w,
    w.x, w.y, w.z, w.w
    )
  );
  \endcode
*/
  Matrix4D	transpose() 			const;
/*!
  Returns a Matrix4D where only the inner rotational matrix3D is transposed. The translational fragment of the matrix is kept.

  Here is how its computed:
  \code
  return(
    Matrix4D(
    x.x, y.x, z.x, x.w,
    x.y, y.y, z.y, y.w,
    x.z, y.z, z.z, z.w,
    w.x, w.y, w.z, w.w
    )
  );
  \endcode
*/
  Matrix4D	transposeMatrix3D()		const;
/*!
  Returns a Matrix4D which is composed by left hand multiplying this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  matrix2 = lrotate( matrix1, M_PI_2, 0.0f, 0.0f ); // matrix2 = matrixRotateXM_PI_2 * matrix1
  \endcode
*/
  Matrix4D	lrotate( const Matrix4D &m, float x, float y, float z ) const;
/*!
  Returns a Matrix4D which is composed by right hand multiplying this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  matrix2 = matrix1.rrotate( matrix1, M_PI_2, 0.0f, 0.0f ); // matrix2 = matrix1 * matrixRotateXM_PI_2
  \endcode
*/
  Matrix4D	rrotate( const Matrix4D &m, float x, float y, float z ) const;
/*!
  Left hand multiplies this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix4D matrix;
  matrix.lrotate( M_PI_2, 0.0f, 0.0f ); // matrix = matrixRotateXM_PI_2 * matrix
  \endcode
*/
  Matrix4D	&lrotate( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix4D matrix;
  matrix.rrotate( M_PI_2, 0.0f, 0.0f ); // matrix = matrix * matrixRotateXM_PI_2
  \endcode
*/
  Matrix4D	&rrotate( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the x axis.

  \code
  Matrix4D matrix;
  matrix.rrotateX( M_PI_2 ); // matrix = matrix * matrixRotateXM_PI_2
  \endcode
*/
  Matrix4D	&rrotateX( float x );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the y axis.

  \code
  Matrix4D matrix;
  matrix.rrotateY( M_PI_2 ); // matrix = matrix * matrixRotateYM_PI_2
  \endcode
*/
  Matrix4D	&rrotateY( float y );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the z axis.

  \code
  Matrix4D matrix;
  matrix.rrotateZ( M_PI_2 ); // matrix = matrix * matrixRotateZM_PI_2
  \endcode
*/
  Matrix4D	&rrotateZ( float z );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the x axis.

  \code
  Matrix4D matrix;
  matrix.lrotateX( M_PI_2 ); // matrix = matrixRotateXM_PI_2 * matrix
  \endcode
*/
  Matrix4D	&lrotateX( float x );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the y axis.

  \code
  Matrix4D matrix;
  matrix.lrotateY( M_PI_2 ); // matrix = matrixRotateYM_PI_2 * matrix
  \endcode
*/
  Matrix4D	&lrotateY( float y );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the z axis.

  \code
  Matrix4D matrix;
  matrix.lrotateZ( M_PI_2 ); // matrix = matrixRotateZM_PI_2 * matrix
  \endcode
*/
  Matrix4D	&lrotateZ( float z );
/*!
  Returns a Matrix4D which is composed by left hand multiplying this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  matrix2 = ltranslate( matrix1, 1.0f, 0.0f, 0.0f ); // matrix2 = matrixTranslateX * matrix1
  \endcode
*/
  Matrix4D	ltranslate( const Matrix4D &m, float x, float y, float z ) const;
/*!
  Returns a Matrix4D which is composed by right hand multiplying this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  matrix2 = rtranslate( matrix1, 1.0f, 0.0f, 0.0f ); // matrix2 = matrix1 * matrixTranslateX
  \endcode
*/
  Matrix4D	rtranslate( const Matrix4D &m, float x, float y, float z ) const;
/*!
  Returns a Matrix4D which is composed by left hand multiplying this matrix with a translation matrix build out of translations \em v.x, \em v.y, \em v.z along the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  Vector3D v( 1.0f, 0.0f, 0.0f );
  matrix2 = ltranslate( matrix1, v ); // matrix2 = matrixTranslateVX * matrix1
  \endcode
*/
  Matrix4D	ltranslate( const Matrix4D &m, const Vector3D &v ) const;
/*!
  Returns a Matrix4D which is composed by right hand multiplying this matrix with a translation matrix build out of translations \em v.x, \em v.y, \em v.z along the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  Vector3D v( 1.0f, 0.0f, 0.0f );
  matrix2 = rtranslate( matrix1, v ); // matrix2 = matrix1 * matrixTranslateVX
  \endcode
*/
  Matrix4D	rtranslate( const Matrix4D &m, const Vector3D &v ) const;
/*!
  Left hand multiplies this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix;
  matrix.ltranslate( 1.0f, 0.0f, 0.0f ); // matrix = matrixTranslateX * matrix
  \endcode
*/
  Matrix4D	&ltranslate( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix;
  matrix.rtranslate( 1.0f, 0.0f, 0.0f ); // matrix = matrix * matrixTranslateX
  \endcode
*/
  Matrix4D	&rtranslate( float x, float y, float z );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translations \em v.x, \em v.y, \em v.z along the x, y and z axis.

  \code
  Matrix4D matrix;
  Vector3D v( 1.0f, 0.0f, 0.0f );
  matrix.ltranslate( v ); // matrix = matrixTranslateVX * matrix
  \endcode
*/
  Matrix4D	&ltranslate( const Vector3D &v );
/*!
  Right hand multiplies this matrix with a translation matrix build out of translations \em v.x, \em v.y, \em v.z along the x, y and z axis.

  \code
  Matrix4D matrix;
  Vector3D v( 1.0f, 0.0f, 0.0f );
  matrix.rtranslate( v ); // matrix = matrix * matrixTranslateVX
  \endcode
*/
  Matrix4D	&rtranslate( const Vector3D &v );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translation \em x along the x axis.

  \code
  Matrix4D matrix;
  matrix.ltranslateX( 1.0f ); // matrix = matrixTranslateX * matrix
  \endcode
*/
  Matrix4D	&ltranslateX( float x );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translation \em y along the y axis.

  \code
  Matrix4D matrix;
  matrix.ltranslateY( 1.0f ); // matrix = matrixTranslateY * matrix
  \endcode
*/
  Matrix4D	&ltranslateY( float y );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translation \em z along the z axis.

  \code
  Matrix4D matrix;
  matrix.ltranslateZ( 1.0f ); // matrix = matrixTranslateZ * matrix
  \endcode
*/
  Matrix4D	&ltranslateZ( float z );
/*!
  Returns a Matrix4D which is composed by left hand multiplying this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  matrix2 = lscale( matrix1, 10.0f, 1.0f, 1.0f ); // matrix2 = matrixScaleX * matrix1
  \endcode
*/
  Matrix4D    lscale( const Matrix4D &matrix, float x, float y, float z ) const;
/*!
  Returns a Matrix4D which is composed by right hand multiplying this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix1, matrix2;
  matrix2 = rscale( matrix1, 10.0f, 1.0f, 1.0f ); // matrix2 = matrix1 * matrixScaleX
  \endcode
*/
  Matrix4D    rscale( const Matrix4D &matrix, float x, float y, float z ) const;
/*!
  Left hand multiplies this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix;
  matrix.lscale( 10.0f, 1.0f, 1.0f ); // matrix = matrixScaleX * matrix
  \endcode
*/
  Matrix4D    &lscale( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix4D matrix;
  matrix.rscale( 10.0f, 1.0f, 1.0f ); // matrix = matrix * matrixScaleX
  \endcode
*/
  Matrix4D    &rscale( float x, float y, float z );
/*!
  Modifies this matrix in a way, that the z axis is pointing along the difference between Vector3D(\em x,\em y,\em z ) and the translational component of this matris.

  \code
  Matrix4D matrix( 0.0f, 0.0f, 1.0f );
  matrix.pointAt( 1.0f, 0.0f, 1.0f );
  \endcode
*/
  Matrix4D    &pointAt( float x, float y, float z );
/*!
  Modifies this matrix in a way, that the z axis is pointing along the difference between Vector3D(\em x,\em y,\em z ) and the translational component of this matris.

  \code
  Matrix4D matrix( 0.0f, 0.0f, 1.0f );
  Vector3D v( 1.0f, 0.0f, 1.0f );
  matrix.pointAt( v );
  \endcode
*/
  Matrix4D    &pointAt( const Vector3D &v );
/*!
  Determines the rotation angles of the rotational component of the matrix.

  The angles can be used to construct a rotation matrix.

  \code
  Matrix4D matrix;
  matrix.lrotate( 1.0f, 1.5f, -3.5f );
  float rx, ry, rz;
  matrix.rotationAngles( rx, ry, rz ); // rx = 1.0, ry = 1.5, rz = -3.5
  \endcode
*/
  void          rotationAngles( float &rotx, float &roty, float &rotz ) const;
/*!
  Matrix vector Multiplication, where only the rotational part of the matrix is taken into account. The vector is not translated.

  The result vector is calculated by:
  \code
  return( Vector3D( 
    x.x*v.x + y.x*v.y + z.x*v.z,
    x.y*v.x + y.y*v.y + z.y*v.z,
    x.z*v.x + y.z*v.y + z.z*v.z
    )
  );
  \endcode
*/
  Vector3D    rotateVector3D( const Vector3D &m ) const;
/*!
  Returns a matrix, composed by multiplying only the rotational parts of this matrix with the given matrix. The Matrix is not translated.

  The result matrix is calculated by:
  \code
  Matrix4D m; 
  m.x = matrix.rotateVector3D( x );
  m.y = matrix.rotateVector3D( y );
  m.z = matrix.rotateVector3D( z );
  return( m );
  \endcode
*/
  Matrix4D    rotate( const Matrix4D &matrix ) const;
/*!
  Returns a matrix, composed by multiplying only the rotational parts of this matrix with the given matrix. The Matrix is not translated. (result = matrix * this )

  The result matrix is calculated by:
  \code
  Matrix4D m; 
  m.x = matrix.rotateVector3D( x );
  m.y = matrix.rotateVector3D( y );
  m.z = matrix.rotateVector3D( z );
  return( m );
  \endcode
*/
  Matrix4D    rotate( const Matrix3H &matrix ) const;
  Vector2D    mult2D( const Vector3D &v ) const;
/*!
  Returns the z component if the vector resulting from the matrix vector multiplication.
  \code
  Matrix4D m; 
  Vector3D v0, v1;

  v1 = m * v0; // v.z equals m.multZ(v) 
  \endcode
*/
  double        multZ( const Vector3D &v ) const;

/*!
  Returns true, if the matrix is the identity matrix, otherwise returns false.
*/
  bool 		isIdentity() const;

/*!
  Adds matrix \em m componentwise to this matrix.
*/
  Matrix4D &operator +=( const Matrix4D &m );
/*!
  Subtracts matrix \em m componentwise from this matrix.
*/
  Matrix4D &operator -=( const Matrix4D &m );
/*!
  Adds vector \em v componentwise to the w (the translational) component of this matrix.
*/
  Matrix4D &operator +=( const Vector3D &v );
/*!
  Subtracts vector \em v componentwise from the w (the translational) component of this matrix.
*/
  Matrix4D &operator -=( const Vector3D &v );
/*!
  Multiplies this matrix with matrix \em m (this=this*m).
*/
  Matrix4D &operator *=( const Matrix4D &m );
/*!
  Multiplies this matrix with matrix \em m (this=this*m).
*/
  Matrix4D &operator *=( const Matrix3H &m );
/*!
  Returns the componentwise sum of this matrix and matrix \em m.
*/
  Matrix4D  operator + ( const Matrix4D &m ) const;
/*!
  Returns the componentwise difference between this matrix and matrix \em m.
*/
  Matrix4D  operator - ( const Matrix4D &m ) const;
/*!
  Returns this matrix with vector \em v added to the w (the translational) component of this matrix.
*/
  Matrix4D  operator + ( const Vector3D &v ) const;
/*!
  Returns this matrix with vector \em v subtracted from the w (the translational) component of this matrix.
*/
  Matrix4D  operator - ( const Vector3D &v ) const;
/*!
  Returns the matrix resulting from this matrix multiplied with matrix \em m.
*/
  Matrix4D  operator * ( const Matrix4D &m ) const;
/*!
  Returns the matrix resulting from this matrix multiplied with matrix \em m.
*/
  Matrix4D  operator * ( const Matrix3H &m ) const;
/*!
  Returns this matrix multiplied with vector \em v.
*/
  Vector4D  operator * ( const Vector4D &v ) const;
/*!
  Returns this matrix multiplied with vector \em v.
*/
  Vector3D  operator * ( const Vector3D &v ) const;
/*!
  Returns this matrix multiplied with vector \em v.
*/
  Vector2D  operator * ( const Vector2D &v ) const;
/*!
  Returns this matrix with all components multiplied with -1.
  \code
    return( Matrix4D( -x, -y, -z, -w ) );
  \endcode
*/
  Matrix4D  operator - () 			 const;
/*!
  Multiplies all components of this matrix with \em c.
*/
  Matrix4D &operator *=( double c      );
/*!
  Devides all components of this matrix with \em c.
*/
  Matrix4D &operator /=( double c      );
/*!
  Returns true if all components of matrix \em m equal the components of this matrix, otherwise returns false.
*/
  bool       operator  ==( const Matrix4D &m ) const;
/*!
  Returns true if one of the components of matrix \em m and this matrix are not equal, otherwise returns false.
*/
  bool       operator  !=( const Matrix4D &m ) const;
  
};

/***************************************************************************
*** 
*** Matrix4D  inlines
***
****************************************************************************/

inline 
Matrix4D::Matrix4D( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz  )
  : x(vx,0.0), y(vy,0.0), z(vz,0.0), w(0.0,1.0)
{}

inline 
Matrix4D::Matrix4D( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz, const Vector3D &vw  )
  : x(vx,0.0), y(vy,0.0), z(vz,0.0), w(vw,1.0)
{}

inline 
Matrix4D::Matrix4D( const Vector4D &vx, const Vector4D &vy, const Vector4D &vz, const Vector4D &vw  )
  : x(vx), y(vy), z(vz), w(vw)
{}

inline 
Matrix4D::Matrix4D()
  : x(1.0,0.0,0.0,0.0), 
    y(0.0,1.0,0.0,0.0), 
    z(0.0,0.0,1.0,0.0), 
    w(0.0,0.0,0.0,1.0)
{}

inline 
Matrix4D::Matrix4D( float xx, float xy, float xz, float xw,
			float yx, float yy, float yz, float yw,
			float zx, float zy, float zz, float zw,
			float wx, float wy, float wz, float ww ) 
  : x(xx, xy, xz, xw),
    y(yx, yy, yz, yw),
    z(zx, zy, zz, zw),
    w(wx, wy, wz, ww) 
{}

inline 
Matrix4D::Matrix4D( float tx, float ty, float tz, float tw ) 
  : x(1.0,0.0,0.0,0.0), 
    y(0.0,1.0,0.0,0.0), 
    z(0.0,0.0,1.0,0.0), 
    w(tx, ty, tz, tw) 
{}

    
inline void
Matrix4D::id()
{ *this = Matrix4D(); 
}

inline bool
Matrix4D::isNull() const
{ return( x.isNull() && y.isNull() && z.isNull() && w.isNull() );
}

inline bool
Matrix4D::isId() const
{ return( x.x == 1 && x.y == 0 && x.z == 0 && x.w == 0 &&
	  y.x == 0 && y.y == 1 && y.z == 0 && y.w == 0 &&
	  z.x == 0 && z.y == 0 && z.z == 1 && z.w == 0 &&
	  w.x == 0 && w.y == 0 && w.z == 0 && w.w == 1
	  );
}


inline Matrix4D 
Matrix4D::rotate( const Matrix4D &matrix ) const
{ 
  return(
    Matrix4D( matrix.rotateVector3D( x ),
		matrix.rotateVector3D( y ),
		matrix.rotateVector3D( z ) )
    );
}

inline Matrix4D 
Matrix4D::lrotate( const Matrix4D &matrix, float x, float y, float z ) const
{ Matrix4D m( matrix ); 
  return( m.lrotate( x, y, z ) );
}

inline Matrix4D 
Matrix4D::rrotate( const Matrix4D &matrix, float x, float y, float z ) const
{ Matrix4D m( matrix ); 
  return( m.rrotate( x, y, z ) );
}

inline Matrix4D
Matrix4D::ltranslate( const Matrix4D &matrix, float x, float y, float z ) const
{ Matrix4D m( matrix ); 
  return( m.ltranslate( x, y, z ) );
}

inline Matrix4D
Matrix4D::ltranslate( const Matrix4D &matrix, const Vector3D &v ) const
{ return( ltranslate( matrix, v.x, v.y, v.z ) );
}

inline Matrix4D &
Matrix4D::ltranslate( const Vector3D &v )
{ return( ltranslate( v.x, v.y, v.z ) );
}

inline Matrix4D &
Matrix4D::ltranslateX( float X )
{ x.x += X * x.w;
  y.x += X * y.w;
  z.x += X * z.w;
  w.x += X * w.w;
return( *this );
}

inline Matrix4D &
Matrix4D::ltranslateY( float Y )
{ x.y += Y * x.w;
  y.y += Y * y.w;
  z.y += Y * z.w;
  w.y += Y * w.w;
  return( *this );
}

inline Matrix4D &
Matrix4D::ltranslateZ( float Z )
{ x.z += Z * x.w;
  y.z += Z * y.w;
  z.z += Z * z.w;
  w.z += Z * w.w;
  return( *this );
}

inline Matrix4D
Matrix4D::rtranslate( const Matrix4D &matrix, float x, float y, float z ) const
{ Matrix4D m( matrix ); 
  return( m.rtranslate( x, y, z ) );
}

inline Matrix4D
Matrix4D::rtranslate( const Matrix4D &matrix, const Vector3D &v ) const
{ return( rtranslate( matrix, v.x, v.y, v.z ) );
}

inline Matrix4D &
Matrix4D::rtranslate( const Vector3D &v )
{ return( rtranslate( v.x, v.y, v.z ) );
}


inline bool 
Matrix4D::operator ==( const Matrix4D &m ) const
{ return( m.x==x && m.y==y && m.z==z && m.w==w );
}

inline bool 
Matrix4D::operator !=( const Matrix4D &m ) const
{ return( m.x!=x || m.y!=y || m.z!=z || m.w!=w );
}

inline Vector4D &
Matrix4D::operator [] ( int index ) const 
{ return( ((Vector4D*)this)[index] ); 
}

inline 
Matrix4D::operator const float*() 
{ return( (float*)this );
}

inline 
Matrix4D::operator float*() 
{ return( (float*)this );
}

inline Matrix4D &
Matrix4D::operator +=( const Matrix4D &m )
{ x += m.x;
  y += m.y;
  z += m.z;
  w += m.w;
  return( *this );
}

inline Matrix4D &
Matrix4D::operator -=( const Matrix4D &m )
{ x -= m.x;
  y -= m.y;
  z -= m.z;
  w -= m.w;
  return( *this );
}

inline Matrix4D &
Matrix4D::operator +=( const Vector3D &v )
{ w.x += v.x;
  w.y += v.y;
  w.z += v.z;
  return( *this );
}

inline Matrix4D &
Matrix4D::operator -=( const Vector3D &v )
{ w.x -= v.x;
  w.y -= v.y;
  w.z -= v.z;
  return( *this );
}

inline Matrix4D &
Matrix4D::operator *=( double c )
{ x *= c;
  y *= c;
  z *= c;
  return( *this );
}

inline Matrix4D &
Matrix4D::operator *=( const Matrix4D &m )
{ return( *this = *this * m );
}


inline Matrix4D &
Matrix4D::operator /=( double c )
{ x /= c;
  y /= c;
  z /= c;
  return( *this );
}

inline Matrix4D 
Matrix4D::operator +( const Matrix4D &m ) const
{ return( Matrix4D( x+m.x, y+m.y, z+m.z, w+m.w ) );
}

inline Matrix4D 
Matrix4D::operator -( const Matrix4D &m ) const
{ return( Matrix4D( x-m.x, y-m.y, z-m.z, w-m.w ) );
}

inline Matrix4D 
Matrix4D::operator +( const Vector3D &v ) const
{ return( Matrix4D( x, y, z, Vector4D( w.x+v.x, w.y+v.y, w.z+v.z, w.w ) ) );
}

inline Matrix4D 
Matrix4D::operator -( const Vector3D &v ) const
{ return( Matrix4D( x, y, z, Vector4D( w.x-v.x, w.y-v.y, w.z-v.z, w.w ) ) );
}

inline Matrix4D 
Matrix4D::operator -() const
{ return( Matrix4D( -x, -y, -z, -w ) );
}

inline Matrix4D
Matrix4D::transpose() const
{ return(
    Matrix4D(
    x.x, y.x, z.x, w.x,
    x.y, y.y, z.y, w.y,
    x.z, y.z, z.z, w.z,
    x.w, y.w, z.w, w.w
    )
  );
}

inline Matrix4D
Matrix4D::transposeMatrix3D() const
{ return(
    Matrix4D(
    x.x, y.x, z.x, x.w,
    x.y, y.y, z.y, y.w,
    x.z, y.z, z.z, z.w,
    w.x, w.y, w.z, w.w
    )
  );
}


inline Matrix4D
Matrix4D::rscale( const Matrix4D &matrix, float x, float y, float z ) const
{
  Matrix4D res = matrix;
  res.x *= x;
  res.y *= y;
  res.z *= z;
  return( res );
}


inline Matrix4D &
Matrix4D::rscale( float X, float Y, float Z )
{ x *= X;
  y *= Y;
  z *= Z;
  return( *this );
}

inline Matrix4D &
Matrix4D::pointAt( const Vector3D &v )
{ return( pointAt( v.x, v.y, v.z ) );
}

inline bool
Matrix4D::isIdentity() const
{ 
  return( w.x == 0.0 &&
	  w.y == 0.0 &&
	  w.z == 0.0 &&
	  x.x == 1.0 &&
	  y.y == 1.0 &&
	  z.z == 1.0 &&
	  w.w == 1.0 &&
	  x.y == 0.0 &&
	  x.z == 0.0 &&
	  x.w == 0.0 &&
	  y.x == 0.0 &&
	  y.z == 0.0 &&
	  y.w == 0.0 &&
	  z.x == 0.0 &&
	  z.y == 0.0 &&
	  z.w == 0.0 );
}

/***************************************************************************
*** 
*** Matrix3H
***
****************************************************************************/

class Matrix3H
{
public:

  Vector3D x; //!< x component row vector of the matrix
  Vector3D y; //!< y component row vector of the matrix
  Vector3D z; //!< z component row vector of the matrix
  Vector3D w; //!< w component row vector of the matrix

/*!
  Constructs a identity Matrix3H.
  \code
    Matrix3H matrix; // constructs an identity matrix
  \endcode
*/
  Matrix3H();
/*!
  Constructs a Matrix3H as a copy of \em m .
  \code
    Matrix3H matrix1; // constructs an identity matrix
    Matrix4D3H matrix2( matrix1 ); // constructs a matrix from the given matrix
  \endcode
*/
  Matrix3H( const Matrix3H &other );
/*!
  Constructs a Matrix3H with x=\em vx, y=\em vy, z=\em vz and w=(0,0,0).
*/
  Matrix3H( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz  );
/*!
  Constructs a Matrix3H with x=\em vx, y=\em vy, z=\em vz and w=vw.
*/
  Matrix3H( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz, const Vector3D &vw  );
/*!
  Constructs a Matrix3H with x=(1,0,0), y=(0,1,0), z=(0,0,1) and w=translation.
*/
  Matrix3H( const Vector3D &translation );
/*!
  Constructs a Matrix3H as a copy of \em m with x=(\em m.x,0), y=(\em m.y,0), z=(\em m.z,0) and w=(0,0,0).
  \code
    Matrix3D matrix1; // constructs an identity matrix
    Matrix4D matrix2( matrix1 ); // constructs a matrix from the given matrix
  \endcode
*/
  Matrix3H( const Matrix3D &m );
/*!
  Constructs a Matrix3H as a copy of \em m with x=(\em m.x,v.x), y=(\em m.y,v.y), z=(\em m.z,v.z) and w=(0,0,0).
  \code
    Matrix3D matrix1; // constructs an identity matrix
    Matrix4D matrix2( matrix1 ); // constructs a matrix from the given matrix
  \endcode
*/
  Matrix3H( const Matrix3D &m, const Vector3D &v );
/*!
  Constructs a Matrix3H with x=(1,0,0), y=(0,1,0), z=(0,0,1) and w=(\em tx,\em ty,\em tz).
*/
  Matrix3H( float tx, float ty, float tz );
/*!
  Constructs a Matrix3H from a float matrix.
*/
  Matrix3H( const float m[3][4]  );
/*!
  Constructs a Matrix3H from float values in row order.
  \code
    Matrix3H matrix( 1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 1.0f,
                       1.0f, 0.0f, 1.0f );
  // constructs a translational matrix with x=(1,0,0), y=(0,1,0), z=(0,0,1), w=(1,0,1)
  \endcode
*/
  Matrix3H( float xx, float xy, float xz,
	      float yx, float yy, float yz,
	      float zx, float zy, float zz,
	      float wx, float wy, float wz );

/*!
  Assigns a Matrix3D \em to this matrix.
*/
  Matrix3H &operator = ( const Matrix3D &m );
/*!
  Assigns a Matrix4D \em to this matrix.
*/
  Matrix3H &operator = ( const Matrix4D &m );
/*!
  Returns the n'th component of the matrix, where component is x with \em index = 0, y with \em index = 1, z with \em index = 2 and w with \em index = 3. The returned component is a Vector3D.
*/
  Vector3D &operator [] ( int index ) const;
/*!
  Converts the Matrix3H to a Matrix4D.
*/
  operator Matrix4D() { return( Matrix4D( x, y, z, w ) ); };
//  operator Matrix3D();
/*!
  Returns a pointer to the first float of the x component
*/
  operator float*();
/*!
  Returns a const pointer to the first float of the x component
*/
  operator const float*();
  
/*!
  Returns true, if all the components of this matrix are zero.
*/
  bool 		isNull() 			const;
/*!
  Returns true, if the matrix is the identity matrix, otherwise returns false.
*/
  bool 		isId()				const;
/*!
  Sets all values to make a identity matrix.
*/
  void 		id();
/*!
  Prints the components of this matrix to the standart output. The dump is preceeded by the text \em text
*/
  void 		print( const char *text="" )   	const;
/*!
  Returns the inverse Matrix3H.
  \code
  Matrix3H m; 
  Matrix3H inverse = m.inverse();

  // inverse * m = m * inverse = id
  \endcode
*/
  Matrix3H    inverse() 	const;
/*!
  Returns the z component of the inverse Matrix3H.
  \code
  Matrix3H m; 
  Matrix3H inverse = m.inverse(); // inverse.z equals m.inverseZ()
  \endcode
*/
  Vector3D    inverseZ() 	const;
/*!
  Returns a transposed Matrix3H, where only the rotational components are transposed, and the w component is set to zero.

  Here is how its computed:
  \code
  return(
    Matrix3H(
    x.x, y.x, z.x,
    x.y, y.y, z.y,
    x.z, y.z, z.z,
    0.0, 0.0, 0.0
    )
  );
  \endcode
*/
  Matrix3H    transpose() 			const;
/*!
  Returns a Matrix3H which is composed by left hand multiplying this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3H matrix1, matrix2;
  matrix2 = lrotate( matrix1, M_PI_2, 0.0f, 0.0f ); // matrix2 = matrixRotateXM_PI_2 * matrix1
  \endcode
*/
  Matrix3H	lrotate( const Matrix3H &matrix, float x, float y, float z ) const;
/*!
  Returns a Matrix3H which is composed by right hand multiplying this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3H matrix1, matrix2;
  matrix2 = matrix1.rrotate( matrix1, M_PI_2, 0.0f, 0.0f ); // matrix2 = matrix1 * matrixRotateXM_PI_2
  \endcode
*/
  Matrix3H	rrotate( const Matrix3H &matrix, float x, float y, float z ) const;
/*!
  Left hand multiplies this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3H matrix;
  matrix.lrotate( M_PI_2, 0.0f, 0.0f ); // matrix = matrixRotateXM_PI_2 * matrix
  \endcode
*/
  Matrix3H	&lrotate( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3H matrix;
  matrix.rrotate( M_PI_2, 0.0f, 0.0f ); // matrix = matrix * matrixRotateXM_PI_2
  \endcode
*/
  Matrix3H	&rrotate( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the x axis.

  \code
  Matrix3H matrix;
  matrix.rrotateX( M_PI_2 ); // matrix = matrix * matrixRotateXM_PI_2
  \endcode
*/
  Matrix3H	&rrotateX( float x );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the y axis.

  \code
  Matrix3H matrix;
  matrix.rrotateY( M_PI_2 ); // matrix = matrix * matrixRotateYM_PI_2
  \endcode
*/
  Matrix3H	&rrotateY( float y );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the z axis.

  \code
  Matrix3H matrix;
  matrix.rrotateZ( M_PI_2 ); // matrix = matrix * matrixRotateZM_PI_2
  \endcode
*/
  Matrix3H	&rrotateZ( float z );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the x axis.

  \code
  Matrix3H matrix;
  matrix.lrotateX( M_PI_2 ); // matrix = matrixRotateXM_PI_2 * matrix
  \endcode
*/
  Matrix3H	&lrotateX( float x );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the y axis.

  \code
  Matrix3H matrix;
  matrix.lrotateY( M_PI_2 ); // matrix = matrixRotateYM_PI_2 * matrix
  \endcode
*/
  Matrix3H	&lrotateY( float y );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the Z axis.

  \code
  Matrix3H matrix;
  matrix.lrotateZ( M_PI_2 ); // matrix = matrixRotateZM_PI_2 * matrix
  \endcode
*/
  Matrix3H	&lrotateZ( float z );
/*!
  Returns a Matrix3H which is composed by left hand multiplying this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix1, matrix2;
  matrix2 = ltranslate( matrix1, 1.0f, 0.0f, 0.0f ); // matrix2 = matrixTranslateX * matrix1
  \endcode
*/
  Matrix3H	ltranslate( const Matrix3H &matrix, float x, float y, float z ) const;
/*!
  Returns a Matrix3H which is composed by right hand multiplying this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix1, matrix2;
  matrix2 = rtranslate( matrix1, 1.0f, 0.0f, 0.0f ); // matrix2 = matrix1 * matrixTranslateX
  \endcode
*/
  Matrix3H	rtranslate( const Matrix3H &matrix, float x, float y, float z ) const;
/*!
  Left hand multiplies this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix;
  matrix.ltranslate( 1.0f, 0.0f, 0.0f ); // matrix = matrixTranslateX * matrix
  \endcode
*/
  Matrix3H	&ltranslate( float x, float y, float z );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translations \em v.x, \em v.y, \em v.z along the x, y and z axis.

  \code
  Matrix3H matrix;
  Vector3D v( 1.0f, 0.0f, 0.0f );
  matrix.ltranslate( v ); // matrix = matrixTranslateVX * matrix
  \endcode
*/
  Matrix3H	&ltranslate( const Vector3D &v );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translation \em x along the x axis.

  \code
  Matrix3H matrix;
  matrix.ltranslateX( 1.0f ); // matrix = matrixTranslateX * matrix
  \endcode
*/
  Matrix3H	&ltranslateX( float x );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translation \em y along the y axis.

  \code
  Matrix3H matrix;
  matrix.ltranslateY( 1.0f ); // matrix = matrixTranslateY * matrix
  \endcode
*/
  Matrix3H	&ltranslateY( float y );
/*!
  Left hand multiplies this matrix with a translation matrix build out of translation \em z along the z axis.

  \code
  Matrix3H matrix;
  matrix.ltranslateZ( 1.0f ); // matrix = matrixTranslateZ * matrix
  \endcode
*/
  Matrix3H	&ltranslateZ( float z );
/*!
  Right hand multiplies this matrix with a translation matrix build out of translations \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix;
  matrix.rtranslate( 1.0f, 0.0f, 0.0f ); // matrix = matrix * matrixTranslateX
  \endcode
*/
  Matrix3H	&rtranslate( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a translation matrix build out of translations \em v.x, \em v.y, \em v.z along the x, y and z axis.

  \code
  Matrix3H matrix;
  Vector3D v( 1.0f, 0.0f, 0.0f );
  matrix.rtranslate( v ); // matrix = matrix * matrixTranslateVX
  \endcode
*/
  Matrix3H	&rtranslate( const Vector3D &v );
/*!
  Right hand multiplies this matrix with a translation matrix build out of translation \em x along the x axis.

  \code
  Matrix3H matrix;
  matrix.rtranslateX( 1.0f ); // matrix = matrix * matrixTranslateX
  \endcode
*/
  Matrix3H	&rtranslateX( float x );
/*!
  Right hand multiplies this matrix with a translation matrix build out of translation \em y along the y axis.

  \code
  Matrix3H matrix;
  matrix.rtranslateY( 1.0f ); // matrix = matrix * matrixTranslateY
  \endcode
*/
  Matrix3H	&rtranslateY( float y );
/*!
  Right hand multiplies this matrix with a translation matrix build out of translation \em z along the z axis.

  \code
  Matrix3H matrix;
  matrix.rtranslateZ( 1.0f ); // matrix = matrix * matrixTranslateZ
  \endcode
*/
  Matrix3H	&rtranslateZ( float z );
/*!
  Returns a Matrix3H which is composed by right hand multiplying this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix1, matrix2;
  matrix2 = rscale( matrix1, 10.0f, 1.0f, 1.0f ); // matrix2 = matrix1 * matrixScaleX
  \endcode
*/
  Matrix3H    rscale( const Matrix3H &matrix, float x, float y, float z ) const;
/*!
  Returns a Matrix3H which is composed by left hand multiplying this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix1, matrix2;
  matrix2 = lscale( matrix1, 10.0f, 1.0f, 1.0f ); // matrix2 = matrixScaleX * matrix1
  \endcode
*/
  Matrix3H    lscale( const Matrix3H &matrix, float x, float y, float z ) const;
/*!
  Right hand multiplies this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix;
  matrix.rscale( 10.0f, 1.0f, 1.0f ); // matrix = matrix * matrixScaleX
  \endcode
*/
  Matrix3H    &rscale ( float x, float y, float z );
/*!
  Left hand multiplies this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3H matrix;
  matrix.lscale( 10.0f, 1.0f, 1.0f ); // matrix = matrixScaleX * matrix
  \endcode
*/
  Matrix3H    &lscale ( float x, float y, float z );
/*!
  Modifies this matrix in a way, that the z axis is pointing along the difference between Vector3D(\em x,\em y,\em z ) and the translational component of this matris.

  \code
  Matrix3H matrix( 0.0f, 0.0f, 1.0f );
  matrix.pointAt( 1.0f, 0.0f, 1.0f );
  \endcode
*/
  Matrix3H    &pointAt( float x, float y, float z );
/*!
  Modifies this matrix in a way, that the z axis is pointing along the difference between Vector3D(\em x,\em y,\em z ) and the translational component of this matris.

  \code
  Matrix3H matrix( 0.0f, 0.0f, 1.0f );
  Vector3D v( 1.0f, 0.0f, 1.0f );
  matrix.pointAt( v );
  \endcode
*/
  Matrix3H    &pointAt( const Vector3D &v );
  
/*!
  Determines the rotation angles of the rotational component of the matrix.

  The angles can be used to construct a rotation matrix.

  \code
  Matrix3H matrix;
  matrix.lrotate( 1.0f, 1.5f, -3.5f );
  float rx, ry, rz;
  matrix.rotationAngles( rx, ry, rz ); // rx = 1.0, ry = 1.5, rz = -3.5
  \endcode
*/
  void          rotationAngles( float &rotx, float &roty, float &rotz ) const;
/*!
  Matrix vector Multiplication, where only the rotational part of the matrix is taken into account. The vector is not translated.

  The result vector is calculated by:
  \code
  return( Vector3D( 
    x.x*v.x + y.x*v.y + z.x*v.z,
    x.y*v.x + y.y*v.y + z.y*v.z,
    x.z*v.x + y.z*v.y + z.z*v.z
    )
  );
  \endcode
*/
  Vector3D    rotateVector3D( const Vector3D &m ) const;
/*!
  Returns a matrix, composed by multiplying only the rotational parts of this matrix with the given matrix. The Matrix is not translated. (result = matrix * this )

  The result matrix is calculated by:
  \code
  Matrix3H m; 
  m.x = matrix.rotateVector3D( x );
  m.y = matrix.rotateVector3D( y );
  m.z = matrix.rotateVector3D( z );
  return( m );
  \endcode
*/
  Matrix3H    rotate( const Matrix3H &matrix ) const;
/*!
  Returns true, if the matrix is the identity matrix, otherwise returns false.
*/
  bool 		isIdentity() const;

/*!
  Adds matrix \em m componentwise to this matrix.
*/
  Matrix3H &operator +=( const Matrix3H &m );
/*!
  Subtracts matrix \em m componentwise from this matrix.
*/
  Matrix3H &operator -=( const Matrix3H &m );
/*!
  Adds vector \em v componentwise to the w (the translational) component of this matrix.
*/
  Matrix3H &operator +=( const Vector3D &v );
/*!
  Subtracts vector \em v componentwise from the w (the translational) component of this matrix.
*/
  Matrix3H &operator -=( const Vector3D &v );
/*!
  Multiplies this matrix with matrix \em m (this=this*m).
*/
  Matrix3H &operator *=( const Matrix3H &m );
/*!
  Multiplies this matrix with matrix \em m (this=this*m).
*/
  Matrix3H &operator *=( const Matrix4D &m );
/*!
  Returns the componentwise sum of this matrix and matrix \em m.
*/
  Matrix3H  operator + ( const Matrix3H &m ) const;
/*!
  Returns the componentwise difference between this matrix and matrix \em m.
*/
  Matrix3H  operator - ( const Matrix3H &m ) const;
/*!
  Returns this matrix with vector \em v added to the w (the translational) component of this matrix.
*/
  Matrix3H  operator + ( const Vector3D &v ) const;
/*!
  Returns this matrix with vector \em v subtracted from the w (the translational) component of this matrix.
*/
  Matrix3H  operator - ( const Vector3D &v ) const;
/*!
  Returns the matrix resulting from this matrix multiplied with matrix \em m.
*/
  Matrix3H  operator * ( const Matrix3H &m ) const;
/*!
  Returns the matrix resulting from this matrix multiplied with matrix \em m.
*/
  Matrix4D  operator * ( const Matrix4D &m ) const;
/*!
  Returns this matrix multiplied with vector \em v.
*/
  Vector3D  operator * ( const Vector3D &m ) const;
/*!
  Returns this matrix multiplied with vector \em v.
*/
  Vector2D  operator * ( const Vector2D &m ) const;
/*!
  Returns this matrix with all components multiplied with -1.
  \code
    return( Matrix3H( -x, -y, -z, -w ) );
  \endcode
*/
  Matrix3H  operator - () 			 const;
/*!
  Multiplies all components of this matrix with \em c.
*/
  Matrix3H &operator *=( double c      );
/*!
  Devides all components of this matrix with \em c.
*/
  Matrix3H &operator /=( double c      );
/*!
  Returns true if all components of matrix \em m equal the components of this matrix, otherwise returns false.
*/
  bool       operator  ==( const Matrix3H &m ) const;
/*!
  Returns true if one of the components of matrix \em m and this matrix are not equal, otherwise returns false.
*/
  bool       operator  !=( const Matrix3H &m ) const;
  
};

/***************************************************************************
*** 
*** dependend Matrix4D  inlines
***
****************************************************************************/

inline 
Matrix4D::Matrix4D( const Matrix3H &m )
  : x(m.x,0.0), y(m.y,0.0), z(m.z,0.0), w(m.w,1.0)
{}

inline Matrix4D &
Matrix4D::operator = ( const Matrix3H &m )
{ x   = m.x;
  x.w = 0.0;
  y   = m.y;
  y.w = 0.0;
  z   = m.z;
  z.w = 0.0;
  w = Vector4D(0,0,0,1);
  return( *this );
}

inline Matrix4D &
Matrix4D::operator *=( const Matrix3H &m )
{ return( *this = *this * m );
}

inline Matrix4D 
Matrix4D::rotate( const Matrix3H &matrix ) const
{
  return(
    Matrix4D( matrix.rotateVector3D( x ),
		matrix.rotateVector3D( y ),
		matrix.rotateVector3D( z ) )
    );
}

/***************************************************************************
*** 
*** Matrix3H  inlines
***
****************************************************************************/

inline 
Matrix3H::Matrix3H( const Matrix3H &other )
  : x( other.x ),
    y( other.y ),
    z( other.z ),
    w( other.w )
{}
   

inline 
Matrix3H::Matrix3H( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz  )
  : x(vx), y(vy), z(vz), w(0.0)
{}

inline 
Matrix3H::Matrix3H( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz, const Vector3D &vw  )
  : x(vx), y(vy), z(vz), w(vw)
{}

inline 
Matrix3H::Matrix3H( const Vector3D &translation )
  : x(1.0,0.0,0.0), 
    y(0.0,1.0,0.0), 
    z(0.0,0.0,1.0),
    w(translation)
{}

inline 
Matrix3H::Matrix3H( float tx, float ty, float tz )
  : x(1.0,0.0,0.0), 
    y(0.0,1.0,0.0), 
    z(0.0,0.0,1.0),
    w(tx, ty, tz)
{}

inline 
Matrix3H::Matrix3H()
  : x(1.0,0.0,0.0), 
    y(0.0,1.0,0.0), 
    z(0.0,0.0,1.0), 
    w(0.0,0.0,0.0)
{}

inline 
Matrix3H::Matrix3H( float xx, float xy, float xz,
			float yx, float yy, float yz,
			float zx, float zy, float zz,
			float wx, float wy, float wz ) 
  : x(xx, xy, xz),
    y(yx, yy, yz),
    z(zx, zy, zz),
    w(wx, wy, wz) 
{}

    
inline 
Matrix4D::operator Matrix3H ()
{ return( Matrix3H(
          x.x, x.y, x.z,
          y.x, y.y, y.z,
          z.x, z.y, z.z,
	  w.x, w.y, w.z ) ); 
}

inline Matrix3H &
Matrix3H::operator = ( const Matrix4D &m )
{ x = m.x;
  y = m.y;
  z = m.z;
  w = m.w;
  return( *this );
}

inline 
Matrix3H::Matrix3H( const float m[3][4] )
{
  memcpy( (void *)this, (void *)m, sizeof(float[4][4]) );
}

inline void
Matrix3H::id()
{ *this = Matrix3H(); 
}

inline bool
Matrix3H::isNull() const
{ return( x.isNull() && y.isNull() && z.isNull() && w.isNull() );
}

inline bool
Matrix3H::isId() const
{ return( x.x == 1 && x.y == 0 && x.z == 0 &&
	  y.x == 0 && y.y == 1 && y.z == 0 &&
	  z.x == 0 && z.y == 0 && z.z == 1 &&
	  w.x == 0 && w.y == 0 && w.z == 0 
	  );
}


inline bool 
Matrix3H::operator ==( const Matrix3H &m ) const
{ return( m.x==x && m.y==y && m.z==z && m.w==w );
}

inline bool 
Matrix3H::operator !=( const Matrix3H &m ) const
{ return( m.x!=x || m.y!=y || m.z!=z || m.w!=w );
}

inline Vector3D &
Matrix3H::operator [] ( int index ) const 
{ return( ((Vector3D*)this)[index] ); 
}

inline 
Matrix3H::operator const float*() 
{ return( (float*)this );
}

inline 
Matrix3H::operator float*() 
{ return( (float*)this );
}

inline Matrix3H &
Matrix3H::operator +=( const Matrix3H &m )
{ x += m.x;
  y += m.y;
  z += m.z;
  w += m.w;
  return( *this );
}

inline Matrix3H &
Matrix3H::operator -=( const Matrix3H &m )
{ x -= m.x;
  y -= m.y;
  z -= m.z;
  w -= m.w;
  return( *this );
}

inline Matrix3H &
Matrix3H::operator +=( const Vector3D &v )
{ w.x += v.x;
  w.y += v.y;
  w.z += v.z;
  return( *this );
}

inline Matrix3H &
Matrix3H::operator -=( const Vector3D &v )
{ w.x -= v.x;
  w.y -= v.y;
  w.z -= v.z;
  return( *this );
}

inline Matrix3H &
Matrix3H::operator *=( double c )
{ x *= c;
  y *= c;
  z *= c;
  return( *this );
}

inline Matrix3H &
Matrix3H::operator *=( const Matrix3H &m )
{ return( *this = *this * m );
}

inline Matrix4D 
Matrix3H::operator *( const Matrix4D &m ) const
{ return( Matrix4D( *this ) * m );
}

inline Matrix3H &
Matrix3H::operator *=( const Matrix4D &m )
{ return( *this = *this * m );
}


inline Matrix3H &
Matrix3H::operator /=( double c )
{ x /= c;
  y /= c;
  z /= c;
  return( *this );
}

inline Matrix3H 
Matrix3H::operator +( const Matrix3H &m ) const
{ return( Matrix3H( x+m.x, y+m.y, z+m.z, w+m.w ) );
}

inline Matrix3H 
Matrix3H::operator -( const Matrix3H &m ) const
{ return( Matrix3H( x-m.x, y-m.y, z-m.z, w-m.w ) );
}

inline Matrix3H 
Matrix3H::operator +( const Vector3D &v ) const
{ return( Matrix3H( x, y, z, Vector3D( w.x+v.x, w.y+v.y, w.z+v.z ) ) );
}

inline Matrix3H 
Matrix3H::operator -( const Vector3D &v ) const
{ return( Matrix3H( x, y, z, Vector3D( w.x-v.x, w.y-v.y, w.z-v.z ) ) );
}

inline Matrix3H 
Matrix3H::operator -() const
{ return( Matrix3H( -x, -y, -z, -w ) );
}

inline Matrix3H
Matrix3H::transpose() const
{ return(
    Matrix3H(
    x.x, y.x, z.x,
    x.y, y.y, z.y,
    x.z, y.z, z.z,
    0.0, 0.0, 0.0
    )
  );
}

inline Matrix3H
Matrix3H::rotate( const Matrix3H &matrix ) const
{ 
  return(
    Matrix3H( matrix.rotateVector3D( x ),
		matrix.rotateVector3D( y ),
		matrix.rotateVector3D( z ) )
    );
}

inline Matrix3H 
Matrix3H::lrotate( const Matrix3H &matrix, float x, float y, float z ) const
{ Matrix3H m( matrix ); 
  return( m.lrotate( x, y, z ) );
}

inline Matrix3H
Matrix3H::rrotate( const Matrix3H &matrix, float x, float y, float z ) const
{ Matrix3H m( matrix ); 
  return( m.rrotate( x, y, z ) );
}

inline Matrix3H &
Matrix3H::ltranslate( float X, float Y, float Z )
{ w.x += X;
  w.y += Y;
  w.z += Z;
  return( *this );
}

inline Matrix3H &
Matrix3H::ltranslate( const Vector3D &v )
{ return( ltranslate( v.x, v.y, v.z ) );
}

inline Matrix3H &
Matrix3H::ltranslateX( float X )
{ w.x += X;
  return( *this );
}

inline Matrix3H &
Matrix3H::ltranslateY( float Y )
{ w.y += Y;
  return( *this );
}

inline Matrix3H &
Matrix3H::ltranslateZ( float Z )
{ w.z += Z;
  return( *this );
}

inline Matrix3H &
Matrix3H::rtranslate( float X, float Y, float Z )
{ 
  w.x += x.x * X +
         y.x * Y +
         z.x * Z;
  
  w.y += x.y * X +
         y.y * Y +
         z.y * Z;
  
  w.z += x.z * X +
         y.z * Y +
         z.z * Z;
  
  return( *this );
}



inline Matrix3H &
Matrix3H::rtranslate( const Vector3D &v )
{ return( rtranslate( v.x, v.y, v.z ) );
}

inline Matrix3H &
Matrix3H::rtranslateX( float X )
{ w.x += x.x * X;
  w.y += x.y * X;
  w.z += x.z * X;
  return( *this );
}


inline Matrix3H &
Matrix3H::rtranslateY( float Y )
{ w.x += y.x * Y;
  w.y += y.y * Y;
  w.z += y.z * Y;
  return( *this );
}


inline Matrix3H &
Matrix3H::rtranslateZ( float Z )
{ w.x += z.x * Z;
  w.y += z.y * Z;
  w.z += z.z * Z;
  return( *this );
}


inline Matrix3H 
Matrix3H::ltranslate( const Matrix3H &matrix, float x, float y, float z ) const
{ Matrix3H m(matrix);
  return( m.ltranslate( x, y, z ) );
}

inline Matrix3H 
Matrix3H::rtranslate( const Matrix3H &matrix, float x, float y, float z ) const
{ Matrix3H res( matrix );
  return( res.rtranslate( x, y, z ) );
}

inline Vector3D &
Vector3D::operator *=( const Matrix4D &m )
{ return( *this = m * (*this) );
}

inline Vector3D &
Vector3D::operator *=( const Matrix3H &m )
{ return( *this = m * (*this) );
}

inline Vector2D &
Vector2D::operator *=( const Matrix3H &m )
{ return( *this = m * (*this) );
}

inline Matrix3H
Matrix3H::rscale( const Matrix3H &matrix, float x, float y, float z ) const
{
  Matrix3H res = matrix;
  res.x *= x;
  res.y *= y;
  res.z *= z;
  return( res );
}

inline Matrix3H &
Matrix3H::rscale( float X, float Y, float Z )
{ x *= X;
  y *= Y;
  z *= Z;
  return( *this );
}

inline Matrix3H &
Matrix3H::pointAt( const Vector3D &v )
{ return( pointAt( v.x, v.y, v.z ) );
}


inline bool
Matrix3H::isIdentity() const
{ 
  return( w.x == 0.0 &&
	  w.y == 0.0 &&
	  w.z == 0.0 &&
	  x.x == 1.0 &&
	  y.y == 1.0 &&
	  z.z == 1.0 &&
	  x.y == 0.0 &&
	  y.x == 0.0 &&
	  z.x == 0.0 &&
	  x.z == 0.0 &&
	  y.z == 0.0 &&
	  z.y == 0.0 );
}

/***************************************************************************
*** 
*** Matrix3D
***
****************************************************************************/

class Matrix3D
{
public:

  Vector3D x; //!< x component row vector of the matrix
  Vector3D y; //!< y component row vector of the matrix
  Vector3D z; //!< z component row vector of the matrix

/*!
  Constructs a identity Matrix3D.
  \code
    Matrix3D matrix; // constructs an identity matrix
  \endcode
*/
  Matrix3D();
/*!
  Constructs a Matrix3D as a copy of \em m.
  \code
    Matrix3D matrix1; // constructs an identity matrix
    Matrix3D matrix2( matrix1 ); // constructs a matrix from the given matrix
  \endcode
*/
  Matrix3D( const Matrix3D &other );
/*!
  Constructs a Matrix3D with x=\em vx, y=\em vy, z=\em vz.
*/
  Matrix3D( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz  );
/*!
  Constructs a Matrix3D from a float matrix.
*/
  Matrix3D( const float m[3][3]  );
/*!
  Constructs a Matrix3D from float values in row order.
  \code
    Matrix3D matrix( 1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 1.0f );
  // constructs a translational matrix with x=(1,0,0), y=(0,1,0), z=(0,0,1)
  \endcode
*/
  Matrix3D( float xx, float xy, float xz,
	      float yx, float yy, float yz,
	      float zx, float zy, float zz );

/*!
  Constructs a Matrix3D as a copy of \em m's rotational components.
*/
  Matrix3D( const Matrix3H &m );
/*!
  Constructs a Matrix3D as a copy of \em m's rotational components.
*/
  Matrix3D( const Matrix4D &m );

/*!
  Assigns a Matrix3D \em to this matrix.
*/
  Matrix3D &operator = ( const Matrix3H &m );
/*!
  Assigns a Matrix4D \em to this matrix.
*/
  Matrix3D &operator = ( const Matrix4D &m );
/*!
  Returns the n'th component of the matrix, where component is x with \em index = 0, y with \em index = 1 and z with \em index = 2. The returned component is a Vector3D.
*/
  Vector3D &operator [] ( int index ) const;
/*!
  Converts the Matrix3H to a Matrix4D.
*/
  operator Matrix4D() { return( Matrix4D( x, y, z ) ); };
/*!
  Returns a pointer to the first float of the x component
*/
  operator float*();
/*!
  Returns a const pointer to the first float of the x component
*/
  operator const float*();
  
/*!
  Sets all values to make a identity matrix.
*/
  void 		id();
/*!
  Returns true, if all the components of this matrix are zero.
*/
  bool 		isNull() 			const;
/*!
  Returns true, if the matrix is the identity matrix, otherwise returns false.
*/
  bool 		isId()				const;
/*!
  Prints the components of this matrix to the standart output. The dump is preceeded by the text \em text
*/
  void 		print( const char *text="" )   	const;
/*!
  Returns the inverse Matrix3D.
  \code
  Matrix3D m; 
  Matrix3D inverse = m.inverse();

  // inverse * m = m * inverse = id
  \endcode
*/
  Matrix3D    inverse() 	const;
/*!
  Returns a transposed Matrix3H, where only the rotational components are transposed, and the w component is set to zero.

  Here is how its computed:
  \code
  return(
    Matrix3D(
    x.x, y.x, z.x,
    x.y, y.y, z.y,
    x.z, y.z, z.z
    )
  );
  \endcode
*/
  Matrix3D    transpose() 			const;
/*!
  Left hand multiplies this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3D matrix;
  matrix.lrotate( M_PI_2, 0.0f, 0.0f ); // matrix = matrixRotateXM_PI_2 * matrix
  \endcode
*/
  Matrix3D	&lrotate( float x, float y, float z );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3D matrix;
  matrix.rrotate( M_PI_2, 0.0f, 0.0f ); // matrix = matrix * matrixRotateXM_PI_2
  \endcode
*/
  Matrix3D	&rrotate( float x, float y, float z );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the x axis.

  \code
  Matrix3D matrix;
  matrix.lrotateX( M_PI_2 ); // matrix = matrixRotateXM_PI_2 * matrix
  \endcode
*/
  Matrix3D	&lrotateX( float x );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the y axis.

  \code
  Matrix3D matrix;
  matrix.lrotateY( M_PI_2 ); // matrix = matrixRotateYM_PI_2 * matrix
  \endcode
*/
  Matrix3D	&lrotateY( float y );
/*!
  Left hand multiplies this matrix with a rotation matrix build out of a rotation around the Z axis.

  \code
  Matrix3D matrix;
  matrix.lrotateZ( M_PI_2 ); // matrix = matrixRotateZM_PI_2 * matrix
  \endcode
*/
  Matrix3D	&lrotateZ( float z );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the x axis.

  \code
  Matrix3D matrix;
  matrix.rrotateX( M_PI_2 ); // matrix = matrix * matrixRotateXM_PI_2
  \endcode
*/
  Matrix3D	&rrotateX( float x );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the y axis.

  \code
  Matrix3D matrix;
  matrix.rrotateY( M_PI_2 ); // matrix = matrix * matrixRotateYM_PI_2
  \endcode
*/
  Matrix3D	&rrotateY( float y );
/*!
  Right hand multiplies this matrix with a rotation matrix build out of a rotation around the z axis.

  \code
  Matrix3D matrix;
  matrix.rrotateZ( M_PI_2 ); // matrix = matrix * matrixRotateZM_PI_2
  \endcode
*/
  Matrix3D	&rrotateZ( float z );
/*!
  Returns a Matrix3D which is composed by left hand multiplying this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3D matrix1, matrix2;
  matrix2 = lrotate( matrix1, M_PI_2, 0.0f, 0.0f ); // matrix2 = matrixRotateXM_PI_2 * matrix1
  \endcode
*/
  Matrix3D	lrotate( const Matrix3D &matrix, float x, float y, float z ) const;
/*!
  Returns a Matrix3D which is composed by right hand multiplying this matrix with a rotation matrix build out of rotations \em x, \em y, \em z around the x, y and z axis.

  \code
  Matrix3D matrix1, matrix2;
  matrix2 = matrix1.rrotate( matrix1, M_PI_2, 0.0f, 0.0f ); // matrix2 = matrix1 * matrixRotateXM_PI_2
  \endcode
*/
  Matrix3D	rrotate( const Matrix3D &matrix, float x, float y, float z ) const;
/*!
  Returns a Matrix3D which is composed by right hand multiplying this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3D matrix1, matrix2;
  matrix2 = rscale( matrix1, 10.0f, 1.0f, 1.0f ); // matrix2 = matrix1 * matrixScaleX
  \endcode
*/
  Matrix3D    rscale ( const Matrix3D &matrix, float x, float y, float z ) const;
/*!
  Returns a Matrix3D which is composed by left hand multiplying this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3D matrix1, matrix2;
  matrix2 = lscale( matrix1, 10.0f, 1.0f, 1.0f ); // matrix2 = matrixScaleX * matrix1
  \endcode
*/
  Matrix3D    lscale ( const Matrix3D &matrix, float x, float y, float z ) const;
/*!
  Right hand multiplies this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3D matrix;
  matrix.rscale( 10.0f, 1.0f, 1.0f ); // matrix = matrix * matrixScaleX
  \endcode
*/
  Matrix3D    &rscale( float x, float y, float z );
/*!
  Left hand multiplies this matrix with a scale matrix build out of scale factors \em x, \em y, \em z along the x, y and z axis.

  \code
  Matrix3D matrix;
  matrix.lscale( 10.0f, 1.0f, 1.0f ); // matrix = matrixScaleX * matrix
  \endcode
*/
  Matrix3D    &lscale( float x, float y, float z );
  
/*!
  Determines the rotation angles of the rotational component of the matrix.

  The angles can be used to construct a rotation matrix.

  \code
  Matrix3D matrix;
  matrix.lrotate( 1.0f, 1.5f, -3.5f );
  float rx, ry, rz;
  matrix.rotationAngles( rx, ry, rz ); // rx = 1.0, ry = 1.5, rz = -3.5
  \endcode
*/
  void          rotationAngles( float &rotx, float &roty, float &rotz ) const;
/*!
  Returns true, if the matrix is the identity matrix, otherwise returns false.
*/
  bool 		isIdentity() const;

/*!
  Returns this matrix transposed multiplied with vector \em v.

  Equals matrix.transpose() * v without changing matrix.
  
*/
  Vector3D    transposeMult( const Vector3D &v ) const;

/*!
  Adds matrix \em m componentwise to this matrix.
*/
  Matrix3D &operator +=( const Matrix3D &m );
/*!
  Subtracts matrix \em m componentwise from this matrix.
*/
  Matrix3D &operator -=( const Matrix3D &m );
/*!
  Multiplies this matrix with matrix \em m (this=this*m).
*/
  Matrix3D &operator *=( const Matrix3D &m );
/*!
  Returns the componentwise sum of this matrix and matrix \em m.
*/
  Matrix3D  operator + ( const Matrix3D &m ) const;
/*!
  Returns the componentwise difference between this matrix and matrix \em m.
*/
  Matrix3D  operator - ( const Matrix3D &m ) const;
/*!
  Returns the matrix resulting from this matrix multiplied with matrix \em m.
*/
  Matrix3D  operator * ( const Matrix3D &m ) const;
/*!
  Returns this matrix multiplied with vector \em v.
*/
  Vector3D  operator * ( const Vector3D &v ) const;
/*!
  Returns this matrix multiplied with vector \em v.
*/
  Vector2D  operator * ( const Vector2D &v ) const;
/*!
  Returns this matrix with all components multiplied with -1.
  \code
    return( Matrix3D( -x, -y, -z ) );
  \endcode
*/
  Matrix3D  operator - () 			 const;
/*!
  Multiplies all components of this matrix with \em c.
*/
  Matrix3D &operator *=( double c      );
/*!
  Devides all components of this matrix with \em c.
*/
  Matrix3D &operator /=( double c      );
/*!
  Returns true if all components of matrix \em m equal the components of this matrix, otherwise returns false.
*/
  bool       operator  ==( const Matrix3D &m ) const;
/*!
  Returns true if one of the components of matrix \em m and this matrix are not equal, otherwise returns false.
*/
  bool       operator  !=( const Matrix3D &m ) const;
  
};

/***************************************************************************
*** 
*** Matrix3D  inlines
***
****************************************************************************/

inline 
Matrix3D::Matrix3D( const Matrix3D &other )
  : x( other.x ),
    y( other.y ),
    z( other.z )
{}


inline 
Matrix3D::Matrix3D( const Vector3D &vx, const Vector3D &vy, const Vector3D &vz  )
  : x(vx), y(vy), z(vz)
{}

inline 
Matrix3D::Matrix3D()
  : x(1.0,0.0,0.0), 
    y(0.0,1.0,0.0), 
    z(0.0,0.0,1.0)
{}

inline 
Matrix3D::Matrix3D( float xx, float xy, float xz,
			float yx, float yy, float yz,
			float zx, float zy, float zz ) 
  : x(xx, xy, xz),
    y(yx, yy, yz),
    z(zx, zy, zz) 
{}

inline   
Matrix3D::Matrix3D( const Matrix3H &m )
  : x(m.x),
    y(m.y),
    z(m.z)
{}


inline   
Matrix3D::Matrix3D( const Matrix4D &m )
  : x(m.x.x,m.x.y,m.x.z),
    y(m.y.x,m.y.y,m.y.z),
    z(m.z.x,m.z.y,m.z.z)
{}


inline 
Matrix3D::Matrix3D( const float m[3][3] )
{ memcpy( (void *)this, (void *)m, sizeof(float[3][3]) );
}

inline Matrix3D &
Matrix3D::operator = ( const Matrix3H &m )
{ x = m.x;
  y = m.y;
  z = m.z;
  return( *this );
}

inline Matrix3D &
Matrix3D::operator = ( const Matrix4D &m )
{ x = m.x;
  y = m.y;
  z = m.z;
  return( *this );
}

inline void
Matrix3D::id()
{ *this = Matrix3D(); 
}

inline bool
Matrix3D::isNull() const
{ return( x.isNull() && y.isNull() && z.isNull() );
}

inline bool
Matrix3D::isId() const
{ return( x.x == 1 && x.y == 0 && x.z == 0 &&
	  y.x == 0 && y.y == 1 && y.z == 0 &&
	  z.x == 0 && z.y == 0 && z.z == 1  
	  );
}

inline bool 
Matrix3D::operator ==( const Matrix3D &m ) const
{ return( m.x==x && m.y==y && m.z==z );
}

inline bool 
Matrix3D::operator !=( const Matrix3D &m ) const
{ return( m.x!=x || m.y!=y || m.z!=z );
}

inline Vector3D &
Matrix3D::operator [] ( int index ) const 
{ return( ((Vector3D*)this)[index] ); 
}

inline 
Matrix3D::operator const float*() 
{ return( (float*)this );
}

inline 
Matrix3D::operator float*() 
{ return( (float*)this );
}

inline Matrix3D &
Matrix3D::operator +=( const Matrix3D &m )
{ x += m.x;
  y += m.y;
  z += m.z;
  return( *this );
}

inline Matrix3D &
Matrix3D::operator -=( const Matrix3D &m )
{ x -= m.x;
  y -= m.y;
  z -= m.z;
  return( *this );
}

inline Matrix3D &
Matrix3D::operator *=( double c )
{ x *= c;
  y *= c;
  z *= c;
  return( *this );
}

inline Matrix3D &
Matrix3D::operator /=( double c )
{ x /= c;
  y /= c;
  z /= c;
  return( *this );
}

inline Matrix3D 
Matrix3D::operator +( const Matrix3D &m ) const
{ return( Matrix3D( x+m.x, y+m.y, z+m.z ) );
}

inline Matrix3D 
Matrix3D::operator -( const Matrix3D &m ) const
{ return( Matrix3D( x-m.x, y-m.y, z-m.z ) );
}

inline Matrix3D 
Matrix3D::operator -() const
{ return( Matrix3D( -x, -y, -z ) );
}

inline Matrix3D
Matrix3D::transpose() const
{ return(
    Matrix3D(
    x.x, y.x, z.x,
    x.y, y.y, z.y,
    x.z, y.z, z.z
    )
  );
}

inline 
Matrix4D::Matrix4D( const Matrix3D &m )
  : x( m.x.x, m.x.y, m.x.z, 0.0 ),
    y( m.y.x, m.y.y, m.y.z, 0.0 ),
    z( m.z.x, m.z.y, m.z.z, 0.0 ),
    w( 0.0, 0.0, 0.0, 1.0 ) 
{}


inline 
Matrix4D::Matrix4D( const Matrix3D &m, const Vector3D &v )
  : x( m.x.x, m.x.y, m.x.z, 0.0 ),
    y( m.y.x, m.y.y, m.y.z, 0.0 ),
    z( m.z.x, m.z.y, m.z.z, 0.0 ),
    w( v.x, v.y, v.z, 1.0 )
{}

inline 
Matrix3H::Matrix3H( const Matrix3D &m )
  : x( m.x.x, m.x.y, m.x.z ),
    y( m.y.x, m.y.y, m.y.z ),
    z( m.z.x, m.z.y, m.z.z ),
    w( 0.0, 0.0, 0.0 )
{}

inline 
Matrix3H::Matrix3H( const Matrix3D &m, const Vector3D &v )
  : x( m.x.x, m.x.y, m.x.z ),
    y( m.y.x, m.y.y, m.y.z ),
    z( m.z.x, m.z.y, m.z.z ),
    w( v.x, v.y, v.z )
{}


inline 
Matrix4D::operator Matrix3D ()
{ return( Matrix3D(
          x.x, x.y, x.z,
          y.x, y.y, y.z,
          z.x, z.y, z.z ) ); 
}

inline Matrix3H &
Matrix3H::operator = ( const Matrix3D &m )
{ x.x = m.x.x;
  x.y = m.x.y;
  x.z = m.x.z;
  y.x = m.y.x;
  y.y = m.y.y;
  y.z = m.y.z;
  z.x = m.z.x;
  z.y = m.z.y;
  z.z = m.z.z;
  return( *this );
}

inline Matrix4D &
Matrix4D::operator = ( const Matrix3D &m )
{ x.x = m.x.x;
  x.y = m.x.y;
  x.z = m.x.z;
  y.x = m.y.x;
  y.y = m.y.y;
  y.z = m.y.z;
  z.x = m.z.x;
  z.y = m.z.y;
  z.z = m.z.z;
  return( *this );
}

inline Vector3D &
Vector3D::operator *=( const Matrix3D &m )
{ return( *this = m * (*this) );
}

inline Matrix3D &
Matrix3D::operator *=( const Matrix3D &m )
{ return( *this = *this * m );
}


inline Matrix3D 
Matrix3D::lrotate( const Matrix3D &matrix, float x, float y, float z ) const
{ Matrix3D m( matrix );
  return( m.lrotate( x, y, z ) );
}


inline Matrix3D
Matrix3D::rrotate( const Matrix3D &matrix, float x, float y, float z ) const
{ Matrix3D m( matrix );
  return( m.rrotate( x, y, z ) );
}


inline Matrix3D
Matrix3D::rscale( const Matrix3D &matrix, float x, float y, float z ) const
{
  Matrix3D res = matrix;
  res.x *= x;
  res.y *= y;
  res.z *= z;
  return( res );
}


inline Matrix3D &
Matrix3D::rscale( float X, float Y, float Z )
{ x *= X;
  y *= Y;
  z *= Z;
  return( *this );
}

inline bool
Matrix3D::isIdentity() const
{ 
  return( x.x == 1.0 &&
	  y.y == 1.0 &&
	  z.z == 1.0 &&
	  x.y == 0.0 &&
	  x.z == 0.0 &&
	  y.x == 0.0 &&
	  y.z == 0.0 &&
	  z.x == 0.0 &&
	  z.y == 0.0 );
}


inline Vector2D &
Vector2D::operator *=( const Matrix3D &m )
{ return( *this = m * (*this) );
}


#endif



