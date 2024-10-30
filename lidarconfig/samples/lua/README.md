# Lua


In observer.txt two observers are defined, which are executing the scripts udp.lua and osc.lua.

Parameters given in the observer definition starting with @xxx are readable in the script via the table access obsv.param.xxx

To reduce jittering, the data is smoothed and the coordinates can be centered around the region center or normalized to [0..1] in the region bounds.

The execution can be dependend on the Run Mode, here it switches the verbosity.

## UDP

To dump the UDP messages, use the command `netcat`:

```console
> netcat -l -u -p 9090
```

## OSC

The script osc.lua uses the module losc, install it with:

```console
> luarocks install losc
```

To dump the OSC messages, use the command `oscdump` delivered with liblo:

```console
> oscdump osc.udp://:9091
```

