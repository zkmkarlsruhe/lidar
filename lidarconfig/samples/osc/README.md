# OSC

Open device 0, track objects, report messages via OSC to localhost on port 5555:

```console
> ./lidarTool +d 0 +track +observer @type=osc :frame,frame_id,frame_end,object,move,id,x,y,z,size @url=osc.udp://localhost:5000
```

To listen and dump the OSC messages, use the command `oscdump` delivered with liblo:

```console
> oscdump osc.udp://:5000
```
