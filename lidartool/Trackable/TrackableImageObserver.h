// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_IMAGE_OBSERVER_H 
#define TRACKABLE_IMAGE_OBSERVER_H

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#if USE_QT5
#include <QImage>
#endif


#include "TrackableObserver.h"

#define cimg_use_jpeg
#define cimg_use_png
#include "CImg/CImg.h"

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

using namespace cimg_library;

typedef float ObsvImgPixel_t;
typedef CImg<ObsvImgPixel_t> ObsvImg;
typedef CImg<unsigned char> rgbImg;

/***************************************************************************
*** 
*** TrackableImageObserver
***
****************************************************************************/

class TrackableImageObserver : public TrackableObserver
{
protected:

// SPDX-License-Identifier: Apache-2.0

// Author: Anton Mikhailov

// The look-up tables contains 256 entries. Each entry is a an sRGB triplet.

#define USE_TURBO_LUT 1

#if USE_TURBO_LUT
   const unsigned char turbo_LUT[256][3] = {{48,18,59},{50,21,67},{51,24,74},{52,27,81},{53,30,88},{54,33,95},{55,36,102},{56,39,109},{57,42,115},{58,45,121},{59,47,128},{60,50,134},{61,53,139},{62,56,145},{63,59,151},{63,62,156},{64,64,162},{65,67,167},{65,70,172},{66,73,177},{66,75,181},{67,78,186},{68,81,191},{68,84,195},{68,86,199},{69,89,203},{69,92,207},{69,94,211},{70,97,214},{70,100,218},{70,102,221},{70,105,224},{70,107,227},{71,110,230},{71,113,233},{71,115,235},{71,118,238},{71,120,240},{71,123,242},{70,125,244},{70,128,246},{70,130,248},{70,133,250},{70,135,251},{69,138,252},{69,140,253},{68,143,254},{67,145,254},{66,148,255},{65,150,255},{64,153,255},{62,155,254},{61,158,254},{59,160,253},{58,163,252},{56,165,251},{55,168,250},{53,171,248},{51,173,247},{49,175,245},{47,178,244},{46,180,242},{44,183,240},{42,185,238},{40,188,235},{39,190,233},{37,192,231},{35,195,228},{34,197,226},{32,199,223},{31,201,221},{30,203,218},{28,205,216},{27,208,213},{26,210,210},{26,212,208},{25,213,205},{24,215,202},{24,217,200},{24,219,197},{24,221,194},{24,222,192},{24,224,189},{25,226,187},{25,227,185},{26,228,182},{28,230,180},{29,231,178},{31,233,175},{32,234,172},{34,235,170},{37,236,167},{39,238,164},{42,239,161},{44,240,158},{47,241,155},{50,242,152},{53,243,148},{56,244,145},{60,245,142},{63,246,138},{67,247,135},{70,248,132},{74,248,128},{78,249,125},{82,250,122},{85,250,118},{89,251,115},{93,252,111},{97,252,108},{101,253,105},{105,253,102},{109,254,98},{113,254,95},{117,254,92},{121,254,89},{125,255,86},{128,255,83},{132,255,81},{136,255,78},{139,255,75},{143,255,73},{146,255,71},{150,254,68},{153,254,66},{156,254,64},{159,253,63},{161,253,61},{164,252,60},{167,252,58},{169,251,57},{172,251,56},{175,250,55},{177,249,54},{180,248,54},{183,247,53},{185,246,53},{188,245,52},{190,244,52},{193,243,52},{195,241,52},{198,240,52},{200,239,52},{203,237,52},{205,236,52},{208,234,52},{210,233,53},{212,231,53},{215,229,53},{217,228,54},{219,226,54},{221,224,55},{223,223,55},{225,221,55},{227,219,56},{229,217,56},{231,215,57},{233,213,57},{235,211,57},{236,209,58},{238,207,58},{239,205,58},{241,203,58},{242,201,58},{244,199,58},{245,197,58},{246,195,58},{247,193,58},{248,190,57},{249,188,57},{250,186,57},{251,184,56},{251,182,55},{252,179,54},{252,177,54},{253,174,53},{253,172,52},{254,169,51},{254,167,50},{254,164,49},{254,161,48},{254,158,47},{254,155,45},{254,153,44},{254,150,43},{254,147,42},{254,144,41},{253,141,39},{253,138,38},{252,135,37},{252,132,35},{251,129,34},{251,126,33},{250,123,31},{249,120,30},{249,117,29},{248,114,28},{247,111,26},{246,108,25},{245,105,24},{244,102,23},{243,99,21},{242,96,20},{241,93,19},{240,91,18},{239,88,17},{237,85,16},{236,83,15},{235,80,14},{234,78,13},{232,75,12},{231,73,12},{229,71,11},{228,69,10},{226,67,10},{225,65,9},{223,63,8},{221,61,8},{220,59,7},{218,57,7},{216,55,6},{214,53,6},{212,51,5},{210,49,5},{208,47,5},{206,45,4},{204,43,4},{202,42,4},{200,40,3},{197,38,3},{195,37,3},{193,35,2},{190,33,2},{188,32,2},{185,30,2},{183,29,2},{180,27,1},{178,26,1},{175,24,1},{172,23,1},{169,22,1},{167,20,1},{164,19,1},{161,18,1},{158,16,1},{155,15,1},{152,14,1},{149,13,1},{146,11,1},{142,10,1},{139,9,2},{136,8,2},{133,7,2},{129,6,2},{126,5,2},{122,4,3}};
#endif

  bool endsWithCaseInsensitive(const std::string& value, const std::string& ending) {
    if (ending.size() > value.size()) {
        return false;
    }
    return std::equal(ending.crbegin(), ending.crend(), value.crbegin(),
        [](const unsigned char a, const unsigned char b) {
            return std::tolower(a) == std::tolower(b);
        });
  }
  
  const double LOG05 = -0.69314718055994529;

  inline double _gamma( double x, double gamma )
  { return( x == 0.0 ? 0.0 : pow( x, 1.0/gamma ) ); }
  
  inline double _gain( double x, double g )
  { 
    double p = ::log(1.0-g) / LOG05;
    if ( x < 0.5 )
      return( pow(2*x,p) * 0.5 );
    else
      return( 1.0 - pow(2*(1-x), p) * 0.5 );
  }

public:

  float 	coordSpaceHeight, coordSpaceX, coordSpaceY, coordSpaceResolutionX, coordSpaceResolutionY;
  
  int	imgWidth, imgHeight, imgChannels;

  struct Context
  {
    std::string	name;
    std::string	fileTemplate;
    std::string	lastFileName;
    ObsvImg	*obsvImg;
    
    Context() : obsvImg(NULL)  {}
    ~Context()
     { if ( obsvImg != NULL )
	 delete obsvImg;
     }
    
    ObsvImg	*takeImage()
    { ObsvImg *img = obsvImg;
      obsvImg = NULL;
      return img;
    }
    
  };
  
  std::vector<Context> contexts;

  float 	minHeat;
  float 	meanFrom;
  float 	meanMap;
  float 	gamma;
  float 	gain;
  float 	minThres;
  float 	maxThres;
  int   	reportMSec;

  int		cellSize;
  float		scale;
  float		traceSize;
  float		minLen;
  float		maxLen;
  float 	coverage;
  float		opacity;
  int  		minSteps;
  int 		maxSteps;
  double	seed;
  
  float 	dim;
  float 	backgroundWeight;

  std::string	backgroundType;
  std::string	backgroundColor;
  std::string	flowmapMode;
  
  TrackableImageObserver( float x=-3.0, float y=-3.0, float width=6.0, float height=6.0 )
  : TrackableObserver(),
    imgChannels   ( 3 ),
    minHeat	  ( 0.05 ),
//    colPow           ( 0.05  ),
//    colEnhance       ( 60.0  )
    meanFrom	  ( 0.25  ),
    meanMap	  ( 0.25  ),
    gamma	  ( 1.0  ),
    gain	  ( 0.5  ),
    minThres	  ( 0.0   ),
    maxThres	  ( 0.95  ),
    reportMSec	  ( 1000  ),
    cellSize	  ( 0 ),
    scale	  ( 1.0 ),
    traceSize	  ( 0 ),
    minLen	  ( 0 ),
    maxLen	  ( 0 ),
    coverage	  ( 0.0 ),
    opacity	  ( 0 ),
    minSteps	  ( 0 ),
    maxSteps	  ( 0 ),
    seed	  ( 0.0 ),
    backgroundType(),
    backgroundColor(),
    flowmapMode	  ( "stream" ),
    dim     	  ( 0.001 ),
    backgroundWeight(  0.5 )
  {
    type = Image;
    name = "image";

    reportDistance = 0.125;
    validDuration  = 2.0;
    
    TrackableObserver::setRect( x, y, width, height );
    init();
  }

  virtual ~TrackableImageObserver()
  {
    for ( int i = 0; i < contexts.size(); ++i )
    { if ( contexts[i].obsvImg != NULL )
	delete contexts[i].obsvImg;
    }
  }
  
  virtual void addFileName( const char *name, const char *fileName="" )
  {
    for ( int i = 0; i < contexts.size(); ++i )
    { if ( contexts[i].name == name )
      { contexts[i].fileTemplate = replaceTemplates( fileName );
	return;
      }
    }
    
    contexts.push_back( Context() );
    contexts.back().name         = name;
    contexts.back().fileTemplate = replaceTemplates( fileName );
  }

  virtual void setFileName( const char *fileName )
  {
    addFileName( "file", fileName );
  }

  virtual void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    for ( int i = -1; i < 100; ++i )
    {
      char name[10];
      if ( i < 0 )
	sprintf( name, "file" );
      else
	sprintf( name, "file%d", i );

      std::string fileName;
      if ( descr.get( name, fileName ) )
	addFileName( name, fileName.c_str() );
    }
      
    descr.get( "thres", maxThres );
//    descr.get( "maxThres", maxThres );
    descr.get( "minThres", minThres );
    descr.get( "mean",  meanFrom );
    descr.get( "meanMap",  meanMap );
    descr.get( "minHeat",  minHeat );
    descr.get( "gamma", gamma );
    descr.get( "gain",  gain );

    float spaceResolution;
    if ( descr.get( "spaceResolution", spaceResolution )  )
      setSpaceResolution( spaceResolution );

    float reportSec;
    if ( descr.get( "reportSec", reportSec )  )
      reportMSec = 1000 * reportSec;
 
    descr.get( "cellSize", 	cellSize );
    descr.get( "scale", 	scale );
    descr.get( "traceSize", 	traceSize );
    descr.get( "minLen", 	minLen );
    descr.get( "maxLen", 	maxLen );
    descr.get( "coverage", 	coverage );
    descr.get( "opacity", 	opacity );
    descr.get( "minSteps", 	minSteps );
    descr.get( "maxSteps", 	maxSteps );
    descr.get( "mode",		flowmapMode );

    descr.get( "dim", 		   dim );
    descr.get( "backgroundType",   backgroundType   );
    descr.get( "backgroundColor",  backgroundColor  );
    descr.get( "backgroundWeight", backgroundWeight );
  
    if ( descr.get( "seed", seed ) )
      srand( seed * RAND_MAX );
  }
  
  virtual void setSpaceResolution( float resolution )
  { 
    if ( resolution == reportDistance )
      return;
    
    reportDistance = resolution;
    init();
  }

  virtual ObsvRect *setRect( float x=-3.0, float y=-3.0, float width=6.0, float height=6.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  { 
    if ( rect().x == x && rect().y == y && rect().width == width && rect().height == height )
      return &rect();
    
    ObsvRect *rect = TrackableObserver::setRect( x, y, width, height, edge, shape );
    init();

    return rect;
  }

  virtual ObsvRect *setRect( const char *name, float x=-3.0, float y=-3.0, float width=6.0, float height=6.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  { 
    if ( rect().x == x && rect().y == y && rect().width == width && rect().height == height )
      return &rect();
    
    ObsvRect *rect = TrackableObserver::setRect( name, x, y, width, height, edge, shape );
    init();
    
    return rect;
  }

  rgbImg rgbImgWithBackgroundFill( const unsigned int  w, const unsigned int  h, 
			       const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char a )
  {
    rgbImg res(w,h,1,4);
    unsigned char *pR = &res(0,0,0,0), *pG = &res(0,0,0,1), *pB=&res(0,0,0,2), *pA=&res(0,0,0,3);
    for (unsigned int off=res.width()*res.height(); off>0; --off)
    {
      *(pR++) = r;
      *(pG++) = g;
      *(pB++) = b;
      *(pA++) = a;
    }
    return res;
  }

  rgbImg rgbImgWithBackgroundFill( const unsigned int  w, const unsigned int  h, 
			       const unsigned char r, const unsigned char g, const unsigned char b )
  {
    rgbImg res(w,h,1,3);
    unsigned char *pR = &res(0,0,0,0), *pG = &res(0,0,0,1), *pB=&res(0,0,0,2);
    for (unsigned int off=res.width()*res.height(); off>0; --off)
    {
      *(pR++) = r;
      *(pG++) = g;
      *(pB++) = b;
    }
    return res;
  }

  rgbImg rgbImgWithBackground( const unsigned int w, const unsigned int h, 
			       unsigned char r, unsigned char g, unsigned char b, unsigned char a )
  {
    if ( !backgroundColor.empty() )
    { unsigned long val = std::stol(backgroundColor.c_str(), nullptr, 16);
      r = (val&0xff);
      g = ((val>>8)&0xff);
      b = ((val>>16)&0xff);
      a = ((val>>24)&0xff);
    }
      
    return rgbImgWithBackgroundFill( w, h, r, g, b, a );  
  }
  
  rgbImg rgbImgWithBackground( const unsigned int w, const unsigned int h, 
			       unsigned char r, unsigned char g, unsigned char b )
  {
    if ( !backgroundColor.empty() )
    { unsigned long val = std::stol(backgroundColor.c_str(), nullptr, 16);
      r = (val&0xff);
      g = ((val>>8)&0xff);
      b = ((val>>16)&0xff);
    }
      
    return rgbImgWithBackgroundFill( w, h, r, g, b );  
  }
  
  virtual ObsvImg *createImage()
  {
    return new ObsvImg( imgWidth, imgHeight, 1, 7, 0 );
  }
  
  virtual void clearImages()
  { 
    for ( int i = 0; i < contexts.size(); ++i )
    { if ( contexts[i].obsvImg != NULL )
	delete contexts[i].obsvImg;
      contexts[i].obsvImg      = createImage();
      contexts[i].lastFileName = "";
    }
  }

  virtual void  clear()
  { 
    init();
  }

  virtual void	init()
  {
    float x 	 = rect().x;
    float y 	 = rect().y;
    float width  = rect().width;
    float height = rect().height;

    float spaceResolution = reportDistance;

//    x = ceil( x / spaceResolution ) * spaceResolution;
//    y = ceil( y / spaceResolution ) * spaceResolution;
    
    width  = ceil( width  / spaceResolution ) * spaceResolution;
    height = ceil( height / spaceResolution ) * spaceResolution;
    
    int w = width  / spaceResolution;
    if ( w < 1 )
      w = 1;
    
    int h = height / spaceResolution;
    if ( h < 1 )
      h = 1;

    coordSpaceX = x + spaceResolution;
    coordSpaceY = y + spaceResolution;

    coordSpaceResolutionX = spaceResolution;
    coordSpaceResolutionY = spaceResolution;
    coordSpaceHeight      = (rect().height / (h+1)) * h;

    imgWidth  = w;
    imgHeight = h;

    clearImages();
  }
  
  void getContext( Context *&context )
  {
    if ( context == NULL )
      context = &contexts[0];
  }
  

  virtual void getCoord( int &x, int &y, float sx, float sy )
  {
    x = round( (sx-coordSpaceX) / coordSpaceResolutionX ) + 1;
    y = round( (coordSpaceY+coordSpaceHeight-sy) / coordSpaceResolutionY ) - 1;
  }

  virtual void getCoord( float &x, float &y, float sx, float sy )
  {
    x = (sx-coordSpaceX) / coordSpaceResolutionX + 1;
    y = (coordSpaceY+coordSpaceHeight-sy) / coordSpaceResolutionY - 1;
  }

  void getMinMax( ObsvImg &img, ObsvImgPixel_t &min, ObsvImgPixel_t &max, int channel=0 )
  {
    min = -1;
    max = 0;
    for ( int y = img.height()-1; y >= 0; --y )
    { for ( int x = img.width()-1; x >= 0; --x )
      { ObsvImgPixel_t value = img( x, y, 0, channel );
	if ( value > 0.0 )
	{ if ( value > max )
	    max = value;
	  if ( value < min || min < 0 )
	    min = value;
	}
      }
    }

    if ( min < 0 )
      min = max;
  }

  ObsvImgPixel_t getMean( ObsvImg &img )
  {
    double mean = 0;
    int   count = 0;
    
    for ( int y = img.height()-1; y >= 0; --y )
    { for ( int x = img.width()-1; x >= 0; --x )
      { ObsvImgPixel_t value = img( x, y, 0, 0 );
	if ( value > 0.0 )
	{ mean += value;
	  count += 1;
	}
      }
    }

    if ( count > 1 )
      mean /= count;

    return mean;
  }

  double getMean( ObsvImg &img, ObsvImgPixel_t &min, ObsvImgPixel_t &max, double meanThres, double mean )
  {
    if ( max == 0.0 )
      return 0.0;
    
    int histSize = img.width() * img.height();
    
    std::vector<ObsvImgPixel_t> hist;
    hist.resize( histSize, 0 );
    
    int index = 0;

    for ( int y = img.height()-1; y >= 0; --y )
    { for ( int x = img.width()-1; x >= 0; --x )
      { ObsvImgPixel_t value = img( x, y, 0, 0 );
	if ( value > 0 )
	{ if ( (value-=min) < 0 )
	    value = 0;
	  hist[index++] = value;
	}
      }
    }


    histSize = index;
    hist.resize( histSize );

    sort( hist.begin(), hist.end() );

    int maxIndex = (histSize-1) * meanThres;
    max =  hist[maxIndex];

    double meanValue = 0;
    int    count = 0;
    
    int meanIndex = (histSize-1) * mean;
    for ( int i = meanIndex; i < maxIndex; ++i )
    { double value = hist[i];
      meanValue += value;
      count += 1;
    }

    if ( count > 1 )
      meanValue /= count;

    return meanValue / max;
  }

  virtual rgbImg calcImage( Context *context=NULL )
  {
    getContext( context );
    
    rgbImg img( rgbImg( context->obsvImg->width(), context->obsvImg->height(), 1, imgChannels, 0 ) );

    return img;
  }

  virtual bool save( const char *fileName, Context *context )
  {
    std::string   fn( fileName );
    if ( fn.empty() )
      return true;

    std::string path( filePath( fileName ) );
    if ( !path.empty() && !fileExists( path.c_str() ) )
      std::filesystem::create_directories( path.c_str() );

    std::string pfm( ".pfm" );
    if ( endsWithCaseInsensitive( fn, pfm ) )
      return context->obsvImg->save( fileName );

    rgbImg img( calcImage(context) );
    
    return img.save( fileName );
  }

  bool saveTimed( uint64_t timestamp=0, bool force=true )
  {
    if ( timestamp == 0 )
      timestamp = getmsec();

    for ( int i = 0; i < contexts.size(); ++i )
    {
      Context &ctx( contexts[i] );
      
      if ( !ctx.fileTemplate.empty() )
      {
	std::string fileName( applyDateToString( ctx.fileTemplate.c_str(), timestamp ) );
  
	if ( ctx.lastFileName.empty() )
        { if ( ctx.obsvImg != NULL )
	    delete ctx.obsvImg;
	  ctx.obsvImg = createImage();
	  ctx.lastFileName = fileName;
        }
	else if ( ctx.lastFileName != fileName )
        {
	  if ( verbose )
	    error( "TrackableImageObserver(%s): save: %s\n", name.c_str(), ctx.lastFileName.c_str() );
	
	  if ( ctx.obsvImg != NULL )
          { save( ctx.lastFileName.c_str(), &ctx );
	    delete ctx.obsvImg;
	  }
	  ctx.obsvImg = createImage();
	  ctx.lastFileName = fileName;
	}
      }
    }
	
    return true;
  }

  virtual void doMove( uint64_t timestamp, int id, float x0, float y0, float z0, float x1, float y1, float z1, float size0, float size1, float distance, float durationSec )
  {
    int ix0, iy0, ix1, iy1;

    const double phi = 255 * 1.6180;
    const float ID = ((int)(id*phi)) % 255;
    
    getCoord( ix0, iy0, x0, y0 );
    getCoord( ix1, iy1, x1, y1 );
    
    ObsvImgPixel_t weight = 0.0;
    if ( distance < reportDistance )
      weight = durationSec;
    else
      weight = reportDistance / distance * durationSec;

    ObsvImgPixel_t pixel[7] = { weight, ID, 1, (x1-x0)/durationSec, (y1-y0)/durationSec, distance/durationSec, 1 };

    for ( int i = 0; i < contexts.size(); ++i )
      if ( contexts[i].obsvImg != NULL )
      { contexts[i].obsvImg->draw_line_op( ix0,  iy0,  ix1, iy1, pixel, 1, (1<<0)|(1<<3)|(1<<4)|(1<<5)|(1<<6) );
	contexts[i].obsvImg->draw_line_op( ix0,  iy0,  ix1, iy1, pixel, 0, (1<<1)|(1<<2) );
      }
  }
    
  
  virtual void report()
  {
    if ( type & TraceMap )
    {
      for ( int i = 0; i < contexts.size(); ++i )
	if ( contexts[i].obsvImg != NULL )
        {
	  ObsvImg *obsvImg = contexts[i].obsvImg;
	
	  for ( int y = obsvImg->height()-1; y >= 0; --y )
	    for ( int x = obsvImg->width()-1; x >= 0; --x )
	    {
	      ObsvImgPixel_t value = (*obsvImg)( x, y, 0, 2 );
	      if ( value > 0 )
	      { if ( value > dim )
		  value -= dim;
		else
		  value = 0;
		(*obsvImg)(x,y,0,2) = value;
	      }
	    }
	}
    }

    for ( auto &iter: rect().objects )
    { 
      ObsvObject &object( iter.second );

      if ( isResuming )
	object.moveDone();
      else
      {
	if ( object.isTouched() )
	{
	  int64_t duration = object.timestamp - object.timestamp0;
	
	  if ( duration > 0 && (object.d >= reportDistance || duration > reportMSec) )
          {
	    if ( isValidSpeed( duration, object.d ) )
	      doMove( object.timestamp, object.id, object.x0, object.y0, object.z0, object.x, object.y, object.z, object.size0, object.size, object.d, duration / 1000.0 );

	    object.moveDone();
	  }
	}
      }
    }

    if ( isStarted )
      saveTimed( rect().objects.timestamp );
  }

  virtual bool save( uint64_t timestamp=0 )
  {
    bool result = true;

    if ( timestamp == 0 )
      timestamp = getmsec();
 
    for ( int i = 0; i < contexts.size(); ++i )
    { Context &ctx( contexts[i] );
      if ( !ctx.fileTemplate.empty() && ctx.obsvImg != NULL )
      { std::string fileName( applyDateToString( ctx.fileTemplate.c_str(), timestamp ) );
	if ( !save( fileName.c_str(), &ctx ) )
	  result = false;
      }
    }

    return result;
  }

  virtual bool start( uint64_t timestamp=0 )
  {
    if ( !TrackableObserver::start(timestamp) )
      return false;

    clearImages();

    return true;
  }

  bool stop( uint64_t timestamp=0 )
  { 
    if ( !TrackableObserver::stop(timestamp) )
      return false;

    if ( timestamp == 0 )
      timestamp = getmsec();
 
    bool result = save( timestamp );

    clearImages();

    return result;
  }

  rgbImg heatMap( ObsvImg *obsvImg=NULL, bool fillBack=true )
  {
    if ( obsvImg == NULL )
       obsvImg = contexts[0].obsvImg;
 
    ObsvImg oImg( obsvImg->width()*scale, obsvImg->height()*scale, 1, 1, 0 );
    ObsvImg *obsImg = &oImg;

    int scalei = (int)scale;

    float spaceRes  = reportDistance / scalei;
    int radScale = (scalei > 2 ? (scalei/2) : scalei );

    int radius      = traceSize * radScale / spaceRes / 2;
//    if ( radius < 1 )
//      radius = 1;
    
    for ( int y = obsvImg->height()-1; y >= 0; --y )
    { int y0 = y * scalei + radius;
      for ( int x = obsvImg->width()-1; x >= 0; --x )
      { int x0 = x * scalei + radius;
	ObsvImgPixel_t color = (*obsvImg)( x, y, 0, 0 );
	oImg.draw_circle_op( x0, y0, radius, &color, 1 );
      }
    }
    
    ObsvImgPixel_t min, max;
    getMinMax( *obsImg, min, max );

    max -= min;
    if ( max <= 0 )
      max = 1;

//    printf( "\n" );  
//    printf( "max: %g\n", max );  

    double meanValue = getMean( *obsImg, min, max, maxThres, meanFrom );

//    printf( "max: %g mean: %g   thres: %g\n", max, meanValue, maxThres );  
//    double exponent   = 1.0;

//    if ( meanValue > 0 )
//      exponent = std::log(meanMap) / std::log( meanValue );
    
//    printf( "max: %g mean: %g   thres: %g pow: %g exponent: %g -> %g\n", max, meanValue, maxThres, meanFrom, exponent, pow( meanValue, exponent) );  

    rgbImg img ( rgbImg( obsImg->width(), obsImg->height(), 1, imgChannels, 0 ) );
    rgbImg cImg(CImg<>::jet_LUT256());

    unsigned char backColor[4];
    
    if ( !backgroundColor.empty() )
    {
      unsigned long val = std::stol(backgroundColor.c_str(), nullptr, 16);

      backColor[0] = (val&0xff);
      backColor[1] = ((val>>8)&0xff);
      backColor[2] = ((val>>16)&0xff);
      backColor[3] = ((val>>24)&0xff);
    }
    else if ( !fillBack )
    {
      backColor[0] = 0;
      backColor[1] = 0;
      backColor[2] = 0;
      backColor[3] = 0;
    }
    else
    {
#if USE_TURBO_LUT
      backColor[0] = turbo_LUT[0][0];
      backColor[1] = turbo_LUT[0][1];
      backColor[2] = turbo_LUT[0][2];
#else
      backColor[0] = cImg(0,0,0,0);
      backColor[1] = cImg(0,0,0,1);
      backColor[2] = cImg(0,0,0,2);
#endif
      backColor[3] = 255;
    }

    for ( int y = obsImg->height()-1; y >= 0; --y )
    { for ( int x = obsImg->width()-1; x >= 0; --x )
      {
	ObsvImgPixel_t sample = (*obsImg)( x, y, 0, 0 );
	ObsvImgPixel_t value = sample;
      
	if ( sample > 0.0 )
	{
	  value -= min;
	  if ( value < 0 )
	    value = 0;
	  
	  if ( value > 0 )
	  {
	    value /= max;

//	    value = pow( value, exponent );
	
	    value -= minThres;
	    
	    if ( value < 0 )
	      value = 0;
	  
	    if ( value > 0 )
	    {
	      value /= 1.0 - minThres;

	      if ( value > 1.0 )
		value = 1.0;
	
	      if ( gain != 0.5 )
		value = _gain( value, gain );
	
	      if ( gamma != 1.0 )
		value = _gamma( value, gamma );  
	    }
	  }
	  
	  value = minHeat + value * (1-minHeat);
	}
	
	const unsigned char pixVal = floor(255*value);

	if ( sample != 0.0 )
	{
#if USE_TURBO_LUT
	  img(x,y,0,0) = turbo_LUT[pixVal][0];
	  img(x,y,0,1) = turbo_LUT[pixVal][1];
	  img(x,y,0,2) = turbo_LUT[pixVal][2];	  
#else
	  img(x,y,0,0) = cImg(pixVal,0,0,0);
	  img(x,y,0,1) = cImg(pixVal,0,0,1);
	  img(x,y,0,2) = cImg(pixVal,0,0,2);
#endif
	  if ( imgChannels > 3 )
	    img(x,y,0,3) = 255;
	}
	else 
	{
	  img(x,y,0,0) = backColor[0];
	  img(x,y,0,1) = backColor[1];
	  img(x,y,0,2) = backColor[2];
	  if ( imgChannels > 3 )
	    img(x,y,0,3) = backColor[3];
	}
	
      }
    }

    return img;
  }

  rgbImg flowMapVector( ObsvImg *obsvImg=NULL )
  {
    if ( obsvImg == NULL )
      obsvImg = contexts[0].obsvImg;
    
    if ( cellSize == 0 )
      this->cellSize = 13;
    
    rgbImg img ( rgbImgWithBackground( obsvImg->width()*cellSize, obsvImg->height()*cellSize, 0, 0, 0 ) );
    rgbImg cImg(CImg<>::jet_LUT256());

    float maxSpeed =  0.0;
    float minSpeed = 10.0;
    
    for ( int y = obsvImg->height()-1; y >= 0; --y )
    { for ( int x = obsvImg->width()-1; x >= 0; --x )
      { ObsvImgPixel_t vn = (*obsvImg)( x, y, 0, 6 );
	if ( vn > 0 )
	{ ObsvImgPixel_t s = (*obsvImg)( x, y, 0, 5 );
	  s /= vn;
	  if ( s > maxSpeed )
	    maxSpeed = s;
	  else if ( s > 0.0 && s < minSpeed )
	    minSpeed = s;
	}
      }
    }
    
    if ( maxSpeed <= 0 )
      return img;

    const float speedRange = (maxSpeed - minSpeed <= 0 ? 1.0 : maxSpeed - minSpeed);
    const float maxLen   = cellSize * 0.95;
    const float minLen   = cellSize * 0.1;
    const float lenRange = maxLen - minLen;
	
    for ( int y = obsvImg->height()-1; y >= 0; --y )
    { for ( int x = obsvImg->width()-1; x >= 0; --x )
      {
	ObsvImgPixel_t vx = (*obsvImg)( x, y, 0, 3 );
	ObsvImgPixel_t vy = (*obsvImg)( x, y, 0, 4 );
	ObsvImgPixel_t vs = (*obsvImg)( x, y, 0, 5 );
	ObsvImgPixel_t vn = (*obsvImg)( x, y, 0, 6 );
      
	if ( vn > 0 )
	{
	  vx /=  vn;
	  vy /= -vn;
	  vs /=  vn;

	  float norm = sqrt( vx*vx + vy*vy );

	  if ( norm > 0.001 )
	  {
	    const unsigned char pixVal = floor(255*vs/maxSpeed);
	  
	    int x0 = x * cellSize + cellSize / 2;
	    int y0 = y * cellSize + cellSize / 2;
	  
	    float len = (vs - minSpeed) / speedRange;
	    len = minLen + len * lenRange;

	    int x1 = x0 - len * vx / norm;
	    int y1 = y0 - len * vy / norm;

	    int x2 = x0 + len * vx / norm;
	    int y2 = y0 + len * vy / norm;

	    unsigned char color[3] = {
#if USE_TURBO_LUT
	      turbo_LUT[pixVal][0],
	      turbo_LUT[pixVal][1],
	      turbo_LUT[pixVal][2]
#else
	      cImg(pixVal,0,0,0),
	      cImg(pixVal,0,0,1),
	      cImg(pixVal,0,0,2)
#endif
	    };

	    img.draw_arrow( x1,  y1,  x2, y2, color, 1, 45, -40 );
	  }
	}
      }
    }

    return img;
  }

  inline double randNorm()
  {
    return rand() / (double)RAND_MAX;
  }

  inline void coordRange( int &x, int max )
  {
    if ( x < 0 )
      x = 0;
    else if ( x >= max )
      x = max - 1;
  }

  rgbImg flowMapStream( ObsvImg *obsvImg=NULL )
  {
    if ( obsvImg == NULL )
      obsvImg = contexts[0].obsvImg;
    
    if ( cellSize == 0 )
      cellSize = 3;

    if ( this->minLen == 0 )
      this->minLen = 1.0;
    
    if ( this->maxLen == 0 )
      this->maxLen = 1.5;
    
    if ( coverage == 0 )
      coverage = 0.04;
    
    if ( minSteps == 0 )
      minSteps = 30;

    if ( maxSteps == 0 )
      maxSteps = 40;

    if ( opacity == 0 )
      opacity = 0.2;

    if ( minThres == 0 )
      minThres = 0.1;

    if ( maxThres == 0 )
      maxThres = 0.999;

    rgbImg img ( rgbImgWithBackground( obsvImg->width()*cellSize, obsvImg->height()*cellSize, 0, 0, 0, 0 ) );
    rgbImg cImg(CImg<>::jet_LUT256());
    
    int histSize = obsvImg->width() * obsvImg->height();
    std::vector<ObsvImgPixel_t> hist;
    hist.resize( histSize, 0 );
    int index = 0;

    float maxSpeed =  0.0;
    float minSpeed = 10.0;
       
    int owm1 = obsvImg->width()-1;
    int ohm1 = obsvImg->height()-1;

    for ( int y = ohm1; y >= 0; --y )
    { for ( int x = owm1; x >= 0; --x )
      { ObsvImgPixel_t vn = (*obsvImg)( x, y, 0, 6 );
	if ( vn > 0 )
	{ ObsvImgPixel_t s = (*obsvImg)( x, y, 0, 5 );
	  s /= vn;
	  if ( s > maxSpeed )
	    maxSpeed = s;
	  else if ( s > 0.0 && s < minSpeed )
	    minSpeed = s;
	  hist[index++] = s;
	}
      }
    }
    
    if ( maxSpeed <= 0 )
      return img;

    histSize = index;
    hist.resize( histSize );

    sort( hist.begin(), hist.end() );

    int minIndex = (histSize-1) * minThres;
    minSpeed =  hist[minIndex];

    int maxIndex = (histSize-1) * maxThres;
    maxSpeed =  hist[maxIndex];

    const float speedRange = (maxSpeed - minSpeed <= 0 ? 1.0 : maxSpeed - minSpeed);
    const float maxLen   = this->maxLen;
    const float minLen   = this->minLen;
    const float lenRange = maxLen - minLen;

    const int    numSamples = coverage * (img.width() * img.height());

    if ( seed != 0 )
      srand( seed * RAND_MAX );

    for ( int i = numSamples-1; i >= 0; --i )
    {
      double rx = randNorm();
      double ry = randNorm();
      double rs = randNorm();

      int steps = minSteps + (maxSteps-minSteps) * rs;

      for ( int s = steps; s > 0; --s )
      {
	int ox = round(owm1 * rx);
	int oy = round(ohm1 * ry);
      
	int x0 = round((img.width() -1) * rx);
	int y0 = round((img.height()-1) * ry);

	coordRange( ox, obsvImg->width() );
	coordRange( oy, obsvImg->height() );
	
	coordRange( x0, img.width()  );
	coordRange( y0, img.height() );

	ObsvImgPixel_t vx = (*obsvImg)( ox, oy, 0, 3 );
	ObsvImgPixel_t vy = (*obsvImg)( ox, oy, 0, 4 );
	ObsvImgPixel_t vs = (*obsvImg)( ox, oy, 0, 5 );
	ObsvImgPixel_t vn = (*obsvImg)( ox, oy, 0, 6 );
      
	if ( vn > 0 )
        {
	  vx /=  vn;
	  vy /= -vn;

	  float norm = sqrt( vx*vx + vy*vy );

	  if ( norm > 0.001 )
          {
	    vs /=  vn;

	    double value = vs/maxSpeed;

	    value = value * (minThres+1) - minThres;

	    if ( value < 0 )
	      value = 0;
	    else if ( value > 1.0 )
	      value = 1.0;
	    
	    value = minHeat + value * (1-minHeat);
	    const unsigned char pixVal = floor(255*value);
	  
	    unsigned char color[4] = { 
#if USE_TURBO_LUT
	      turbo_LUT[pixVal][0],
	      turbo_LUT[pixVal][1],
	      turbo_LUT[pixVal][2],
#else
	      cImg(pixVal,0,0,0),
	      cImg(pixVal,0,0,1),
	      cImg(pixVal,0,0,2),
#endif
	      255
	    };

	  
	    float len = (vs - minSpeed) / speedRange;
	    len = minLen + len * lenRange;

	    rx += len * vx / norm / (obsvImg->width() -1);
	    ry += len * vy / norm / (obsvImg->height()-1);

	    int x1 = round((img.width() -1) * rx);
	    int y1 = round((img.height()-1) * ry);

	    img.draw_line( x0, y0, x1, y1, color, opacity );
//	    img.draw_arrow( x0,  y0,  x1, y1, color, 0.2, 45, -30 );

	    x0 = x1;
	    y0 = y1;

	    ox = x0 / cellSize;
	    oy = y0 / cellSize;
	  }
	  else
	    s = 0;
	}
	else
	  s = 0;
      }
    }

    return img;
  }

  rgbImg flowMap( ObsvImg *obsvImg=NULL )
  {
    if ( flowmapMode == "vector" )
      return flowMapVector( obsvImg );

    return flowMapStream( obsvImg );
  }

  rgbImg tracemap( ObsvImg *obsvImg=NULL )
  {
    if ( obsvImg == NULL )
      obsvImg = contexts[0].obsvImg;
    
    rgbImg img;
    if ( backgroundWeight > 0 && !backgroundType.empty() )
    {
      if ( backgroundType == "heatmap" )
	img = heatMap( obsvImg, false );
      else
	img = flowMap( obsvImg );
    }
    else
      img = rgbImgWithBackground( obsvImg->width()*cellSize, obsvImg->height()*cellSize, 0, 0, 0, 0 );
      
    rgbImg cImg(CImg<>::jet_LUT256());

    if ( cellSize == 0 )
      cellSize = 1;

    for ( int y = obsvImg->height()-1; y >= 0; --y )
    { for ( int x = obsvImg->width()-1; x >= 0; --x )
      {
	ObsvImgPixel_t value = (*obsvImg)( x, y, 0, 2 );
	  
	ObsvImgPixel_t id    = (*obsvImg)( x, y, 0, 1 );
	value = pow( value, 0.25 );

	unsigned char pixVal = id;
	
	if ( pixVal < 0 )
	  pixVal = 0;
	else if ( pixVal > 255 )
	  pixVal = 255;

	if ( backgroundWeight > 0 )
	{ const float hm = backgroundWeight;
	  img(x,y,0,0) = hm * (1-value) * img(x,y,0,0) + value * cImg(pixVal,0,0,0);
	  img(x,y,0,1) = hm * (1-value) * img(x,y,0,1) + value * cImg(pixVal,0,0,1);
	  img(x,y,0,2) = hm * (1-value) * img(x,y,0,2) + value * cImg(pixVal,0,0,2);
	  if ( imgChannels > 3 )
	    img(x,y,0,3) = 255;
	}
	else
	{ img(x,y,0,0) = value * cImg(pixVal,0,0,0);
	  img(x,y,0,1) = value * cImg(pixVal,0,0,1);
	  img(x,y,0,2) = value * cImg(pixVal,0,0,2);
	  if ( imgChannels > 3 )
	    img(x,y,0,3) = 255;
	}
      }
    }
  
    return img;
  }

};

/***************************************************************************
*** 
*** TrackableHeatMapObserver
***
****************************************************************************/

class TrackableHeatMapObserver : public TrackableImageObserver
{
public:

  TrackableHeatMapObserver( float x=-3.0, float y=-3.0, float width=6.0, float height=6.0 )
  : TrackableImageObserver( x, y, width, height )
  {
    type |=  HeatMap;
    name  = "heatmap";
  }
  
  rgbImg calcImage( Context *context=NULL )
  {
    getContext( context );
    
    return heatMap( context->obsvImg );
  }

};


/***************************************************************************
*** 
*** TrackableFlowMapObserver
***
****************************************************************************/

class TrackableFlowMapObserver : public TrackableImageObserver
{
public:

  TrackableFlowMapObserver( float x=-3.0, float y=-3.0, float width=6.0, float height=6.0 )
  : TrackableImageObserver( x, y, width, height )
  {
    maxThres = 0;
    type |=  FlowMap;
    name  = "flowmap";
  }
  
  rgbImg calcImage( Context *context=NULL )
  {
    getContext( context );
    
    return flowMap( context->obsvImg );
  }


};


/***************************************************************************
*** 
*** TrackableTraceMapObserver
***
****************************************************************************/

class TrackableTraceMapObserver : public TrackableHeatMapObserver
{
public:

  TrackableTraceMapObserver( float x=-3.0, float y=-3.0, float width=6.0, float height=6.0 )
  : TrackableHeatMapObserver( x, y, width, height )
  {
    type &=  ~HeatMap;
    type |=  TraceMap;
    name  = "tracemap";

    reportMSec = 250;
  }
  

  rgbImg calcImage( Context *context=NULL )
  {
    getContext( context );
    
    return tracemap( context->obsvImg );
  }

};


#endif


