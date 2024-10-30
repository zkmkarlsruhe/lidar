local socket = require("socket")
local udp    = assert(socket.udp())

local lastReportTime = 0

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

   local msg = "/" .. objects:regionName() .. "/" .. pattern
   return msg

end


function sendMsg( msg )

   udp:send( msg .. "\n" )

end

function sendSpace( objects )

   local rx = objects:regionX()
   local ry = objects:regionY()
   local rw = objects:regionWidth()  * 0.5
   local rh = objects:regionHeight() * 0.5

   local message = header(objects,'space')
   message = message .. " " .. objects:regionX()
   message = message .. " " .. objects:regionY()
   message = message .. " " .. objects:regionWidth()
   message = message .. " " .. objects:regionHeight()

   sendMsg( message )

end

function sendClear( objects )

   local msg = header(objects,'frame')
   msg = msg .. " []";

   sendMsg( msg )

end


function objectsStart( objects )
   sendClear( objects )
   sendSpace( objects )
end

function objectsStop( objects )
   sendClear( objects )
end

function roundedCoord( value )
   local resolution = 10000.0
   return math.floor( value * resolution ) /  resolution 
end

function objectsObserve( objects, timestamp )

   local rx = objects:regionX()
   local ry = objects:regionY()
   local rw = objects:regionWidth()
   local rh = objects:regionHeight()

   local msg  = header(objects,"frame")
   msg = msg .. " [";
   for o=0,objects:size()-1
   do
      local object = objects[o]

      if o ~= 0 then
	 msg = msg .. ","
      end
	
      local x = object:x()
      local y = object:y()

      if true then
	 x = x / rw + 0.5
	 y = y / rh + 0.5
      end
      
      x = roundedCoord( x )
      y = roundedCoord( y )

      msg = msg .. "[" .. x .. "," .. y .. "]"

   end

   msg = msg .. "]"
  
   if verbose and timestamp-lastReportTime > 333 then -- report 3 times per sec
      lastReportTime = timestamp
      print( obsv.timestamp('%c',timestamp) .. ": " .. msg )
   end

   sendMsg( msg )

end

function start( timestamp )

--   log( timestamp, "\"action\":\"start\"" )

end

function stop( timestamp )

--   log( timestamp, "\"action\":\"stop\"" );

end

