Observers
=========

Observers are a mechanism to report tracking information to external applications. Each observer implements a specific communication channel, e.g. using websockets, UDP, or OSC. There can be an arbitrary number of observers, even using the same protocol but with different parameters. One observer could be used to send OSC data to a realtime graphics application, while in parallel another observer sends UDP messages to another application that switches a light on when a person enters the room.

Regions
-------

To each observer, an arbitrary number of regions can be assigned which mask the tracking data such that object motion is reported by this observer only if the object is in one of these regions. Entering and leaving is reported for each region separately; regions then should not overlap.

Regions are rectangles in the world coordinate system which are globally defined within the lidar tool and can be assigned to several observers. Thus a set of regions can be used by one observer to switch audio while another logs visitor movement to a file. Regions are created via command line interface and can be edited in the Web GUI.

See [Regions](../regions) for more information.

## Observer Definition

Observers are defined in the command line when starting the lidar tool. Alternatively the definition can be stored in a configuration file, which is loaded on demand during startup of the lidar tool with setting the argument `+useObservers` for all observers or `+useObserver name` for a single observer.

## Definition in Command Line

In the command line, an observer is defined with a filter and a list of parameters. A parameter is defined by a key value pair preceded by an `@`. Example: `@script=SwitchAudio.sh`

#### Observer Types

The *type* of an observer is defined with the parameter *@type=XXX*. Type is one of the following:

| Type              | Description                                                                   | Format                |
|:----------------- |:----------------------------------------------------------------------------- |:--------------------- |
| `file`            | writes log information to a file                                              | JSON                  |
| `bash`            | calls a bash script if the occupation or number of people in a region changes | Bash Script Execution |
| `mqtt`            | publishes infos to an MQTT server                                             | JSON                  |
| `websocket`       | opens a websocket port as server                                              | JSON                  |
| `osc`             | sends OSC messages to a server                                                | OSC Message Type      |
| `udp`             | sends UDP messages to a server                                                | String                |
| `heatmap`         | writes heatmap images to the file system                                      | JPEG                  |
| `flowmap`         | writes flowmap images to the file system                                      | JPEG                  |
| `packedfile`      | writes tracking data in packed object format to the file system               | pkf                   |
| `packedwebsocket` | opens a websocket port as a server                                            | pkf                   |
| `lua`             | calls Lua script for observer evaluation                                      | Lua Script Execution  |

#### Observer Names

Names are used to distinguish reference observers in operations: `@name=myName`

#### Debug Observers

Set observer parameter `@verbose=true` to print observer debug information on the console.

#### Observer Common Parameter

These parameters are common to all observers:

| Parameter            | Type                    | Description                                                                                                           |
|:-------------------- |:----------------------- |:--------------------------------------------------------------------------------------------------------------------- |
| `verbose`            | bool                    | print observer debug information on the console                                                                       |
| `reporting`          | bool                    | switch on or off value reporting                                                                                      |
| `fullFrame`          | bool                    | `true`: all frame information is reported within one message<br>`false`: split frame information in separate messages |
| `continuous`         | bool                    | `true`: report values every frame<br>`false`: report value only on value change                                       |
| `regions`            | comma separated strings | regions the observer is subscribed to                                                                                 |
| `regionCentered`     | bool                    | `true`: report positions relative to the region center                                                                |
| `reportDistance`     | float                   | if not in `continuous` mode, report *move* only if position change is larger than `reportDistance`                    |
| `maxFPS`             | float                   | maximum frame rate in which reports happen                                                                            |
| `smoothing`          | float                   | smoothing weight for positional and size values [0..1]; with larger being smoother, (default=0)                       |
| `showSwitchStatus`   | bool                    | `true`: current switch value for observer regions will be shown in UI                                                 |
| `showCountStatus`    | bool                    | `true`: current count value for observer regions will be shown in UI                                                  |
| `aliveTimeout`       | float                   | time frequency in sec for alive signal                                                                                |
| `requestedDevices`   | comma separated strings | device nik names which belong to this observer for operational calculation                                            |
| `runMode`            | string                  | set by `StartServer.sh` with the current run mode                                                                     |
| `alwaysOn`           | bool                    | `true`: observer will never be stopped                                                                                |
| `operationalDevices` | string                  | comma separated list of device names necessary for this observer to function correctly                                |
| `scheme`             | string                  | scheme definition                                                                                                      |
| `schemeFile`         | string                  | file with the scheme definition                                                                                       |

#### Regions

Observers can be restricted to regions which are defined as a comma separated list of region names, e.g. @regions='Region1,Region2,Region3'

Enclose in ' ' if names contain spaces: `@regions='Region 1,Region 2,Region 3'`

Several Regions can be united by assigning them a region name: `@regions=Region1,Region2,Region3=UnitedRegion`

#### Multiple Parameter

Multiple parameters can be combined in a single statement:
`@{type=bash,name=SomeName,script=SwitchAudio.sh,regions='Region1,Region2,Region3'}`

### Filter

Define a filter for the data fields which an observer should take into account. A filter is a comma separated list of tags preceded with a colon:

`:switch,region`

Optionally a tag can have an alias which is a replacement for the tag name when the tag name is reported:

`:timestamp=ts,action=running,start=true,stop=false`

### Example

A full command line example:

```console
> ./lidarTool +d 0 +track +observer :switch,region @{type=bash,name=myObserver,script=SwitchAudio.sh,regions='Region1,Region2,Region3'}
```

## Observer Definition

### StartServer.sh

If you use `lidarAdmin` to manage your LiDARs, put your observer definitions in a text file `observer.txt` in the configuration directory. The script `StartServer.sh` sources the files `observer.txt `and `conf/observer.txt` if they exist.

Example of an `observer.txt`:

```console
# add a osc observer
observer+=(+observer @type=osc :frame,frame_id,frame_end,object,move,id,x,y,z,size @name=my_osc @url=osc.udp://localhost:8000 @regions='myRegion')

# add a bash observer
observer+=(+observer @type=bash :switch,region @name=my_switch @script=$conf/Switch.sh @regions='myRegion')
```

## Configuration File

The definition of observers via the command line can quickly become very unwieldy and confusing, therefore observers can also be defined in a configuration file. When starting lidartool, you only need to specify that the observer definition is to be read from the configuration file.

The observer configuration file is: `conf/observer.json`

There are operations to create, delete, and rename observers as well as set observer parameters. Like in the CLI, certain parameters have special meanings such `type`, `name`, etc.

#### Use Observers

To use the observer's configuration file start lidarTool with the option `+useObservers`

#### Create and Set Observer Parameters

Set observer parameters with `+setObserverValues` followed by the *name* of the observer and observer parameters in the same format as on the command line. The observer will be created if it does not already exist:

```console
> ./lidarTool +setObserverValues myObserver :switch,region @{type=bash,script=SwitchAudio.sh,regions='Region1,Region2,Region3'}
```

#### List All Observers

To list all observers defined in the configuration file:

```console
> ./lidarTool +listObservers
```

#### Set Observer Parameter Value

To overwrite an observer value use ``+setObserverValue`` with a new value:

```console
> ./lidarTool +setObserverValue myObserver @script=scriptBak
> ./lidarTool +setObserverValue myObserver :switch,region
```

#### Rename Observer

To rename the observer use ``+renameObserver oldName newName``:

```console
> ./lidarTool +renameObserver myObserver SwitchAudio
```

#### Delete Observer

To delete an observer entry in the configuration file, type ``+removeObserver name``:

```console
> ./lidarTool +removeObserver SwitchAudio
```

#### Remove Observer Parameter

To delete a single observer parameter ``+removeObserverValue observerName parameterName``:

```console
> ./lidarTool +removeObserverValue SwitchAudio script
```

## Observer Types and Type Specific Parameter

### File Observer: @type=file

| Type | Parameter      | Description                                |
|:---- |:-------------- |:------------------------------------------ |
| file | @file=fileName | file name or template to write log data to |

File names can contain placeholders for date information, formatted in the Unix date format (see 'date -h').

The file name `log/log_%Y-%m-%d.log` expands at the 1st of June 2022 to `log/log_2022-06-01.log`. If a date-dependent file name changes with the next log entry, a new file is created. Thus one can create log files on a monthly or daily basis.

The given file names can be a real file name or a synonym:

```console
  monthly    synonym for '%Y-%m'
  weekly     synonym for '%Y-%V'
  daily      synonym for '%Y-%m-%d'
  hourly     synonym for '%Y-%m-%d-%H:00'
  minutely   synonym for '%Y-%m-%d-%H:%M'
```

Example:

```console
> date
Fr 18. Mär 09:55:43 CET 2022
> ./lidarTool +d 0 +track +observer :timestamp,action,start,stop,object,enter,move,leave,id,x,y @type=file @file=log/log_%daily.log 
```

Will create log files like log/log_2022-03-18.log, log/log_2022-03-19.log, log/log_2022-03-20.log ...

Text log files are stored line by line in JSON format:

```console
{"timestamp":1644302992879,"action":"start"}
{"timestamp":1644302995748,"object":{"x":-0.6,"y":0.08,"z":0.0,"size":0.587,"type":"enter","id":3}}
{"timestamp":1644302996004,"object":{"x":-0.627,"y":0.649,"z":0.0,"size":1.611,"type":"move","id":3}}
{"timestamp":1644302996116,"object":{"x":-0.599,"y":0.146,"z":0.0,"size":0.699,"type":"move","id":3}}
{"timestamp":1644302996519,"object":{"x":-0.597,"y":0.667,"z":0.0,"size":1.651,"type":"move","id":3}}
{"timestamp":1644302996587,"object":{"x":-0.585,"y":0.132,"z":0.0,"size":0.671,"type":"move","id":3}}
{"timestamp":1644302996621,"object":{"x":-0.582,"y":0.199,"z":0.0,"size":0.781,"type":"leave","id":3}}
{"timestamp":1644302997591,"action":"stop"}
```

The timestamp is in milliseconds since January 1, 1970. To convert it in a readable time format, use the Unix date command and remove the last three digits of the timestamp. This is the timestamp in seconds:

```console
> date --date='@1644302992'
Di  8 Feb 2022 07:49:52 CET
```

### Heatmap Observer: @type=heatmap

| Type    | Parameter      | Description                                  |
|:------- |:-------------- |:-------------------------------------------- |
| heatmap | @file=fileName | file name or template to write image data to |

See type=file section for File name placeholders.

### Flowmap Observer:  @type=flowmap

| Type    | Parameter      | Description                                  |
| ------- | -------------- | -------------------------------------------- |
| flowmap | @file=fileName | file name or template to write image data to |

See type=file section for File name placeholders.

### Bash Script Observer: @type=bash

| Type | Parameter                  | Description                                       |
|:---- |:-------------------------- |:------------------------------------------------- |
| bash | @script=scriptFile.sh      | Script to execute on event                        |
|      | @count=true                | Execute Script on number of objects change        |
|      | @switch=true               | Execute Script on existence of object change      |
|      | @scriptParameter="p0 p1.." | Additional application specific static parameters |

Executes a bash script when the person count or the occupation changes.

### OSC Observer: @type=osc

| Type | Parameter                    | Description                                               |
|:---- |:---------------------------- |:--------------------------------------------------------- |
| osc  | @url=[osc[.tcp]://]host:port | url to send OSC messages to                               |
|      | @version=versionstring       | versionstring is used as a prefix for the address pattern |

Examples:

```
@url=localhost:5000 # OSC over UDP send to host localhost port 5000
@url=osc://localhost:5000 # OSC over UDP send to host localhost port 5000
@url=osc.tcp://localhost:5000 # OSC over TCP send to host localhost port 5000

@version=v2 # /v2 prefixes the address pattern (e.g. /frame xxx  changes to /v2/frame xxx)
```

### UDP Observer: @type=udp

| Type | Parameter      | Description             |
|:---- |:-------------- |:----------------------- |
| osc  | @url=host:port | url to send messages to |

Examples:

```
@url=localhost:5000 # send '\0‘ terminated text message to host localhost port 5000
```

### Websocket Observer: @type=websocket

| Type      | Parameter        | Description                                                 |
|:--------- |:---------------- |:----------------------------------------------------------- |
| websocket | @port=portNumber | port on which the websocket listens for connection requests |

Examples:

```
@port=5000 # listens for connections at port 5000
```

### MQTT Observer: @type=mqtt

| Type | Parameter                           | Description                |
|:---- |:----------------------------------- |:-------------------------- |
| mqtt | @url=[user[:topic]@]hostname[:port] | url of the MQTT connection |

The url has the format: `[user[:topic]@]hostname[:port]` 

For thingsboard, `user` is the device access token

The `topic` defaults to *v1/devices/me/telemetry*, the `port` to MQTT default port *1883*.

Examples:

```console
# Send thingsboard telemetry to host thingsboard.domain.com, device with access token 7GTdpHQjauzQ9RhQdCXh, use non standard port 9876 and topic "my/telemetry"
@url=7GTdpHQjauzQ9RhQdCXh:my/telemetry@thingsboard.domain.com:9876

# Send thingsboard telemetry to host thingsboard.domain.com, device with access token 7GTdpHQjauzQ9RhQdCXh, use standard port 1883 and default topic "v1/devices/me/telemetry"
@url=7GTdpHQjauzQ9RhQdCXh@thingsboard.domain.com

# Send MTQQ message with topic "my/telemetry", use standard port 1883 and default topic "v1/devices/me/telemetry"
@url=my/telemetry@thingsboard.domain.com
```

### Lua Observer: @type=lua

| Type   | Parameter       | Description     |
|:------ |:--------------- |:--------------- |
| script | lua script file | the script file |

Inside the Lua script, function `init()` is called on script loading.

```
function init()
    print( "init lua" )
    lastcount = -1
end
```

Functions `start( timestamp )` and `stop( timestamp )` are called on observer start and stop events.

```
function start( timestamp )
    print( obsv.timestamp('%c',timestamp) .. ": " .. tostring(timestamp) .. " start lua observer" )
end

function stop( timestamp )
    print( obsv.timestamp('%c',timestamp) .. ": " .. tostring(timestamp) .. " stop lua observer" )
end
```

Function  `observe( timestamp )` is called on each evaluation of the observer.

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

See [lua](../lua/README.md) for details.

## Filter

Define a filter for the layout and data fields of a message. The filter definition starts with a `:` followed by the filter tags to be reported. If a filter tag is set, the value of the tag referring to the parameter is reported with the given name. If for example *size* is set, then the value of *size* is reported with the parameter name `size`.

Example:

```
# report the (x,y) position and size of a detected object 
:frame,frame_id,object,id,x,y,size
```

In JSON format a message looks similar to:

```
{"frame_id":158,"object":{"id":"1","x":0.523,"y":-1.849,"size":0.72}}
```

### Filter Tag Aliasing

Tags can be aliased if that fits the receiving entity better: 

```
# report the (x,y) position and size of a detected object
:frame,frame_id=fid,object=blob,id=bid,x,y,size=extent
```

Results in:

```
{"fid":158,"blob":{"bid":"1","x":0.523,"y":-1.849,"extent":0.72}}
```

Or in OSC messages like:

```console
> oscdump osc.udp://:5000
```

For dropping the tag names in OSC, assign empty aliases:

```
:frame,frame_id=fid,object=blob,id=,x=,y=,size=
```

Results in:

```console
> oscdump osc.udp://:5000
e8ef7560.7724d8fc /frame sh "fid" 2188
e8ef7560.77317efd /blob shsisfsfsf "fid" 2188 "bid" 1 "x" -1.225907 "y" -0.134654 "extent" 0.416994
e8ef7560.7740f669 /blob shsisfsfsf "fid" 2188 "bid" 3 "x" -0.276697 "y" -0.885780 "extent" 0.512485
e8ef7560.7fa95c84 /frame sh "fid" 2189
e8ef7560.7fb7d848 /blob shsisfsfsf "fid" 2189 "bid" 1 "x" -1.226217 "y" -0.133945 "extent" 0.414815
e8ef7560.7fc610f0 /blob shsisfsfsf "fid" 2189 "bid" 3 "x" -0.277041 "y" -0.886793 "extent" 0.511134
```

### Filter Tag Order

The order of filter tags is irrelevant. The layout of the message is defined by the inherent logic of the data fields and is hardwired in the code.

These filter definitions will have exactly the same result:

```
:size=frame,x=,frame_id=fid,id=,y=,object=blob
:frame,frame_id=fid,object=blob,id=,x=,y=,size=
```

## Filter Tag Semantics

| Tag                | Description                                                                                      |
|:------------------ |:------------------------------------------------------------------------------------------------ |
| frame              | in osc, a single message with *frame* as address pattern indicating the next frame               |
| frame_end          | in osc, a single message with *frame_end* as address pattern indicating the end of the frame     |
| frame_id           | report *frame_id*, a sequential number counting up with each tracking frame, wraps at 16383 to 0 |
| timestamp          | the timestamp of the message in millisecond unix time (see unix `date` command)                  |
| num_objects        | report the number of objects detected                                                            |
| object             | indicates that object parameters should be reported                                              |
| objects            | indicates that objects should aggregated/grouped                                                 |
| id                 | the object tracking id                                                                           |
| position           | the object position aggregated                                                                   |
| x,y,z              | the object position as separate parameters                                                       |
| size               | the object size                                                                                  |
| type               | report the event type, can be *enter*, *leave*, *move*                                           |
| enter, leave, move | report if the the event matches                                                                  |
| lifespan           | report lifespan when object leaves                                                               |
| count              | number of objects detected                                                                       |
| switch             | 0 if no object detected, 1 otherwise                                                             |
| region             | report the region in which objects are detected                                                  |
| regions            | indicates that regions should aggregated/grouped                                                 |
| alive              | emits 1 every `aliveTimeout` sec                                                                 |
| operational        | emits the fraction of operational devices and `requestedDevices` every `aliveTimeout` sec        |

## Explore Filter Tags

You can check the result of your filter definition in a web browser with the ``websocketClientExample.html`` in this repository.

Start tracking with:

```
+track +observer @type=websocket :frame,frame_id,objects,type,enter,move,leave,x,y,z,size,id,lifespan,count @{name=WS,port=5000}
```

Then open the URL [http://localhost:8080/websocketClientExample.html](http://localhost:8080/websocketClientExample.html) in a web browser. Press *Reload* if you restart the lidar tool.

If you want to explore behaviour with regions, define a region *Region1* and start tracking with:

```
+track +observer @type=websocket :frame,frame_id,objects,type,enter,move,leave,regions,region,x,y,z,size,id,lifespan,count @{name=WS,port=5000,regions='Region1'}
```

Now play with filter tags in the browser.
