# Lidar Config Samples

## Check out Samples

To create a new lidar configuration and initialize it with one of the samples use `editConfig.sh` with option `create` *path_to_sample/conf*.

### Example

To check out the osc sample, type:

```console
cd lidar/lidarconfig
./editConfig.sh create samples/osc
```

Change to the newly created configuration directory:

```console
cd ../lidarconfig-osc
./StartAdmin.sh +v
```

Open [http://localhost:8000](http://localhost:8080) in your web browser:

- Click *Start Server*.

- Click Tab *UI* to change to the *lidarTool* Web UI.

Each sample has a file ***init_config.sh***. Read file content for more information.

### Check out with a different name

To check out the osc sample and assign it a different name, type:

```console
./editConfig.sh create myosc samples/osc
```

Change to the newly created configuration directory:

```console
cd ../lidarconfig-myosc
```

## Files

- **observer.txt** contains the observer definitions. Change according to your needs.

- Create a file ****LidarSensors.txt**** if you want to use specific devices. 
  If you have a LD09 connected to serial port 0 and a RPLidar A1M8 connected to serial port 1, the content should be:
  
  ```console
  ld09:0
  a1m8:1
  ```
  