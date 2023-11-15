# Device Name Management

Device name management simplifies the handling of device names. On the one hand, it allows physical devices to be associated with a persistent device name via serial number; on the other hand, nik names facilitate the readability of device names.

Nik names are key value pairs stored in a JSON file `conf/nikNames.json`.

## Nik Names

A nik name is an alias for a device. It makes dealing with devices more intuitive and on Linux it allows binding devices to names depending on their serial number (see section UDEV)

### Set Nik Name

Assign the nik name *myNikName* to device lidar0. *myNikName* can now be used as alias for lidar0:

```console
> ./lidarTool +setNikName myNikName lidar0
```

### Set Nik Name by Serial Number

Set the nik name *myNikName* to device lider*SERIALNUMBER*. *myNikName* can now be used as alias for the lidar with the serial number, independent of the serial port it is assigned to:

```console
> ./lidarTool +setNikName myNikName lidarC8B399F6C9E59AD5C5E59CF734613415
```

### List Nik Names

List all nik names currently defined:

```console
> ./lidarTool +listNikNames
nikNameFile=conf/nikNames.json

key=lidarC8B399F6C9E59AD5C5E59CF734613415
 name=myalias device=/dev/lidar0

key=myNikName
 name=lidar0 device=/dev/lidar0
```

### Remove Nik Name

Remove the nik name *myNikName* from the database of nik names:

```console
> ./lidarTool +removeNikName myNikName
```

### Rename Nik Name

Rename the nik name *oldName* to *newName* in the database of nik names:

```console
> ./lidarTool +renameNikName oldName newName
```

Renames also the corresponding devices in the group definitions.

### Nik Name for Virtual Devices

Nik names can also be used as shortcuts for virtual devices:

```console
> ./lidarTool +setNikName remoteSensor virtual:192.168.1.35:9090
> ./lidarTool +listNikName
nikNameFile=conf/nikNames.json

key=remoteSensor
 name=virtual:192.168.1.35:9090 device=virtual:192.168.1.35:9090
```

The name "remoteSensor" can now be used as a shortcut for the virtual device:

```console
> ./lidarTool +d remoteSensor .....
```

## Device Groups

----

## UDEV

Whenever the system detects a new device on the serial port, the UDEV subsystem assigns alternative device names to the device. An UDEV rule is used at system boot, or when a device is manually plugged in, to call a command that returns alternative names under which the kernel adds symlinks to the devices:

```console
> ./lidarTool +udev ttyUSB0
lidarCE8E99F6C9E59AD5C5E59CF7325C3415 lidar0
```

The command returns a generic device name with the serial number and an alternative device name to which the system generates symlinks:

```console
> ls -l /dev/lidar*
lrwxrwxrwx 1 root root 7 Mär 15 17:30 /dev/lidar0 -> ttyUSB0
lrwxrwxrwx 1 root root 7 Mär 15 17:30 /dev/lidarCE8E99F6C9E59AD5C5E59CF7325C3415 -> ttyUSB0
```

To give a device a persistent name, use:

```console
> ./lidarTool +setNikName lidar5 ttyUSB0
```

This sets the nik name lidar5 for the serial number of the device plugged in /dev/ttyUSB0. The command above would now return:

```console
> ./lidarTool +udev ttyUSB0
lidarCE8E99F6C9E59AD5C5E59CF7325C3415 lidar5
```

Unplugging and replugging the serial device would lead to this device assignment:

```console
> ls -l /dev/lidar*
lrwxrwxrwx 1 root root 7 Mär 15 17:30 /dev/lidar5 -> ttyUSB0
lrwxrwxrwx 1 root root 7 Mär 15 17:30 /dev/lidarCE8E99F6C9E59AD5C5E59CF7325C3415 -> ttyUSB0
```

The device with the serial number CE8E99F6C9E59AD5C5E59CF7325C3415 is always assigned to `/dev/lidar5`, independent of to which ttyUSB*X* the device is assigned to.

It is good practice use the device name liderN format, since the implementation allows shortcuts for these device names.

Note: The RPlidar and the YDLidar series support serial numbers while the LD06/LD19/STL27L and ORADAR MS200 do not.

The CP210x USB<->UART bridges used by all of the three manufacturers do not have distinct serial numbers in their delivered state. You can assign serial numbers with the [cp210x-cfg](https://github.com/DiUS/cp210x-cfg) tool and build your own UDEV rules.
