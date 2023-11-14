// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include <stdio.h>
#include "Vector.h"

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/


/***************************************************************************
*** 
*** HELPERS
***
****************************************************************************/

static inline void
normalize( Vector2D &v )
{
   double len = sqrt( v.x*v.x + v.y*v.y );
  if ( len > 1e-6 && len != 1.0 ) 
    v /= len;
}


static inline void
normalize( Vector3D &v )
{
   double len = sqrt( v.x*v.x + v.y*v.y + v.z*v.z );
  if ( len > 1e-6 && len != 1.0 ) 
    v /= len;
}


/***************************************************************************
*** 
*** Vector2D
***
****************************************************************************/

/*!
  \class Vector2D Vector.h
  \ingroup Vector
  \brief class Vector2D provides operations on two dimensional floating point vectors.

  A Vector2D consists out of 2 components \em x and \em y

  Here is a brief class declaration:
  \code
  class Vector3D
  {
  public:
    float x;
    float y;
  };
  \endcode

  \sa Vector3D, Vector4D, Matrix3D, Matrix3H, Matrix4D

  Example:
  \code

  Vector2D v0( -1.0f, -1.0f );
  Vector2D v1(  1.0f,  1.0f );

  Vector2D v2( v0-v1 );

  float length = v2.length();

  float x = v2.x;
  float y = v2.y;

  v2.x = x+1;
  v2.y = y+1;

  v2.normalize();

  v2 = v0.product( v1 ); // product v0 X v1

  \endcode
*/

void
Vector2D::print( const char *text ) const
{ printf( "%s%g %g\n", text, x, y );
}


double 
Vector2D::angle( const Vector2D &other ) const
{
  Vector2D n1 = *this;
  Vector2D n2 = other;

  ::normalize( n1 );
  ::normalize( n2 );

  double dot = n1 * n2;
  if ( dot <= -1 )
    return M_PI;
  else if ( dot >= 1 )
    return M_PI;

  return( acos( dot ) );
}

double 
Vector2D::angle() const
{
  Vector2D n = *this;

  ::normalize( n );

  double angle;

  if ( n.y <= -1.0 )
    return M_PI;
  else if ( n.y >= 1.0 )
    return 0.0;
  else
    angle = acos( n.y );

  if ( n.x < 0 )
    return( -angle );

  return( angle );
}


/***************************************************************************
*** 
*** Vector3D
***
****************************************************************************/


/*!
  \class Vector3D Vector.h
  \ingroup Vector
  \brief class Vector3D provides operations on three dimensional floating point vectors.

  Here is a brief class declaration:
  \code
  class Vector3D
  {
  public:
    float x;
    float y;
    float z;
  };
  \endcode

  \sa Vector2D, Vector4D, Matrix3D, Matrix3H, Matrix4D
*/

void
Vector3D::print( const char *text ) const
{ printf( "%s%g %g %g\n", text, x, y, z );
}


double 
Vector3D::angle( const Vector3D &other ) const
{
  Vector3D n1 = *this;
  Vector3D n2 = other;
  double angle;

  ::normalize( n1 );
  ::normalize( n2 );

  angle = acos( n1 * n2 );
  return( angle );
}

Vector3D 
Vector3D::min( const Vector3D &v0, const Vector3D &v1 )
{
  return ( Vector3D( 
    v0.x < v1.x ? v0.x : v1.x,
    v0.y < v1.y ? v0.y : v1.y,
    v0.z < v1.z ? v0.z : v1.z
    ) );
}

Vector3D 
Vector3D::min( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2 )
{
  Vector3D min( v0.x < v1.x ? v0.x : v1.x,
		  v0.y < v1.y ? v0.y : v1.y,
		  v0.z < v1.z ? v0.z : v1.z );
  
  if ( v2.x < min.x ) min.x = v2.x;
  if ( v2.y < min.y ) min.y = v2.y;
  if ( v2.z < min.z ) min.z = v2.z;
  
  return ( min );
}

Vector3D 
Vector3D::min( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2, const Vector3D &v3 )
{
  Vector3D min( v0.x < v1.x ? v0.x : v1.x,
		  v0.y < v1.y ? v0.y : v1.y,
		  v0.z < v1.z ? v0.z : v1.z );
  
  if ( v2.x < min.x ) min.x = v2.x;
  if ( v2.y < min.y ) min.y = v2.y;
  if ( v2.z < min.z ) min.z = v2.z;
  if ( v3.x < min.x ) min.x = v3.x;
  if ( v3.y < min.y ) min.y = v3.y;
  if ( v3.z < min.z ) min.z = v3.z;
  
  return ( min );
}

Vector3D 
Vector3D::max( const Vector3D &v0, const Vector3D &v1 )
{
  return ( Vector3D( 
    v0.x > v1.x ? v0.x : v1.x,
    v0.y > v1.y ? v0.y : v1.y,
    v0.z > v1.z ? v0.z : v1.z
    ) );
}

Vector3D 
Vector3D::max( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2 )
{
  Vector3D max( v0.x > v1.x ? v0.x : v1.x,
		  v0.y > v1.y ? v0.y : v1.y,
		  v0.z > v1.z ? v0.z : v1.z );
  
  if ( v2.x > max.x ) max.x = v2.x;
  if ( v2.y > max.y ) max.y = v2.y;
  if ( v2.z > max.z ) max.z = v2.z;
  
  return ( max );
}

Vector3D 
Vector3D::max( const Vector3D &v0, const Vector3D &v1, const Vector3D &v2, const Vector3D &v3 )
{
  Vector3D max( v0.x > v1.x ? v0.x : v1.x,
		  v0.y > v1.y ? v0.y : v1.y,
		  v0.z > v1.z ? v0.z : v1.z );
  
  if ( v2.x > max.x ) max.x = v2.x;
  if ( v2.y > max.y ) max.y = v2.y;
  if ( v2.z > max.z ) max.z = v2.z;
  if ( v3.x > max.x ) max.x = v3.x;
  if ( v3.y > max.y ) max.y = v3.y;
  if ( v3.z > max.z ) max.z = v3.z;
  
  return ( max );
}


/***************************************************************************
*** 
*** Vector4D
***
****************************************************************************/

/*!
  \class Vector4D Vector.h
  \ingroup Vector
  \brief class Vector4D provides operations on four dimensional floating point vectors.

  Here is a brief class declaration:
  \code
  class Vector4D
  {
  public:
    float x;
    float y;
    float z;
    float w;
  };
  \endcode

  \sa Vector2D, Vector3D, Matrix3D, Matrix3H, Matrix4D
*/

void
Vector4D::print( const char *text ) const
{ printf( "%s%g %g %g %g\n", text, x, y, z, w );
}
 
/***************************************************************************
*** 
*** Matrix4D <-> Vector4D
***
****************************************************************************/

Vector4D
Matrix4D::operator *( const Vector4D &v ) const
{
  return( Vector4D( 

    x.x*v.x + y.x*v.y + z.x*v.z + w.x*v.w,
    x.y*v.x + y.y*v.y + z.y*v.z + w.y*v.w,
    x.z*v.x + y.z*v.y + z.z*v.z + w.z*v.w,
    x.w*v.x + y.w*v.y + z.w*v.z + w.w*v.w

    ) );
}

/***************************************************************************
*** 
*** Matrix4D <-> Vector3D
***
****************************************************************************/

Vector3D
Matrix4D::operator *( const Vector3D &v ) const
{
  Vector3D tmp( 

    x.x*v.x + y.x*v.y + z.x*v.z + w.x,
    x.y*v.x + y.y*v.y + z.y*v.z + w.y,
    x.z*v.x + y.z*v.y + z.z*v.z + w.z
 
    );

  double h = x.w*v.x + y.w*v.y + z.w*v.z + w.w;

  if ( h == 1.0 )
    return( tmp );
  else if ( h != 0.0 )
    return( tmp /= h );

  return( tmp );
}


/***************************************************************************
*** 
*** Matrix4D <-> Vector2D
***
****************************************************************************/

Vector2D 
Matrix4D::mult2D( const Vector3D &v ) const
{
  Vector2D tmp( 

    x.x*v.x + y.x*v.y + z.x*v.z + w.x,
    x.y*v.x + y.y*v.y + z.y*v.z + w.y
 
    );

  double h = x.w*v.x + y.w*v.y + z.w*v.z + w.w;

  if ( h == 1.0 )
    return( tmp );
  else if ( h != 0.0 )
    return( tmp/h );

  return( tmp );
}

Vector2D 
Matrix4D::operator *( const Vector2D &v ) const
{
  Vector2D tmp( 

    x.x*v.x + y.x*v.y + z.x*v.z + w.x,
    x.y*v.x + y.y*v.y + z.y*v.z + w.y
 
    );

  double h = x.w*v.x + y.w*v.y + z.w*v.z + w.w;

  if ( h == 1.0 )
    return( tmp );
  else if ( h != 0.0 )
    return( tmp/h );

  return( tmp );
}

double
Matrix4D::multZ( const Vector3D &v ) const
{
  double tmp = x.z*v.x + y.z*v.y + z.z*v.z + w.z;
  double h   = x.w*v.x + y.w*v.y + z.w*v.z + w.w;

  if ( h == 1.0 )
    return( tmp );
  else if ( h != 0.0 )
    return( tmp/h );

  return( tmp );
}

/***************************************************************************
*** 
*** Matrix4D
***
****************************************************************************/


/*!
  \class Matrix4D Vector.h
  \ingroup Vector
  \brief class Matrix4D provides operations on 4x4 floating point matrices.

  A Matrix4D consist out of 4 Vector4D, the components x, y, z, w.
  The vectors are stored in row wise order. By that a Matrix4D can be converted by type cast to a float array which is suitable for OpenGL matrix operations.

  Here is a brief class declaration:
  \code
  class Matrix3H
  {
  public:
    Vector3D x;
    Vector3D y;
    Vector3D z;
    Vector3D w;
  };
  \endcode

  \sa Vector2D, Vector3D, Vector4D, Matrix3D, Matrix3H

  Examples:
  \code
    Matrix4D matrix; // constructs an identity matrix
    matrix.x.w = 1.0f; // makes a translation matrix for translation one unit along the x-axis

    Matrix4D matrixTranslational( 1.0f, 0.0f, 0.0f ); // constructs a translation matrix for translation one unit along the x-axis
    matrix.x.w = 1.0f; // makes a translation matrix for translation one unit along the x-axis

    Matrix4D inverseMatrix( matrix.inverse() ); // constructs an matrix as inverse of matrix

    Vector3D v( 1.0f, 2.0f, 3.0f ); // constructs a 3d vector

    Vector3D result( matrix * v ); // constructs a 3d vector as result of a matrix vector multiplication


  \endcode
*/

Matrix4D::Matrix4D( const float m[4][4] )
{ ::memcpy( (void *)this, (void *)m, sizeof(float[4][4]) );
}

void
Matrix4D::print( const char *text ) const
{ printf( "%s", text );
  x.print( " " );
  y.print( " " );
  z.print( " " );
  w.print( " " );
}

Matrix4D 
Matrix4D::operator * ( const Matrix3H &m ) const
{ 
#define mult_column(Y) \
    x.x * m.Y.x +\
    y.x * m.Y.y +\
    z.x * m.Y.z,\
\
    x.y * m.Y.x +\
    y.y * m.Y.y +\
    z.y * m.Y.z,\
\
    x.z * m.Y.x +\
    y.z * m.Y.y +\
    z.z * m.Y.z,\
\
    x.w * m.Y.x +\
    y.w * m.Y.y +\
    z.w * m.Y.z

#define mult_columnW \
    x.x * m.w.x +\
    y.x * m.w.y +\
    z.x * m.w.z +\
    w.x,\
\
    x.y * m.w.x +\
    y.y * m.w.y +\
    z.y * m.w.z +\
    w.y,\
\
    x.z * m.w.x +\
    y.z * m.w.y +\
    z.z * m.w.z +\
    w.z,\
\
    x.w * m.w.x +\
    y.w * m.w.y +\
    z.w * m.w.z +\
    w.w

  return( Matrix4D(
    mult_column(x),
    mult_column(y),
    mult_column(z),
    mult_columnW
    )
  );

#undef mult_column
#undef mult_columnW
}


Matrix4D 
Matrix4D::operator * ( const Matrix4D &m ) const
{ 
#define mult_column(Y) \
    x.x * m.Y.x +\
    y.x * m.Y.y +\
    z.x * m.Y.z +\
    w.x * m.Y.w,\
\
    x.y * m.Y.x +\
    y.y * m.Y.y +\
    z.y * m.Y.z +\
    w.y * m.Y.w,\
\
    x.z * m.Y.x +\
    y.z * m.Y.y +\
    z.z * m.Y.z +\
    w.z * m.Y.w,\
\
    x.w * m.Y.x +\
    y.w * m.Y.y +\
    z.w * m.Y.z +\
    w.w * m.Y.w

  return( Matrix4D(
    mult_column(x),
    mult_column(y),
    mult_column(z),
    mult_column(w)
    )
  );
#undef mult_column
}


#define DET3D(x0,x1,x2,y0,y1,y2)    ((*this)[x0][y0] * ((*this)[x1][y1]*(*this)[x2][y2] - (*this)[x2][y1]*(*this)[x1][y2] ) - \
				     (*this)[x0][y1] * ((*this)[x1][y0]*(*this)[x2][y2] - (*this)[x2][y0]*(*this)[x1][y2] ) + \
				     (*this)[x0][y2] * ((*this)[x1][y0]*(*this)[x2][y1] - (*this)[x2][y0]*(*this)[x1][y1] ))

#define ELEM4D(i,j,x0,x1,x2,y0,y1,y2) (DET3D(x0,x1,x2,y0,y1,y2) / Det * ( (i+j)%2 ? 1 : -1 ))

Matrix4D
Matrix4D::inverse() const
{
  double Det;

  Det = - x.x * (y.y*z.z - z.y*y.z ) +
          x.y * (y.x*z.z - z.x*y.z ) -
          x.z * (y.x*z.y - z.x*y.y );
 

  return( Matrix4D(

    ELEM4D (0,0, 1,2,3, 1,2,3),
    ELEM4D (1,0, 0,2,3, 1,2,3),
    ELEM4D (2,0, 0,1,3, 1,2,3),
    ELEM4D (3,0, 0,1,2, 1,2,3),

    ELEM4D (0,1, 1,2,3, 0,2,3),
    ELEM4D (1,1, 0,2,3, 0,2,3),
    ELEM4D (2,1, 0,1,3, 0,2,3),
    ELEM4D (3,1, 0,1,2, 0,2,3),


    ELEM4D (0,2, 1,2,3, 0,1,3),
    ELEM4D (1,2, 0,2,3, 0,1,3),
    ELEM4D (2,2, 0,1,3, 0,1,3),
    ELEM4D (3,2, 0,1,2, 0,1,3),

    ELEM4D (0,3, 1,2,3, 0,1,2),
    ELEM4D (1,3, 0,2,3, 0,1,2),
    ELEM4D (2,3, 0,1,3, 0,1,2),
    ELEM4D (3,3, 0,1,2, 0,1,2) 
    )
  );
}

Vector4D
Matrix4D::inverseZ() const
{
  double Det;

  Det = - x.x * (y.y*z.z - z.y*y.z ) +
          x.y * (y.x*z.z - z.x*y.z ) -
          x.z * (y.x*z.y - z.x*y.y );
 

  return( Vector4D(
    ELEM4D (0,2, 1,2,3, 0,1,3),
    ELEM4D (1,2, 0,2,3, 0,1,3),
    ELEM4D (2,2, 0,1,3, 0,1,3),
    ELEM4D (3,2, 0,1,2, 0,1,3)
    )
  );
}

Matrix4D &
Matrix4D::rrotate( float xf, float yf, float zf )
{
#define mult_column(Y,Z) \
    y.x * Y +\
    z.x * Z, \
\
    y.y * Y +\
    z.y * Z,\
\
    y.z * Y +\
    z.z * Z

  if ( xf != 0.0 )
  { float cx = cos(xf);
    float sx = sin(xf);

    Vector3D resY( mult_column( cx, sx) );
    Vector3D resZ( mult_column(-sx, cx) );

    ((Vector3D&)y) = resY;
    ((Vector3D&)z) = resZ;
  }
#undef mult_column

#define mult_column(X,Z) \
    x.x * X +\
    z.x * Z, \
\
    x.y * X +\
    z.y * Z,\
\
    x.z * X +\
    z.z * Z

  if ( yf != 0.0 )
  { float cy = cos(yf);
    float sy = sin(yf);

    Vector3D resX( mult_column( cy, sy) );
    Vector3D resZ( mult_column(-sy, cy) );

    ((Vector3D&)x) = resX;
    ((Vector3D&)z) = resZ;
  }

#undef mult_column

#define mult_column(X,Y) \
    x.x * X +\
    y.x * Y, \
\
    x.y * X +\
    y.y * Y,\
\
    x.z * X +\
    y.z * Y

  if ( zf != 0.0 )
  { float cz = cos(zf);
    float sz = sin(zf);

    Vector3D resX( mult_column( cz, sz) );
    Vector3D resY( mult_column(-sz, cz) );

    x = resX;
    y = resY;
  }
  
  return( *this );
#undef mult_column
}

Matrix4D &
Matrix4D::rrotateX( float xf )
{
#define mult_column(Y,Z) \
    y.x * Y +\
    z.x * Z, \
\
    y.y * Y +\
    z.y * Z,\
\
    y.z * Y +\
    z.z * Z,\
    0.0

  if ( xf != 0.0 )
  { float cx = cos(xf);
    float sx = sin(xf);

    Vector4D resY( mult_column(  cx, sx) );
    Vector4D resZ( mult_column( -sx, cx) );

    ((Vector3D&)y) = resY;
    ((Vector3D&)z) = resZ;
  }
  
  return( *this );
#undef mult_column
}

Matrix4D &
Matrix4D::rrotateY( float yf )
{
#define mult_column(X,Z) \
    x.x * X +\
    z.x * Z, \
\
    x.y * X +\
    z.y * Z,\
\
    x.z * X +\
    z.z * Z

  if ( yf != 0.0 )
  { float cy = cos(yf);
    float sy = sin(yf);

    Vector3D resX( mult_column( cy, sy) );
    Vector3D resZ( mult_column(-sy, cy) );

    ((Vector3D&)x) = resX;
    ((Vector3D&)z) = resZ;
  }

  return( *this );
#undef mult_column
}

Matrix4D &
Matrix4D::rrotateZ( float zf )
{
#define mult_column(X,Y) \
    x.x * X +\
    y.x * Y, \
\
    x.y * X +\
    y.y * Y,\
\
    x.z * X +\
    y.z * Y

  if ( zf != 0.0 )
  { float cz = cos(zf);
    float sz = sin(zf);

    Vector3D resX( mult_column( cz, sz) );
    Vector3D resY( mult_column(-sz, cz) );

    ((Vector3D&)x) = resX;
    ((Vector3D&)y) = resY;
  }
  
  return( *this );
#undef mult_column
}

Matrix4D &
Matrix4D::lrotate( float ax, float ay, float az )
{
  if ( x != 0.0 )
  {
    float cx  =  cos(ax);
    float sx  =  sin(ax);
    float msx = -sx;

    Matrix4D rt( 1.0, 0.0, 0.0, 0.0,
		   0.0,  cx,  sx, 0.0,
		   0.0, -sx,  cx, 0.0,
		   0.0, 0.0, 0.0, 1.0 );
    *this = rt * *this;

	/* new
    Vector4D vy(  cx * y.x + sx * z.x,
		    cx * y.y + sx * z.y,
		    cx * y.z + sx * z.z,
		    cx * y.w + sx * z.w );
    
    Vector4D vz( msx * y.x + cx * z.x,
		   msx * y.y + cx * z.y,
		   msx * y.z + cx * z.z,
		   msx * y.w + cx * z.w );

    y = vy;
    z = vz;
	*/
  }

  if ( y != 0.0 )
  { float cy  =  cos(ay);
    float sy  =  sin(ay);
    float msy = -sy;

    Matrix4D rt(  cy, 0.0,  sy, 0.0,
		   0.0, 1.0, 0.0, 0.0,
		   -sy, 0.0,  cy, 0.0,
		   0.0, 0.0, 0.0, 1.0 );
    *this = rt * *this;

	/* new
    Vector4D vx(  cy * x.x + sy * z.x,
		    cy * x.y + sy * z.y,
		    cy * x.z + sy * z.z,
		    cy * x.w + sy * z.w );
    
    Vector4D vz( msy * x.x + cy * z.x,
		   msy * x.y + cy * z.y,
		   msy * x.z + cy * z.z,
		   msy * x.w + cy * z.w );

    x = vx;
    z = vz;
	*/
  }

  if ( z != 0.0 )
  { float cz  =  cos(az);
    float sz  =  sin(az);
    float msz = -sz;

    Matrix4D rt(  cz,  sz, 0.0, 0.0,
		   -sz,  cz, 0.0, 0.0,
		   0.0, 0.0, 1.0, 0.0,
		   0.0, 0.0, 0.0, 1.0 );
    *this = rt * *this;

	/* new
    Vector4D vx(  cz * x.x + sz * y.x,
		    cz * x.y + sz * y.y,
		    cz * x.z + sz * y.z,
		    cz * x.w + sz * y.w );
    
    Vector4D vy( msz * x.x + cz * y.x,
		   msz * x.y + cz * y.y,
		   msz * x.z + cz * y.z,
		   msz * x.w + cz * y.w );

    x = vx;
    y = vy;
	*/
  }
  
  return( *this );
}


Matrix4D &
Matrix4D::lrotateX( float x )
{
  if ( x != 0.0 )
  {
    float cx  =  cos(x);
    float sx  =  sin(x);
    float msx = -sx;

    Matrix4D rt( 1.0, 0.0, 0.0, 0.0,
		   0.0,  cx,  sx, 0.0,
		   0.0, -sx,  cx, 0.0,
		   0.0, 0.0, 0.0, 1.0 );
    *this = rt * *this;

	/* new
    Vector4D vy( cx * y.x + sx * z.x,
		   cx * y.y + sx * z.y,
		   cx * y.z + sx * z.z,
		   cx * y.w + sx * z.w );
    
    Vector4D vz( msx * y.x + cx * z.x,
		   msx * y.y + cx * z.y,
		   msx * y.z + cx * z.z,
		   msx * y.w + cx * z.w );

    y = vy;
    z = vz;
	*/
  }
  
  return( *this );
}

Matrix4D &
Matrix4D::lrotateY( float y )
{
  if ( y != 0.0 )
  { float cy  =  cos(y);
    float sy  =  sin(y);
    float msy = -sy;

    Matrix4D rt(  cy, 0.0,  sy, 0.0,
		   0.0, 1.0, 0.0, 0.0,
		   -sy, 0.0,  cy, 0.0,
		   0.0, 0.0, 0.0, 1.0 );
    *this = rt * *this;
	/* new
    Vector4D vx( cy * x.x + sy * z.x,
		   cy * x.y + sy * z.y,
		   cy * x.z + sy * z.z,
		   cy * x.w + sy * z.w );
    
    Vector4D vz( msy * x.x + cy * z.x,
		   msy * x.y + cy * z.y,
		   msy * x.z + cy * z.z,
		   msy * x.w + cy * z.w );

    x = vx;
    z = vz;
	*/
  }
  
  return( *this );
}


Matrix4D &
Matrix4D::lrotateZ( float z )
{
  if ( z != 0.0 )
  { float cz  =  cos(z);
    float sz  =  sin(z);
    float msz = -sz;

    Matrix4D rt(  cz,  sz, 0.0, 0.0,
		   -sz,  cz, 0.0, 0.0,
		   0.0, 0.0, 1.0, 0.0,
		   0.0, 0.0, 0.0, 1.0 );
    *this = rt * *this;
/*
    Vector4D vx(  cz * x.x + sz * y.x,
		    cz * x.y + sz * y.y,
		    cz * x.z + sz * y.z,
		    cz * x.w + sz * y.w );
    
    Vector4D vy( msz * x.x + cz * y.x,
		   msz * x.y + cz * y.y,
		   msz * x.z + cz * y.z,
		   msz * x.w + cz * y.w );

    x = vx;
    y = vy;
*/
  }
  
  return( *this );
}


Matrix4D &
Matrix4D::ltranslate( float X, float Y, float Z )
{
  x.x += X * x.w;
  x.y += Y * x.w;
  x.z += Z * x.w;
  
  y.x += X * y.w;
  y.y += Y * y.w;
  y.z += Z * y.w;
  
  z.x += X * z.w;
  z.y += Y * z.w;
  z.z += Z * z.w;
  
  w.x += X * w.w;
  w.y += Y * w.w;
  w.z += Z * w.w;
  
  return( *this );
}

Matrix4D &
Matrix4D::rtranslate( float X, float Y, float Z )
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
  
  w.w += x.w * X +
         y.w * Y +
         z.w * Z;
  
  return( *this );
}


Matrix4D
Matrix4D::lscale( const Matrix4D &matrix, float x, float y, float z ) const
{
  Matrix4D res = matrix;
  res.x.x *= x;
  res.x.y *= y;
  res.x.z *= z;
  res.y.x *= x;
  res.y.y *= y;
  res.y.z *= z;
  res.z.x *= x;
  res.z.y *= y;
  res.z.z *= z;
  res.w.x *= x;
  res.w.y *= y;
  res.w.z *= z;
  return( res );
}

Matrix4D &
Matrix4D::lscale( float X, float Y, float Z )
{
  x.x *= X;
  x.y *= Y;
  x.z *= Z;
  y.x *= X;
  y.y *= Y;
  y.z *= Z;
  z.x *= X;
  z.y *= Y;
  z.z *= Z;
  w.x *= X;
  w.y *= Y;
  w.z *= Z;
  return( *this );
}


Vector3D
Matrix4D::rotateVector3D( const Vector3D &v ) const
{ 
  return( Vector3D( 
    x.x*v.x + y.x*v.y + z.x*v.z,
    x.y*v.x + y.y*v.y + z.y*v.z,
    x.z*v.x + y.z*v.y + z.z*v.z
    )
  );
}

void 
Matrix4D::rotationAngles( float &rotx, float &roty, float &rotz ) const
{
  Matrix4D      tmpx, tmpy, tmpz;
  Vector3D      local_x = x;
  Vector3D      local_z = z;
  double          length;
  double          angle_x, angle_y, angle_z = 0.0;

  length = sqrt( local_x.x*local_x.x + local_x.y*local_x.y );

  if ( length != 0.0 )
  { tmpz.x.x =  local_x.x / length;
    tmpz.y.x =  local_x.y / length;
    tmpz.x.y = -local_x.y / length;
    tmpz.y.y =  tmpz.x.x;

    angle_z = asin( -tmpz.y.x );
    
    if ( tmpz.x.x <= 0.0 )
      angle_z = ( angle_z >= 0.0 ) ? M_PI-angle_z : -M_PI-angle_z;
	
    local_x = tmpz * local_x;
    local_z = tmpz * local_z;
  }

  length = local_x.length();

  tmpy.x.x =  local_x.x / length;
  tmpy.z.x =  local_x.z / length;
  tmpy.x.z = -local_x.z / length;
  tmpy.z.z =  tmpy.x.x;

  angle_y = asin( -tmpy.z.x );
    
  if ( tmpy.x.x <= 0.0 )
    angle_y = ( angle_y >= 0.0 ) ? M_PI-angle_y : -M_PI-angle_y;
	
  local_z = tmpy * local_z;

  length = sqrt( local_z.y*local_z.y + local_z.z*local_z.z );

  tmpx.y.y =  local_z.z / length;
  tmpx.z.y =  local_z.y / length;

  angle_x = asin( tmpx.z.y );
    
  if ( tmpx.y.y <= 0.0 )
    angle_x = ( angle_x >= 0.0 ) ? M_PI-angle_x : -M_PI-angle_x;

  rotx = -angle_x;
  roty = -angle_y;
  rotz = -angle_z;
}

Matrix4D &
Matrix4D::pointAt( float ax, float ay, float az )
{ 
  Vector3D Z( ax-w.x, ay-w.y, az-w.z );
  double d = Z.length();
  
  if ( d < 1e-9 )
    return( *this );
  
  z.x = Z.x/d;
  z.y = Z.y/d;
  z.z = Z.z/d;

  Vector3D Y = ((Vector3D&)z).product( (Vector3D&)x );
  d = Y.length();
    
  if ( d < 1e-9 )
  { Y = ((Vector3D&)z).product( (Vector3D&)y );
    d = Y.length();
  }
  
  y.x = Y.x/d;
  y.y = Y.y/d;
  y.z = Y.z/d;

  x = ((Vector3D&)y).product( (Vector3D&)z );

  return( *this );
}

/***************************************************************************
*** 
*** Matrix3H <-> Vector3D
***
****************************************************************************/

Vector3D
Matrix3H::operator *( const Vector3D &v ) const
{ 
  return( Vector3D( 
    x.x*v.x + y.x*v.y + z.x*v.z + w.x,
    x.y*v.x + y.y*v.y + z.y*v.z + w.y,
    x.z*v.x + y.z*v.y + z.z*v.z + w.z
    )
  );
}

Vector2D
Matrix3H::operator *( const Vector2D &v ) const
{ 
  return( Vector2D( 
    x.x*v.x + y.x*v.y + w.x,
    x.y*v.x + y.y*v.y + w.y
    )
  );
}

Vector3D
Matrix3H::rotateVector3D( const Vector3D &v ) const
{ 
  return( Vector3D( 
    x.x*v.x + y.x*v.y + z.x*v.z,
    x.y*v.x + y.y*v.y + z.y*v.z,
    x.z*v.x + y.z*v.y + z.z*v.z
    )
  );
}

/***************************************************************************
*** 
*** Matrix3H
***
****************************************************************************/

/*!
  \class Matrix3H Vector.h
  \ingroup Vector
  \brief class Matrix3H provides operations on 4x3 floating point matrices.


  A Matrix3H consist out of 4 Vector3D, the components x, y, z, w.
  The vectors are stored in row wise order.
  
  Here is a brief class declaration:
  \code
  class Matrix3H
  {
  public:
    Vector3D x;
    Vector3D y;
    Vector3D z;
    Vector3D w;
  };
  \endcode

  Examples:
  \code
    Matrix3H matrix; // constructs an identity matrix
    matrix.x.w = 1.0f; // makes a translation matrix for translation one unit along the x-axis

    Matrix3H matrixTranslational( 1.0f, 0.0f, 0.0f ); // constructs a translation matrix for translation one unit along the x-axis
    matrix.x.w = 1.0f; // makes a translation matrix for translation one unit along the x-axis

    Matrix3H inverseMatrix( matrix.inverse() ); // constructs an matrix as inverse of matrix

    Vector3D v( 1.0f, 2.0f, 3.0f ); // constructs a 3d vector

    Vector3D result( matrix * v ); // constructs a 3d vector as result of a matrix vector multiplication


  \endcode

  \sa Vector2D, Vector3D, Vector4D, Matrix3D, Matrix4D
*/

void
Matrix3H::print( const char *text ) const
{ printf( "%s", text );
  x.print( " " );
  y.print( " " );
  z.print( " " );
  w.print( " " );
}

Matrix3H 
Matrix3H::operator * ( const Matrix3H &m ) const
{ 
#define mult_column(Y) \
    x.x * m.Y.x +\
    y.x * m.Y.y +\
    z.x * m.Y.z,\
\
    x.y * m.Y.x +\
    y.y * m.Y.y +\
    z.y * m.Y.z,\
\
    x.z * m.Y.x +\
    y.z * m.Y.y +\
    z.z * m.Y.z

#define mult_columnW \
    x.x * m.w.x +\
    y.x * m.w.y +\
    z.x * m.w.z +\
    w.x,\
\
    x.y * m.w.x +\
    y.y * m.w.y +\
    z.y * m.w.z +\
    w.y,\
\
    x.z * m.w.x +\
    y.z * m.w.y +\
    z.z * m.w.z +\
    w.z

  if ( m.w.x == 0 && m.w.y == 0 && m.w.z == 0 )
    return( Matrix3H(
      mult_column(x),
      mult_column(y),
      mult_column(z),
      w.x, w.y, w.z
      )
    );

  return( Matrix3H(
    mult_column(x),
    mult_column(y),
    mult_column(z),
    mult_columnW
    )
  );
#undef mult_column
#undef mult_columnW
}




#define DET2D(x0,x1,y0,y1)      ((*this)[x0][y0] * (*this)[x1][y1] - (*this)[x0][y1] * (*this)[x1][y0])
#define ELEM3D(i,j,x0,x1,y0,y1) (DET2D(x0,x1,y0,y1) / Det * ( (i+j)%2 ? 1 : -1 ))

Matrix3H
Matrix3H::inverse() const
{
  double Det;

  Det = - x.x * (y.y*z.z - z.y*y.z ) +
          x.y * (y.x*z.z - z.x*y.z ) -
          x.z * (y.x*z.y - z.x*y.y );
 
  Vector3D vX(
    ELEM3D (0,0, 1,2, 1,2),
    ELEM3D (1,0, 0,2, 1,2),
    ELEM3D (2,0, 0,1, 1,2)
    );
  
  Vector3D vY(
    ELEM3D (0,1, 1,2, 0,2),
    ELEM3D (1,1, 0,2, 0,2),
    ELEM3D (2,1, 0,1, 0,2)
    );
  
  Vector3D vZ(
    ELEM3D (0,2, 1,2, 0,1),
    ELEM3D (1,2, 0,2, 0,1),
    ELEM3D (2,2, 0,1, 0,1)
    );

  Vector3D vW(
    -(vX.x * w.x + vY.x * w.y + vZ.x * w.z),
    -(vX.y * w.x + vY.y * w.y + vZ.y * w.z),
    -(vX.z * w.x + vY.z * w.y + vZ.z * w.z)
   );

  return( Matrix3H( vX, vY, vZ, vW ) );
}

Vector3D
Matrix3H::inverseZ() const
{
  double Det;

  Det = - x.x * (y.y*z.z - z.y*y.z ) +
          x.y * (y.x*z.z - z.x*y.z ) -
          x.z * (y.x*z.y - z.x*y.y );
 
  return( Vector3D(
    ELEM3D (0,2, 1,2, 0,1),
    ELEM3D (1,2, 0,2, 0,1),
    ELEM3D (2,2, 0,1, 0,1) )
    );
}

Matrix3H &
Matrix3H::lrotate( float x, float y, float z )
{ Matrix4D m(*this);
  *this = m.lrotate( x, y, z );
  return( *this );
}

Matrix3H &
Matrix3H::lrotateX( float x )
{ Matrix4D m(*this);
  *this = m.lrotateX( x );
  return( *this );
}

Matrix3H &
Matrix3H::lrotateY( float y )
{ Matrix4D m(*this);
  *this = m.lrotateY( y );
  return( *this );
}

Matrix3H &
Matrix3H::lrotateZ( float z )
{ Matrix4D m(*this);
  *this = m.lrotateZ( z );
  return( *this );
}

Matrix3H &
Matrix3H::rrotate( float xf, float yf, float zf )
{
#define mult_column(Y,Z) \
    y.x * Y +\
    z.x * Z, \
\
    y.y * Y +\
    z.y * Z,\
\
    y.z * Y +\
    z.z * Z

  if ( xf != 0.0 )
  { float cx = cos(xf);
    float sx = sin(xf);

    Vector3D resY( mult_column( cx, sx) );
    Vector3D resZ( mult_column(-sx, cx) );

    y = resY;
    z = resZ;
  }
#undef mult_column

#define mult_column(X,Z) \
    x.x * X +\
    z.x * Z, \
\
    x.y * X +\
    z.y * Z,\
\
    x.z * X +\
    z.z * Z

  if ( yf != 0.0 )
  { float cy = cos(yf);
    float sy = sin(yf);

    Vector3D resX( mult_column( cy, sy) );
    Vector3D resZ( mult_column(-sy, cy) );

    x = resX;
    z = resZ;
  }

#undef mult_column

#define mult_column(X,Y) \
    x.x * X +\
    y.x * Y, \
\
    x.y * X +\
    y.y * Y,\
\
    x.z * X +\
    y.z * Y

  if ( zf != 0.0 )
  { float cz = cos(zf);
    float sz = sin(zf);

    Vector3D resX( mult_column( cz, sz) );
    Vector3D resY( mult_column(-sz, cz) );

    x = resX;
    y = resY;
  }
  
  return( *this );
#undef mult_column
}

Matrix3H &
Matrix3H::rrotateX( float X )
{ ((Matrix3D*)this)->rrotateX( X );
  return( *this );
}

Matrix3H &
Matrix3H::rrotateY( float Y )
{ ((Matrix3D*)this)->rrotateY( Y );
  return( *this );
}

Matrix3H &
Matrix3H::rrotateZ( float Z )
{ ((Matrix3D*)this)->rrotateZ( Z );
  return( *this );
}

Matrix3H
Matrix3H::lscale( const Matrix3H &matrix, float x, float y, float z ) const
{
  Matrix3H res = matrix;
  res.x.x *= x;
  res.x.y *= y;
  res.x.z *= z;
  res.y.x *= x;
  res.y.y *= y;
  res.y.z *= z;
  res.z.x *= x;
  res.z.y *= y;
  res.z.z *= z;
  res.w.x *= x;
  res.w.y *= y;
  res.w.z *= z;
  return( res );
}


Matrix3H &
Matrix3H::lscale( float X, float Y, float Z )
{
  x.x *= X;
  x.y *= Y;
  x.z *= Z;
  y.x *= X;
  y.y *= Y;
  y.z *= Z;
  z.x *= X;
  z.y *= Y;
  z.z *= Z;
  w.x *= X;
  w.y *= Y;
  w.z *= Z;
  return( *this );
}


void
Matrix3H::rotationAngles( float &rotx, float &roty, float &rotz ) const
{ ((Matrix3D*)this)->rotationAngles( rotx, roty, rotz );
}

Matrix3H &
Matrix3H::pointAt( float ax, float ay, float az )
{ 
  Vector3D Z( ax-w.x, ay-w.y, az-w.z );
  double d = Z.length();
  
  if ( d < 1e-9 )
    return( *this );
  
  z = Z/d;

  Vector3D Y = z.product( x );
  d = Y.length();
    
  if ( d < 1e-9 )
  { Y = z.product( y );
    d = Y.length();
  }
  
  y = Y/d;
  x = y.product( z );

  return( *this );
}

/***************************************************************************
*** 
*** Matrix3D <-> Vector3D
***
****************************************************************************/

Vector3D
Matrix3D::operator *( const Vector3D &v ) const
{ 
  return( Vector3D( 
    x.x*v.x + y.x*v.y + z.x*v.z,
    x.y*v.x + y.y*v.y + z.y*v.z,
    x.z*v.x + y.z*v.y + z.z*v.z
    )
  );
}

Vector2D
Matrix3D::operator *( const Vector2D &v ) const
{ 
  return( Vector2D( 
    x.x*v.x + y.x*v.y,
    x.y*v.x + y.y*v.y
    )
  );
}

Vector3D
Matrix3D::transposeMult( const Vector3D &v ) const
{
  return( Vector3D( 
    x.x*v.x + x.y*v.y + x.z*v.z,
    y.x*v.x + y.y*v.y + y.z*v.z,
    z.x*v.x + z.y*v.y + z.z*v.z
    )
  );
}



/***************************************************************************
*** 
*** Matrix3D
***
****************************************************************************/

/*!
  \class Matrix3D Vector.h
  \ingroup Vector
  \brief class Matrix3D provides operations on 3x3 floating point matrices.

  \sa Vector2D, Vector3D, Vector4D, Matrix3H, Matrix4D
*/

void
Matrix3D::print( const char *text ) const
{ printf( "%s", text );
  x.print( " " );
  y.print( " " );
  z.print( " " );
}

Matrix3D 
Matrix3D::operator * ( const Matrix3D &m ) const
{ 
#define mult_column(Y) \
    x.x * m.Y.x +\
    y.x * m.Y.y +\
    z.x * m.Y.z,\
\
    x.y * m.Y.x +\
    y.y * m.Y.y +\
    z.y * m.Y.z,\
\
    x.z * m.Y.x +\
    y.z * m.Y.y +\
    z.z * m.Y.z


  return( Matrix3D(
    mult_column(x),
    mult_column(y),
    mult_column(z)
    )
  );
    
#undef mult_column
}

Matrix3D
Matrix3D::inverse() const
{
  double Det;

  Det = - x.x * (y.y*z.z - z.y*y.z ) +
          x.y * (y.x*z.z - z.x*y.z ) -
          x.z * (y.x*z.y - z.x*y.y );
 
  Vector3D vX(
    ELEM3D (0,0, 1,2, 1,2),
    ELEM3D (1,0, 0,2, 1,2),
    ELEM3D (2,0, 0,1, 1,2)
    );
  
  Vector3D vY(
    ELEM3D (0,1, 1,2, 0,2),
    ELEM3D (1,1, 0,2, 0,2),
    ELEM3D (2,1, 0,1, 0,2)
    );
  
  Vector3D vZ(
    ELEM3D (0,2, 1,2, 0,1),
    ELEM3D (1,2, 0,2, 0,1),
    ELEM3D (2,2, 0,1, 0,1)
    );

  return( Matrix3D( vX, vY, vZ ) );

}

Matrix3D &
Matrix3D::rrotate( float xf, float yf, float zf )
{
#define mult_column(Y,Z) \
    y.x * Y +\
    z.x * Z, \
\
    y.y * Y +\
    z.y * Z,\
\
    y.z * Y +\
    z.z * Z

  if ( xf != 0.0 )
  { float cx = cos(xf);
    float sx = sin(xf);

    Vector3D resY( mult_column( cx, sx) );
    Vector3D resZ( mult_column(-sx, cx) );

    y = resY;
    z = resZ;
  }
#undef mult_column

#define mult_column(X,Z) \
    x.x * X +\
    z.x * Z, \
\
    x.y * X +\
    z.y * Z,\
\
    x.z * X +\
    z.z * Z

  if ( yf != 0.0 )
  { float cy = cos(yf);
    float sy = sin(yf);

    Vector3D resX( mult_column( cy, sy) );
    Vector3D resZ( mult_column(-sy, cy) );

    x = resX;
    z = resZ;
  }

#undef mult_column

#define mult_column(X,Y) \
    x.x * X +\
    y.x * Y, \
\
    x.y * X +\
    y.y * Y,\
\
    x.z * X +\
    y.z * Y

  if ( zf != 0.0 )
  { float cz = cos(zf);
    float sz = sin(zf);

    Vector3D resX( mult_column( cz, sz) );
    Vector3D resY( mult_column(-sz, cz) );

    x = resX;
    y = resY;
  }
  
  return( *this );
#undef mult_column
}

Matrix3D &
Matrix3D::rrotateX( float xf )
{
#define mult_column(Y,Z) \
    y.x * Y +\
    z.x * Z, \
\
    y.y * Y +\
    z.y * Z,\
\
    y.z * Y +\
    z.z * Z

  if ( xf != 0.0 )
  { float cx = cos(xf);
    float sx = sin(xf);

    Vector3D resY( mult_column(  cx, sx) );
    Vector3D resZ( mult_column( -sx, cx) );

    y = resY;
    z = resZ;
  }
  
  return( *this );
#undef mult_column
}

Matrix3D &
Matrix3D::rrotateY( float yf )
{
#define mult_column(X,Z) \
    x.x * X +\
    z.x * Z, \
\
    x.y * X +\
    z.y * Z,\
\
    x.z * X +\
    z.z * Z

  if ( yf != 0.0 )
  { float cy = cos(yf);
    float sy = sin(yf);

    Vector3D resX( mult_column( cy, sy) );
    Vector3D resZ( mult_column(-sy, cy) );

    x = resX;
    z = resZ;
  }

  return( *this );
#undef mult_column
}

Matrix3D &
Matrix3D::rrotateZ( float zf )
{
#define mult_column(X,Y) \
    x.x * X +\
    y.x * Y, \
\
    x.y * X +\
    y.y * Y,\
\
    x.z * X +\
    y.z * Y

  if ( zf != 0.0 )
  { float cz = cos(zf);
    float sz = sin(zf);

    Vector3D resX( mult_column( cz, sz) );
    Vector3D resY( mult_column(-sz, cz) );

    x = resX;
    y = resY;
  }
  
  return( *this );
#undef mult_column
}

Matrix3D &
Matrix3D::lrotate( float xf, float yf, float zf )
{
  if ( xf != 0.0 )
  {
    float cx = cos(xf);
    float sx = sin(xf);

    Matrix3D rt( 1.0, 0.0, 0.0,
		   0.0,  cx,  sx,
		   0.0, -sx,  cx );
    *this = rt * (*this);
  }

  if ( yf != 0.0 )
  { float cy = cos(yf);
    float sy = sin(yf);

    Matrix3D rt(  cy, 0.0,  sy,
		   0.0, 1.0, 0.0,
		   -sy, 0.0,  cy );
    *this = rt * (*this);
  }

  if ( zf != 0.0 )
  { float cz = cos(zf);
    float sz = sin(zf);

    Matrix3D rt(  cz,  sz, 0.0,
		   -sz,  cz, 0.0,
		   0.0, 0.0, 1.0 );
    *this = rt * (*this);
  }
  
  return( *this );
}

Matrix3D &
Matrix3D::lrotateX( float x )
{
  if ( x != 0.0 )
  {
    float cx = cos(x);
    float sx = sin(x);

    Matrix3D rt( 1.0, 0.0, 0.0,
		   0.0,  cx,  sx,
		   0.0, -sx,  cx );
    *this = rt * (*this);
  }

  return( *this );
}

Matrix3D &
Matrix3D::lrotateY( float y )
{
  if ( y != 0.0 )
  { float cy = cos(y);
    float sy = sin(y);

    Matrix3D rt(  cy, 0.0,  sy,
		   0.0, 1.0, 0.0,
		   -sy, 0.0,  cy );
    *this = rt * (*this);

  }
  
  return( *this );
}

Matrix3D &
Matrix3D::lrotateZ( float z )
{
  if ( z != 0.0 )
  { float cz = cos(z);
    float sz = sin(z);

    Matrix3D rt(  cz,  sz, 0.0,
		   -sz,  cz, 0.0,
		   0.0, 0.0, 1.0 );
    *this = rt * (*this);
  }
  
  return( *this );
}

Matrix3D
Matrix3D::lscale( const Matrix3D &matrix, float x, float y, float z ) const
{
  Matrix3D res = matrix;
  res.x.x *= x;
  res.x.y *= y;
  res.x.z *= z;
  res.y.x *= x;
  res.y.y *= y;
  res.y.z *= z;
  res.z.x *= x;
  res.z.y *= y;
  res.z.z *= z;
  return( res );
}


Matrix3D &
Matrix3D::lscale( float X, float Y, float Z )
{
  x.x *= X;
  x.y *= Y;
  x.z *= Z;
  y.x *= X;
  y.y *= Y;
  y.z *= Z;
  z.x *= X;
  z.y *= Y;
  z.z *= Z;
  return( *this );
}


void 
Matrix3D::rotationAngles( float &rotx, float &roty, float &rotz ) const
{
  Matrix3D      tmpx, tmpy, tmpz;
  Vector3D      local_x = x;
  Vector3D      local_z = z;
  double          length;
  double          angle_x, angle_y, angle_z = 0.0;

  length = sqrt( local_x.x*local_x.x + local_x.y*local_x.y );

  if ( length != 0.0 )
  { tmpz.x.x =  local_x.x / length;
    tmpz.y.x =  local_x.y / length;
    tmpz.x.y = -local_x.y / length;
    tmpz.y.y =  tmpz.x.x;

    angle_z = asin( -tmpz.y.x );
    
    if ( tmpz.x.x <= 0.0 )
      angle_z = ( angle_z >= 0.0 ) ? M_PI-angle_z : -M_PI-angle_z;
	
    local_x = tmpz * local_x;
    local_z = tmpz * local_z;
  }

  length = local_x.length();

  tmpy.x.x =  local_x.x / length;
  tmpy.z.x =  local_x.z / length;
  tmpy.x.z = -local_x.z / length;
  tmpy.z.z =  tmpy.x.x;

  angle_y = asin( -tmpy.z.x );
    
  if ( tmpy.x.x <= 0.0 )
    angle_y = ( angle_y >= 0.0 ) ? M_PI-angle_y : -M_PI-angle_y;
	
  local_z = tmpy * local_z;

  length = sqrt( local_z.y*local_z.y + local_z.z*local_z.z );

  tmpx.y.y =  local_z.z / length;
  tmpx.z.y =  local_z.y / length;
//  tmpx.y.z = -local_z.y / length;
//  tmpx.z.z =  local_z.z / length;

  angle_x = asin( tmpx.z.y );
    
  if ( tmpx.y.y <= 0.0 )
    angle_x = ( angle_x >= 0.0 ) ? M_PI-angle_x : -M_PI-angle_x;

  rotx = -angle_x;
  roty = -angle_y;
  rotz = -angle_z;
}









