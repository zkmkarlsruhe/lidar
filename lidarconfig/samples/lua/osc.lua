local osc    = require("losc").new()
local packet = require("losc.packet")

local socket = require("socket")
local udp    = assert(socket.udp())


local lastReportTime  = 0

function init()

   verbose  = obsv.verbose

   if verbose then
      print( "connecting to host " .. obsv.param.host .. ":" .. obsv.param.port )
   end
   
   udp:settimeout(0.01)
   assert(udp:setsockname("*",0))
   assert(udp:setpeername(obsv.param.host, obsv.param.port))

end

function header( objects, pattern )

   local address = "/" .. objects:regionName() .. "/" .. pattern
   return address

end


function sendMsg( message )

  local msg = assert(packet.pack(message))
  udp:send( msg )

end

function sendSpace( objects )

   local rx = objects:regionX()
   local ry = objects:regionY()
   local rw = objects:regionWidth()  * 0.5
   local rh = objects:regionHeight() * 0.5

   local message = osc.new_message {
      address =  header(objects,'space'),
      types = 'ffff',
      objects:regionX(),
      objects:regionY(),
      objects:regionWidth(),
      objects:regionHeight()
   }

   sendMsg( message )

end

function sendClear( objects )

   local message = osc.new_message {
      address = oscAddress(objects,'clear'),
      types = ''
   }

   sendMsg( message )

end

function objectsStart( objects )
   sendClear( objects )
   sendSpace( objects )
end

function objectsStop( objects )
   sendClear( objects )
end

function objectsObserve( objects, timestamp )

   local message = osc.new_message {
      address =  header(objects,'objects'),
      types = ''
   }

   for o=0,objects:size()-1
   do
      local object = objects[o]

      message:add( 'i', object:id() )
      message:add( 'f', object:x()  )
      message:add( 'f', object:y()  )

   end

   sendMsg( message )

end

function start( timestamp )

--   log( timestamp, "\"action\":\"start\"" )

end

function stop( timestamp )

--   log( timestamp, "\"action\":\"stop\"" );

end




