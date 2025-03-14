# Modal Lights

An esp32 project to create smart lights with different modes. The lights will function as alarms, help with timeblindness, and probably some cool stuff too! The end goal is to have smart lights with user-friendly phone/computer interface, but doesn't depend on an internet connection to function (they're smart enough to when to turn on with out having to be being told every single time).

## Important Defines

```c++
#define NUMBER_OF_LIGHTS_CHANNELS 1 // the number of light channels, i.e. RGB = 3, but a single led type = 1
#define DataPreloadChunkSize 5      // the number of events or modes to preload from storage. smaller number means more calls to FRAM/EEPROM/whatever, but reduces the risk of memory overflow
```

## Interface App Requirments

* the app needs to check if two active (or two background) modes have the same activation times, otherwise the mode switching will be indeterminate
* needs to have a force-cancel alarms option. updating a triggering alarm will not cancel it.

## Usage

### Events

#### Adding Events

events should be added to EventManager first as it will perform checks. If the event is good, then add it to DataStorageClass.

#### Removing and Updating Events

Events should be removed/updated from EventManager and DataStorageClass independantly. EventManager shouldn't need to access storage for removal and updates
