Scenarios
=========

Some example scenarios show how to write log files or report data to external applications. They can be used as starting point for your own solutions.

Observers are a mechanism to report tracking information to external applications. Each observer implements a specific communication channel, e.g. using websockets or OSC. To each observer, an arbitrary number of regions can be assigned which mask the tracking data such that object motion is reported by this observer only if the object is in one of these regions.

Please have a look at [Observers](../observers) and [Regions](../regions) first to get an understanding of what observers and regions are and how to define them.

General Setup
=============

Assume three sensors are connected to three regions named *Region1*, *Region2*, and *Region3* which are already defined with the right geometry and location.

Switch Audio via Bash Script Based on Region Occupancy
------------------------------------------------------

With the general setup above:

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer :switch,region @type=bash @name=SwitchAudio @script=SwitchAudio.sh @regions='Region1,Region2,Region3'
```

This executes the script *SwitchAudio.sh* each time the occupancy of a region changes. Environment variable `$switch` is `0` or `1` depending on if at least one person is in the region. Environment variable `$region` holds the name of the region.

Content of `SwitchAudio.sh` in the installation folder, please replace the `echo` by some real commands:

```shell
#!/bin/bash

echo switch=$switch region name=\"$region\"
```

Count People via Bash Script
----------------------------

With the general setup above:

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer :count,region @type=bash @name=CountPeople @script=CountPeople.sh @regions='Region1,Region2,Region3'
```

This executes the script *CountPeople.sh* each time the number of people inside a region changes. Script argument `$1` is the people count in the region. Script argument `$2` is the name of the region.

Content of `CountPeople.sh` in the installation folder, please replace the `echo` by some real commands:

```shell
#!/bin/bash

echo count=$count region name=\"$region\"
```

Publish the Number of People in Regions via MQTT
------------------------------------------------

With the general setup above:

Setup a device in e.g. Thingsboard (not described here). Lets assume it has the access token 7GTdpHQjauzQ9RhQdCXh and ThingsBoard is listening at standard port 1883. The telemetry should be writen in `numberOfPeople`. Thingsboard runs on the server `thingsboard.mydomain.com`:

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer @type=mqtt :count,region @name=peopleCountMQTT @url=7GTdpHQjauzQ9RhQdCXh:numberOfPeople@thingsboard.mydomain.com @regions='Region1,Region2,Region3'
```

Publish How Long People Stay in Regions via MQTT
------------------------------------------------

With the Thingsboard setup described above:

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer @type=mqtt :timestamp=ts,leave,lifespan,region @name=peopleStayMQTT @url=7GTdpHQjauzQ9RhQdCXh:numberOfPeople@thingsboard.mydomain.com @regions='Region1,Region2,Region3'
```

Log People Locations to File
----------------------------

With the general setup above:

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer @type=file :timestamp=ts,action,start,stop,object=person,enter,move,leave,id,x,y,z,count,lifespan,region @name=log
@file=/var/log/lidar/lidar_%daily.log
@regions='Region1,Region2,Region3'
@logDistance=0.75
```

This creates a daily log file in the folder `/var/log/lidar/` which contains single line tracking information in JSON format.
Peoples' locations are logged when they enter or leave or if they move further then 0.75 meter (default=0.5)

Send People Locations via OSC
-----------------------------

Assume three sensors are connected and one region named *Region1* is already defined with the right geometry and location.

Send tracking data with peoples tracking id, position, and size to localhost port 5555 as OSC UDP messages:

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer @type=osc :frame,frame_id,frame_end,object,move,id,x,y,z,size @name=oscObsv @url=osc.udp://localhost:5000 @regions='Region1'
```

To listen and dump the OSC messages, use the command `oscdump` delivered with liblo:

```console
> oscdump osc.udp://:5000
```

Publish Peoples Locations via WebSocket
---------------------------------------

Assume three sensors are connected and one region named *Region1* is already defined with the right geometry and location.

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer @type=websocket :frame,frame_id,objects,type,enter,move,leave,regions,region,x,y,z,size,id,lifespan,count @name=WS @port=5000 @regions='Region1'
```

You can check the result with the `websocketClientExample.html` in this repository. In a web browser, open the URL [http://localhost:8080/websocketClientExample.html](http://localhost:8080/websocketClientExample.html) and play with the filters.

Press *Reload* if you restart the lidar tool.

Multiple Observers
------------------

### Switch Audio via Script AND Log People locations to File

This combines two of the examples above. You can define an arbitrary number of observers doing different things as long as the shared resources used, such as TCP ports to listen or file names, do not interfere: 

```console
> ./lidarTool +d 0 +d 1 +d 2 +track +observer :switch,region @type=bash @name=SwitchAudio @script=SwitchAudio.sh @regions='Region1,Region2,Region3' +observer @type=file :timestamp=ts,action,start,stop,object=person,enter,move,leave,id,x,y,z,count,lifespan,region @name=log @file=/var/log/lidar/lidar_%daily.log @regions='Region1,Region2,Region3' @logDistance=0.75
```
