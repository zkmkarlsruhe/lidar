// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#include <httpserver.hpp>
#include "lidarKit.h"
#include "lidarTrack.h"

#define cimg_use_jpeg
#define cimg_use_png
#include "CImg/CImg.h"

typedef cimg_library::CImg<unsigned char> rpImg;

class LidarPainter
{
public:

  Matrix3H matrix,    matrixInv;
  float cx, cy;
  float extent;
  float extent_x, extent_y;
  int   width, height;
  int   canv_width, canv_height;
  int   sampleRadius;
  int	objectRadius;
  bool  showPoints;
  bool  showGrid;
  bool  showLines;
  bool  showObjects;
  bool  showObjCircle;
  bool  showConfidence;
  bool  showCurvature;
  bool  showSplitProb;
  bool  showLifeSpan;
  bool  showMotion;
  bool  showMotionPred;
  bool  showMarker;
  bool  showDevices;
  bool  showDeviceInfo;
  bool  showObserverStatus;
  bool  showTracking;
  bool  showRegions;
  bool  showStages;
  bool  showEnv;
  bool  showEnvThres;
  bool  showCoverage;
  bool  showCoveragePoints;
  bool  showObstacles;
  bool  showPrivate;
  bool  showControls;
  bool  showOutline;
  
  bool	viewUpdated;

  std::set<std::string> layers;

  rpImg *img; 
  
  uint64_t lastAccess;
  std::string	uiImageFileName;

  LidarPainter();
  ~LidarPainter();

  void  setUIImageFileName( const char *type, const char *key );
  
  void	updateExtent();
  void	begin();
  void  end  ();
  
  void  getCoord( float &sx, float &sy, int x, int y );
  void  getCoord( int &x, int &y, float sx, float sy );
  void  getCanvCoord( int &x, int &y, float sx, float sy );

  void  paintEnv( LidarDevice &device );
  void  paintCoverage( LidarDevice &device );
  void  paintDevice  ( LidarDevice &device );
  void  paintMarker  ( LidarDevice &device );
  void  paintBlobMarkerUnion( pv::Trackable<pv::BlobMarkerUnion> &object, int colorIndex=-1, bool showLabel=true, bool drawMotion=false, uint64_t timestamp=0, bool drawConfidence=false, bool drawCircle=true );
  void  paintStage( pv::TrackableStage<pv::BlobMarkerUnion> &stage, int colorIndex=-1, bool showLabel=true, bool drawMotion=false, uint64_t timestamp=0 );
  void  paintMultiStage( pv::TrackableMultiStage<pv::BlobMarkerUnion> &stage, bool showTracking=true, bool substages=false, int colorIndex=-1, bool drawMotion=false );
  void  paintObstacles();

  void  paintGrid();
  void  paintAxis();

  void	paint( LidarDevice &device );
  void	paint( TrackableRegion  &region  );
  void	paint( TrackableRegions &regions );
  
};

