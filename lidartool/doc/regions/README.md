Regions
===========

Regions are rectangular areas in world coordinates which mask tracking data. Regions are created via command line interface and can be edited in the Web-GUI.

Each region can be associated with several observers and each observer can be associated with several regions. Thus a set of regions can be used by one observer to switch audio while another observer logs people's movements within these regions to a file. Regions mask the tracking data such that an object's motion is reported only if the object is in one of these regions. Entering and leaving is reported for each region separately; regions then should not overlap.

The region definitions are stored in a json file in `conf/regions.json`

# Regions Management

## Create a Region

Create a region and set the geometry. If the region already exists, the given parameter will overwrite the stored ones:

```console
./lidarTool +conf confName +setRegion regionName [@x=PosX] [@y=PosY] [@width=Width] [@height=Height]
```

#### Example

Create a region with a width of 10 meter and a height of 6 meter around the center of the coordinate system and elliptical shape:

```console
myRaspi> ./lidarTool +conf confName +setRegion "Switch Region" @x=0 @y=0 @width=10 @height=6 @shape=Ellipse
```

### List all regions

```console
myRaspi> ./lidarTool +conf confName +listRegions

regionFile=conf/regions.json

name="Switch Region"
 x=0
 y=0
 width=10
 height=6
```

### Region Parameters

All measurements are in meter

| Type            | Description                                             |
| --------------- | ------------------------------------------------------- |
| `name`          | name of the                                             |
| `x`             | position center x of the region                         |
| `y`             | position center y of the region                         |
| `w` \| `width`  | width of the region                                     |
| `h` \| `height` | height of the region                                    |
| `shape`         | geometric shape of the region: `Rectangle` or `Ellipse` |
| `tags`          | comma separated list of tag names                       |
| `layers`        | comma separated list of layer names                     |

When used in observers, regions can be referenced by name or tag, or the layer they belong to. In the Web UI, they are grouped by layer.

## Delete a Region

To delete the region *regionName*:

```console
./lidarTool +conf confName +removeRegion regionName
```

## Edit Regions via Web GUI

### Create Region

To create a region via Web GUI, select "Create Region" in the "*Regions*" submenu. Choose a region name which does not exist yet and confirm.

### Edit Region Geometry

To edit a region's **geometry** via Web GUI, select "Edit *regionName*" in the "*Regions*" submenu. The selected region will be editable via the grips for edges and corners of the region's rectangle. Select the **Shape** (either *Rectangle* or *Ellipse*) on the top right.

To finish editing press the "*Done Regions*" button on the bottom right to close the editor. 

![](images/RegionsEdit.jpg)

**Note:** This does not save the edited regions in the config file, but just closes the editor.

## Save Canges

To Save the changes in the config file, choose "*Save Regions*" from the "*Regions*" menu and confirm in the confirmation dialog.

The saved state will be loaded on the next startup.

## Reload Saved State

To undo changes, you can reload the saved state. Choose "*Reload Regions*" from the "*Regions*" menu and confirm in the confirmation dialog.
