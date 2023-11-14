// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _SCAN_DATA_H_
#define _SCAN_DATA_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include <stdint.h>
#include <vector>

/***************************************************************************
*** 
*** ScanPoint
***
****************************************************************************/

struct ScanPoint {
  float 	angle;
  float		distance;
  int		quality;

};

typedef std::vector<ScanPoint> ScanData;

/***************************************************************************
*** 
*** ScanData
***
****************************************************************************/


#endif

