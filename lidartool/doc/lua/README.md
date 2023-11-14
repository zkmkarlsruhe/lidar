Lua
=========

### Lua Observer: @type=lua

| Type   | Parameter       | Description     |
|:------ |:--------------- |:--------------- |
| script | lua script file | the script file |

inside the lua script, any other parameter are accessible via

- obsv.param.*type*(  *name*, *defaultValue* )

for example:

```
verbose = obsv.param.bool( "verbose", true )
runMode = obsv.param.string( "runMode", "setup" )
scale   = obsv.param.number( "scale", 1.0 )
count   = obsv.param.integer( "count", 1 )
```

Function `init()` is called on script loading.

```
function init()
   print( "init lua" )
   lastcount = -1
end
```

Functions `start( timestamp )` and `stop( timestamp )` are called on start and stop of observers.

```
function start( timestamp )
   print( obsv.timestamp('%c',timestamp) .. ": " .. tostring(timestamp) .. " start lua observer" )
end

function stop( timestamp )
   print( obsv.timestamp('%c',timestamp) .. ": " .. tostring(timestamp) .. " stop lua observer" )
end
```

function  `observe( timestamp )` is called on each evaluation of the observer.

```
function observe( timestamp )
   print( obsv.timestamp('%c',timestamp) .. ": observe" )

   for i=obsv.numRegions()-1,0,-1
   do
      local objects = obsv.objects( i );
      local region  = objects:region()
      print( region )

      for o=objects:size()-1,0,-1
      do
         local object = objects[o]
         if object:hasMoved() then
             print( "objects(" .. object:id() .. ")  x = " .. object:x() .. ", y = " .. object:y() )
             object:moveDone()
          end
      end

      print( "switch(" .. region .. "): " .. tostring( objects:switch() ) )

      local count = objects:count()
      if count ~= lastcount then
          print( "count(" .. region .. "): " .. tostring( objects:count() ) )
          lastcount = count
      end
   end
end
```

## Working with Objects

Functions `objectsStart( objects, timestamp )` and `objectsStop( objects, timestamp )` are called on start and stop of observers on the objects of each region. If no region is defined, there are default objects.

objects are persistent and hold a table where you can store region dependend data:

```
function objectsStart( objects, timestamp )
    objects.startTime = timestamp
   print( obsv.timestamp('%c',timestamp) .. ": start lua observer" )
end

function objectsStop( objects, timestamp )
   print( obsv.timestamp('%c',timestamp) .. ": stop lua observer, ran " .. tostring(timestamp-objects.startTime) .. " msec" )
end
```
