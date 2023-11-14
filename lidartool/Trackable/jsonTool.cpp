// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef JSON_TOOL_CPP
#define JSON_TOOL_CPP


#include "jsonTool.h"

namespace rapidjson
{

Document::AllocatorType allocator( Document().GetAllocator() );

}

#endif // JSON_TOOL_CPP
