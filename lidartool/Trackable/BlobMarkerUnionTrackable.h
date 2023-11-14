// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef BLOBMARKERUNION_TRACKABLE_H
#define BLOBMARKERUNION_TRACKABLE_H

#if USE_MARKER
#include "markerTool.h"

#include "linmath.h"
using namespace linmath;

#endif

#include "filterTool.h"
#include "Trackable.h"

namespace pv {

class BlobMarkerUnion
{
public:
  typedef std::shared_ptr<BlobMarkerUnion> Ptr;

  float p[3];
  float size;
  int   type;
  int   numId;
#if USE_MARKER
  float matrix[4][4];
#endif
  enum Type{
    Blob,
    Marker
  };

  static bool calculateDistance2D;

  typedef float vec3[3];
  typedef float vec4[4];
  typedef vec4 mat4x4[4];

  inline void mat4x4_mul_vec4(vec4 r, mat4x4 M, vec4 v)
  {
    int i, j;
    for(j=0; j<4; ++j) {
        r[j] = 0.f;
        for(i=0; i<4; ++i)
            r[j] += M[i][j] * v[i];
    }
  }

  inline void mat4x4_mul_vec3(vec4 r, mat4x4 M, vec3 v)
  {
    vec4 r4, v4 = { v[0], v[1], v[2], 1.0f };
    mat4x4_mul_vec4(r4, M, v4 );
    r[0] = r4[0];
    r[1] = r4[1];
    r[2] = r4[2];
  } 

  BlobMarkerUnion( int t=Blob )
    : size( 0.1 ),
      type( t )
  {
    p[0] = 0.0;
    p[1] = 0.0;
    p[2] = NAN;
 
#if USE_MARKER
    mat4x4_identity( matrix );
#endif
  }

#ifdef TRACKABLE_LOGGER_H
  void getLogInfo( TrackableLogObject &logObject )
  {
    logObject.x    = p[0];
    logObject.y    = p[1];
    logObject.z    = p[2];
    logObject.size = size;
  }
#endif

#ifdef TRACKABLE_OBSERVER_H
  void getObservInfo( ObsvObject &obsvObject )
  {
    obsvObject.x    = p[0];
    obsvObject.y    = p[1];
    obsvObject.z    = p[2];
    obsvObject.size = size;
  }
#endif

  double distanceTo( BlobMarkerUnion &other, float offsetX=0.0f, float offsetY=0.0f, float offsetZ=0.0f )
  {
    if ( type == Marker )
    {
      if ( numId == other.numId )
	return 0.0;
      
      return 1000000.0;
    }
    
    double d0 = p[0] - other.p[0] + offsetX;
    double d1 = p[1] - other.p[1] + offsetY;
    
    if ( calculateDistance2D || std::isnan(p[2]) )
      return sqrt( d0*d0 + d1*d1 );
    
    double d2 = p[2] - other.p[2] + offsetZ;

    return sqrt( d0*d0 + d1*d1 + d2*d2 );
  }
  
  void mixWith( BlobMarkerUnion &other, double weight=-1.0 )
  {
    if ( weight < 0 )
    { weight = 0.5;
      if ( size > 0 && other.size > 0 )
	weight = size / (size + other.size);
    }
    
    double oneMinusWeight = 1.0-weight;
    
    p[0] = weight * p[0] + oneMinusWeight * other.p[0];
    p[1] = weight * p[1] + oneMinusWeight * other.p[1];
    if ( !isnan(p[2]) )
      p[2] = weight * p[2] + oneMinusWeight * other.p[2];

//    printf( "size: %g %g\n", size, other.size );

    size = weight * size + oneMinusWeight * other.size;
    
#if USE_MARKER
    linmath::mat4x4_mix( matrix, matrix, other.matrix, weight );
#endif
  }
  
  void transform( float transform[4][4] )
  {
    mat4x4_mul_vec3( p, transform, p );
#if USE_MARKER
    mat4x4_mul( matrix, transform, matrix );
#endif
  }

  bool fromJson( rapidjson::Value &json )
  {
    if ( rapidjson::fromJson( json, "x", p[0] ) )
#if USE_MARKER
      matrix[3][0] = p[0]
#endif
      ;
    if ( rapidjson::fromJson( json, "y", p[1] ) )
#if USE_MARKER
      matrix[3][1] = p[1]
#endif
      ;
    if ( rapidjson::fromJson( json, "z", p[2] ) )
#if USE_MARKER
      matrix[3][2] = p[2]
#endif
      ;

    rapidjson::fromJson( json, "size", size );

#if USE_MARKER
    if ( type == Marker )
    {
      rapidjson::fromJson( json, "marker_id", numId  );

      if ( rapidjson::fromJson( json, "matrix", matrix ) )
      { p[0] = matrix[3][0];
	p[1] = matrix[3][1];
	p[2] = matrix[3][2];
      }

      rapidjson::axisFromJson( json, "axis", matrix );
    }
#endif
    
    return true;
  }
  
  rapidjson::Value toJson()
  {	
    rapidjson::Value json( rapidjson::kObjectType );
    
#if USE_MARKER
    if ( type == Marker )
    { rapidjson::setInt   ( json, "marker_id", numId );
      rapidjson::setMatrix( json, "matrix",    matrix );
    }
    else
#endif
    {
      rapidjson::setFloat( json, "x", p[0] );
      rapidjson::setFloat( json, "y", p[1] );
      if ( !isnan(p[2]) )
	rapidjson::setFloat( json, "z", p[2] );
    }
    
    rapidjson::setFloat( json, "size", size );

    return json;
  }

};

/***** READER ****/

extern bool BlobMarkerUnionParseJson( TrackableStage<BlobMarkerUnion> &stage,
#if USE_CAMERA
				      imCamera *camera,
#endif
				      rapidjson::Value &json );
extern rapidjson::Value BlobMarkerUnionToJson( TrackableStage<BlobMarkerUnion> &stage, Filter::Filter &flt, BlobMarkerUnion::Type type );


class BlobMarkerUnionReader : public TrackableJsonReader<BlobMarkerUnion>
{
public:
  bool parseJson( TrackableStage<BlobMarkerUnion> &stage,
#if USE_CAMERA
		  imCamera *camera, 
#endif
		  rapidjson::Value &json )
  {
    return BlobMarkerUnionParseJson( stage, 
#if USE_CAMERA
				     camera,
#endif
				     json);
  }
};
  

};


#endif // BLOBMARKERUNION_TRACKABLE_H

