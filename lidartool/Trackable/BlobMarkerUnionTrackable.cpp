// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#include "BlobMarkerUnionTrackable.h"

namespace pv {

bool BlobMarkerUnion::calculateDistance2D = true;

rapidjson::Value BlobMarkerUnionToJson( TrackableStage<BlobMarkerUnion> &stage, Filter::Filter &flt, BlobMarkerUnion::Type type )
{
  Trackables<BlobMarkerUnion> &objects( *stage.latest );

  rapidjson::Value      json( rapidjson::kObjectType );

  if ( flt.filterEnabled( Filter::TIMESTAMP ) )
    rapidjson::setInt64( json, flt.kmc(Filter::Timestamp), stage.lastTime );
  
  if ( flt.filterEnabled( Filter::FRAME_ID ) )
    rapidjson::setInt64( json, flt.kmc(Filter::FrameId), stage.frame_count );
  
  if ( flt.filterEnabled( Filter::BLOB_NUM_BLOBS ) )
    rapidjson::setInt( json, flt.kmc(Filter::NumBlobs), objects.size() );

#if USE_MARKER
  if ( flt.filterEnabled( Filter::MARKER_NUM_MARKERS ) )
    rapidjson::setInt( json, flt.kmc(Filter::NumMarkers), objects.size() );
#endif

  rapidjson::Value objectsJson( rapidjson::kArrayType );
  for(unsigned int i = 0; i < objects.size(); i++)
  {
#if USE_MARKER
    if ( objects[i]->type == BlobMarkerUnion::Marker )
    {
      Trackable<pv::BlobMarkerUnion> &marker( *objects[i] );
    
      rapidjson::Value markerJson( rapidjson::kObjectType );
    
      if ( flt.filterEnabled( Filter::MARKER_ID ) )
	rapidjson::setInt( markerJson, flt.kmc(Filter::MarkerId), marker.numId );
  
      if ( flt.filterEnabled( Filter::MARKER_POSITION ) )
      { 
	rapidjson::setFloat( markerJson, Filter::MarkerX, marker.matrix[3][0] );
	rapidjson::setFloat( markerJson, Filter::MarkerY, marker.matrix[3][1] );
	rapidjson::setFloat( markerJson, Filter::MarkerZ, marker.matrix[3][2] );
      }
	
      if ( flt.filterEnabled( Filter::MARKER_AXIS ) )
	rapidjson::setAxis( markerJson, flt.kmc(Filter::MarkerAxis), marker.matrix );
  
      if ( flt.filterEnabled( Filter::MARKER_SIZE ) )
	rapidjson::setFloat( markerJson, flt.kmc(Filter::MarkerSize), marker.size );
      
      objectsJson.PushBack( markerJson, rapidjson::allocator );
    }
    else if ( objects[i]->type == BlobMarkerUnion::Blob )
#endif
    {
      Trackable<pv::BlobMarkerUnion> &blob( *objects[i] );
    
      rapidjson::Value blobJson( rapidjson::kObjectType );
    
      if ( flt.filterEnabled( Filter::BLOB_ID ) )
	rapidjson::setString( blobJson, flt.kmc(Filter::BlobId), blob.id() );
  
      if ( flt.filterEnabled( Filter::BLOB_POSITION ) )
      { 
	rapidjson::setFloat( blobJson, Filter::BlobX, blob.p[0] );
	rapidjson::setFloat( blobJson, Filter::BlobY, blob.p[1] );
	if ( !isnan(blob.p[2]) && flt.filterEnabled( Filter::BLOB_3D ) )
	  rapidjson::setFloat( blobJson, Filter::BlobZ, blob.p[2] );
      }
	
      if ( flt.filterEnabled( Filter::BLOB_SIZE ) )
	rapidjson::setFloat( blobJson, flt.kmc(Filter::BlobSize), blob.size );
      
      objectsJson.PushBack( blobJson, rapidjson::allocator );
    }
  }

#if USE_MARKER
  if ( type == BlobMarkerUnion::Marker )
    json.AddMember( "markers", objectsJson, rapidjson::allocator );
  else
#endif
    json.AddMember( "blobs", objectsJson, rapidjson::allocator );

  return json;
}
  

bool BlobMarkerUnionParseJson( TrackableStage<BlobMarkerUnion> &stage,
#if USE_CAMERA
			       imCamera *camera,
#endif
			       rapidjson::Value &json )
{
#if USE_CAMERA
  float worldMatrix[4][4];
  bool  worldMatrixValid = ( camera != NULL && camera->getWorldMatrix( worldMatrix ) );
#endif
  
  uint64_t timestamp = getmsec();
  
  if ( json.HasMember(Filter::Blobs) )
  {
    stage.finish( timestamp );
    stage.swap();
      
    rapidjson::Value &blobsJson( json[Filter::Blobs] );
    if ( !blobsJson.IsNull() && blobsJson.HasMember(Filter::Blobs) )
    {
      rapidjson::Value &trackablesJson( blobsJson[Filter::Blobs] );

      if ( trackablesJson.IsArray() )
      {
	for ( int i = 0; i < trackablesJson.Size(); ++i )
        { 
	  rapidjson::Value &trackableJson( trackablesJson[i] );
	  Trackable<BlobMarkerUnion>::Ptr trackable = stage.newTrackable( timestamp );
	  trackable->type = BlobMarkerUnion::Blob;
	  trackable->fromJson( trackableJson );

#if USE_CAMERA
	  if ( worldMatrixValid )
	    trackable->transform( worldMatrix );
#endif
	}
      }
    }
  }
  
#if USE_MARKER
  if ( json.HasMember(Filter::Markers) )
  {
    stage.finish( timestamp );
    stage.swap();
      
    rapidjson::Value &markersJson( json[Filter::Markers] );
    if ( !markersJson.IsNull() && markersJson.HasMember(Filter::Markers) )
    {
      rapidjson::Value &trackablesJson( markersJson[Filter::Markers] );

      if ( trackablesJson.IsArray() )
      {
	for ( int i = 0; i < trackablesJson.Size(); ++i )
        { 
	  rapidjson::Value &trackableJson( trackablesJson[i] );
	  Trackable<BlobMarkerUnion>::Ptr trackable = stage.newTrackable( timestamp );
	  trackable->type = BlobMarkerUnion::Marker;
	  trackable->fromJson( trackableJson );

#if USE_CAMERA
	  if ( worldMatrixValid )
	    trackable->transform( worldMatrix );
#endif
	}
      }
    }
  }
#endif

  return true;
}

};

