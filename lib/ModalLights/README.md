
a library for handling the LED light modes.

## Intended functionality

There are a variety of brightness modes that the lights can have, and a couple of wave modes that decide what happens after the end brightness has been reached. Defaults to constant brightness until a mode is explicitly given.

```c++
// main.cpp
setup(){
  ...
  modalLightsSetup()
  ...
}

main(){
  ...
  updateLights(currentTimestamp_uS);
  ...
  delay(10ms);
}

// SoftwareAlarm.cpp
changeMode(){ // FIXME: what is the actual function?
  ...
  ModalLights::setLightsMode(params); // set the new mode at the correct time
}

// BrightnessDial.cpp   (not a real file. can be replaced with an on/off switch to same effect)
readDial(){
  ...
  ModalLights::updateBrightness(dialVal);
}
```

## requirements
  - the modal parameters need to be updated using a timestamp
  - needs to work for normal PWM and AC PWM (led lights should have a common interface)
  - how the parameters update should be interpolated, either:
    - from previous values to final values (handles manual change in brightness better)
    - from initial values to final values (easier and less taxing on MCU)
  - the curve should be settable (i.e. brightness should be either linear or logarithmic)
  - the update method should either:
    - get called from main every ~10ms  (more calculations overall)
    - return a delay until next required update (more calculation in method)
  - any updates to the LED brightness should update the previous parameters (should the LED class store these changes?)

maybe this library should be a list of interpolation functions that are fed LED class instances, and update the instances as appropriate.
deallocation creates memory holes and leaks, so the mode values should be passed by reference to an indexed mode function.
this library should actually be a part of the lights object. The lights object should exist for the entire runtime of the microcontroller, and 

## required modes:
  - on/off
  - sunrise/sunset
  - chirp (sinusoidal on/off with increasing frequency)

# Restructuring the modal controller

The modes are a bit of a confusing mess right now. I don't need a switch for the setup functions, and I don't need waves yet. 

the modes should be split into their own files. the functions should not be exported 
Each modal strategy needs the functions:
  -  (+) setXMode({init params}, {pointer to variables object}): sets the mode and binds functions to the pointer
  -  (+) updateBrightness() - find the new time-dependant brightness
  -  (+) setBrightness()  - set a new brightness with user input

the variables object can contain the default values

the variables can live inside each file for now, but the variables of the inactive modes are still taking up space in the ram. There should eventually be a common reserved area of memory that stores the variables struct.

tools common to all mode files:
  - structure of pointers to the mode functions
  - modalVariables: should either be a structure of variables or the manager of some reserved memory
  - a reserved memory for storing variables

  modalVariables as a structure is probably unfeasible, so this will probably use calloc to reserve a chunk memory and overwrite it with a new variable struct every time the mode switches.

## Behaviours

When the device turns on, it should start at the target brightness. The PWM should initiate at 0. The controller should be set with the correct strategy, and given the correct brightness.

When the strategy is changed, the new one should start with the same brightness.

The behaviour when the lights are turned off and on is strategy dependant. The behaviour of the switch is not dependant on the Modal Lights, but some other controller (alarms are going to get pretty complicated).

## Modes

### constBrightness

brightness doesn't change with time

- switched off and on: starts with previous brightness
- dimmed to 0 and switched on: starts with a min brightness (i.e. 5%)

- tbd: min or max brightnesses

### constChange

brightness changes at a constant rate.

- switched off and on: brightness 

### sunrise

brightness increases at a constant rate
- 

### sunset