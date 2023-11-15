# Blueprints

If you want to use a **Floor Plan** in the background of the Web UI, you need a pixel image of the floor plan as a PNG image and need to know the extent of the floor plan in meters. A resolution of 50 pixel per meter - each pixel covers 2cm - is reasonable.

All floor plans must have the same pixel size.

### Floor Plan Extent

You have to measure the distance between two walls as accurate as possible in real space - e.g. with a laser range finder - and count the number of corresponding pixels in the floor plan. This defines the *blueprintExtent* parameter, which has the form ***PixelCount=Distance***. If for example 2744 pixels correspond to a distance of 58.23 meter, set the *blueprintExtent* to *2744=58,23* (the comma in 58.23 is respecting the floating point notation in the Linux German locale setting).

## Floor Plans

The *blueprintImageFile* is a floor plan image with the **background** set to **black**. A good choice for the image resolution is 30cm per pixel. 

### Simulation Mode

A different floor plan PNG image (*blueprintSimulationFile*) is used to simulate sensors during the planning phase. In the simulation, the simulated sensors cast rays from their center. If a ray hits a pixel with the red channel set to non zero, it is treated as an obstacle. Thus the **obstacles** - like walls - are painted in **red**, **yellow** or **white** colors. The **background** color must be **black**. 

### Masking Areas

An additional image defined by the *blueprintOcclusionFile* encodes functionality in color channels. This is a black image with the following colors:

- <span style="background-color: #ff5; padding: 0px; border: 1px solid black;">yellow</span>: Ignore objects at these locations. If certain areas create noisy LiDAR data, objects can pop up randomly. These can be ignored by setting these areas yellow.

- <span style="background-color: #55f; padding: 0px; border: 1px solid black;">blue</span>: If an object stays here for a longer time, it will be marked as *private*. Private objects can be filtered out in post-processing, for example museum personal who always sit at defined locations.

- <span style="background-color: #f55; padding: 0px; border: 1px solid black;">red</span>: Entrance or Exit Area. Refines the assignment of tracking IDs when people enter or exit the exhibition.

## Define Image Files

The floor plan images are defined with `editConfig.sh`. You can interactively browse the file system for images:

```console
./editConfig.sh
Extent of blueprint in Pixel=Meter (blueprintExtent=): 2744=58,23
do you want to update the blueprint image file (blueprintImageFile=) ? y
....
```

The names of the image files are stored in **blueprints.json** in the configuration subdirectory.

After choosing a *blueprintImageFile* and defining the *blueprintExtent*, the floor plan should be visible after restarting the Lidar Server. If you want to run in simulation mode, you also have to define a *blueprintSimulationFile*.

## Modify/Paint Image Files

After choosing an image with `editConfig.sh`, this image is copied to the configuration subdirectory. If you want to change - **modify/paint** - the image, **always modify/paint the newly copied image file** in the configuration subdirectory.

For example: If you have created a configuration named *myconf*  in `lidarconfig-myconf`, when you choose the image file `myBlueprintMap.png` from somewhere on system with `editConfig.sh`, it will be copied to `lidarconfig-myconf/myconf/myBlueprintMap.png`.  Modify/paint this image file, since the Lidar Server will read this file.
