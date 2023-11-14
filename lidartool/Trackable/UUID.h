// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef UUID_H
#define UUID_H

#include <cstdint>
#include <string>
#include <cstring>
#include <uuid/uuid.h>
#include <map>

/***************************************************************************
*** 
*** UUID
***
****************************************************************************/

typedef uint8_t uuid_app_id_t[6];

/***************************************************************************
*** 
*** UUID
***
****************************************************************************/

class UUID
{
public:
  uuid_t _uuid;
  
  static inline uuid_app_id_t   app_id       = {'T','R','A','C','K',0};
  
  static void set_app_id( uuid_app_id_t id )
  { memcpy( app_id, id, 6 );
  }

  UUID()
  {
    uuid_clear( _uuid );
  }

  UUID( uint64_t timestamp, uint32_t tid )
  {
    update( timestamp, tid );
  }

  UUID( const UUID &other, uint32_t tid )
  {
    update( other, tid );
  }

  bool operator ==( const UUID &other ) const
  {
    return uuid_compare( _uuid, other._uuid ) == 0;
  }

  bool operator !=( const UUID &other ) const
  {
    return uuid_compare( _uuid, other._uuid ) != 0;
  }

  bool operator <( const UUID &other ) const
  { return uuid_compare( _uuid, other._uuid ) < 0;
  }

  void update() 
  {
    if ( uuid_is_null( _uuid ) )
      uuid_generate_time_safe( _uuid );
  }
  
  void update( uint64_t timestamp, uint32_t tid=0 ) 
  {
    memcpy(  _uuid,     app_id, 6 );
    memcpy( &_uuid[6],  &timestamp, 6 );
    _uuid[12] = ((tid>>24)&0xff);
    _uuid[13] = ((tid>>16)&0xff);
    _uuid[14] = ((tid>> 8)&0xff);
    _uuid[15] = ((tid>> 0)&0xff);
  }
  
  void update( const UUID &other, uint32_t tid ) 
  {
    memcpy(  _uuid,     other._uuid, 14 );
    _uuid[12] = ((tid>>24)&0xff);
    _uuid[13] = ((tid>>16)&0xff);
    _uuid[14] = ((tid>> 8)&0xff);
    _uuid[15] = ((tid>> 0)&0xff);
  }
  
  std::string str()
  {
    std::string result;
    update();
    char uuid_str[37]; 
    uuid_unparse_lower( _uuid, uuid_str );
    result = uuid_str;
    return result;
  }
  
};



#endif // UUID_H

