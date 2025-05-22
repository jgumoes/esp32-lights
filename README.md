# Smart Lights

## About

This is an esp32 project to create smart lights with different modes. The lights will function as alarms, help with time blindness, and probably some other cool stuff too! The end goal is to have smart lights with user-friendly phone/computer interface, but doesn't depend on an internet connection to function (they're smart enough to when to turn on with out having to be being told every single time).

There's a few motivations and a constraint driving this project. The **motivations** are:

- I want a sunrise alarm clock that's actually bright enough to mimic a sunrise. I have a dumb lamp that should be bright enough (*also the min brightness is too bright I want to reduce it*), but if it isn't, every single light in my room working at the exact same time should do it!
- I want my lights to dim themselves off late at night. That way, I can't lose track of time and stay up till 4am playing piano!
- I want a big RGB neopixel wall clock that turns itself off late at night when all other lights are off. If other lights are on, however, I want it to slowly flash red. I want it to start off dim, maybe like 0%-5% peak-to-peak, but slowly flash brighter until all other lights are turned off.
- When I set an alarm for all the lights, I don't want to change each light individually. Computers can do that, they love doing that stuff. When I'm on holiday for a week, I want to pause all morning alarms for that week and for the alarms to unpause themselves ready to go back to work. That should be one operation in the phone app. Otherwise, I'm going to remember to turn the alarms back on every single day except the night before.
- I have a few ideas for ADHD management tools, and this project will either: (1) be directly implemented in those tools with minor variations; (2) parts of this project can be seperated into libraries for those tools; or (3) this project will provide a learning experience and reference for creating those tools.
- I want a big impressive project to show what I'm capable of. If you're a prospective employer reading this, hopefully this showcase of my skills will demonstrate the value I can bring to your business and please hire me i hate being poor

The **constraint** is pretty frustrating. ISP WiFi routers tend to be underpowered. Sure, it can handle gigabyte through-put, but it's also a computer managing every device connected to it. When you live in shared accomodation, there are a lot of devices connected at the same time and our modem tends to get overwhelmed and kicks devices off the network. Even though I have no intention of exposing my smart light system to the outside world, and even though the only time it might access the internet is for time server apis, the mere act of connecting lots of smart devices to the WiFi will cause the same connection issues we've had in the past. This could probably be solved by buying a business router and using the ISP router just as a modem, but that requires money and I am poor.

I need a way of only connecting one device to the WiFi, and connecting every other device to that one device. I need a mesh network. I've looked around, and the best solution seems to be ESP-WIFI-MESH.

### Technical

***Development is ongoing!**
this README is more of a design document than a detailing of current functionality. As of writing, a slightly-less-than-barebones version is running in one of my lights, but an actually-barebones version is coming very soon. The main development branch is currently MVP*

Currently, this project is targeting the Arduino platform as it's quick and easy and I like tackling learning cliffs one at a time. I eventually want to use ESP-WIFI-MESH for networking, which will require learning and porting over to esp-idf. When I implement hardware features such as PWM or capacitive touch, I use the esp-idf docs so that I won't need to port them (also, the espressif api is a lot more customizable and the only way to do some stuff like complimentary square waves).

This project is unit tested! There are many tests! so many tests. Dependancy injection in c++ is not fun, but virtual classes don't seem to have any performance hit. Still, when everything is done, I'll try switching out the virtual classes and I'll be able to directly measure the performance hit (10 year old picoscope FTW).

This project is mostly OOP, as it makes development and testing so much easier. For classes that exist for the entire lifecycle of the program, there isn't any performance difference compared to not-OOP, and you get to use dependancy inversion to support a wide range of physical implementations. However, constructing and destructing classes during run-time can lead to heap allocation which leads to heap fragmentation and stack overflow. The light modes currently use the oop strategy pattern, but once they are all implemented and fully tested I'll figure something out with structs and function pointers.

AI did not write any of this codebase. In my experience, AI is ~~great~~ pretty good at c++ syntax, but Refactoring Guru's design pattern catalog is a way better source for problem solving and code design.

### Project Milestones

- **get the curtain lights working without events:** this light needs a +/- 5V square wave as the LEDs are wired alternating in parallel
- **overhaul my white-and-warm bedside lamp:** fabricate the circuitry, design and print a project, rebuild the whole thing from scratch. Implement EventManager so that it's white during the day but warm at night
- **rebuild the umbrella lamp:** higher powered LED lamp, and the encoder will either have to be buffered or read through an AtTiny85 via a long I2C cable
- **implement EEPROM storage:** (*with* wear leveling, of course). This is required to change config values and CRUD events and modes
- **basic WiFi connectivity:** mDNS API server and React Native app should be fine to start with. maybe have an internal switch to toggle connection mode.
- **RTOS:** WiFi functionality should be always running on one core, and the light interpolations, event management, physical inputs, etc. running on the other core. The device should always be ready for connections, but the connections can't block the core functionality.
- **device management server:** dockerised, running on my NAS, and database synchronisation with the React Native app
- **ESP-WIFI-MESH:** mesh network.
- **Make the UI nice:** at this point, the UI will probably be confusing garbage
- **integrate environmental sensors:** i.e. mmWave presence detector, light level detector.

## The Modes

The core of the project is the modes themselves! The modes can be active or background: background modes are a constant loop, and active modes function as alarms that interrupt the current background mode. Cancelling an active mode resumes the background mode from where it left off. Any mode can be called as active or background, depending on the event that triggers the mode. Down the road, I want to add networking with a phone app and maybe google home or something. When the user requests an arbitrary mode be set, the incoming mode will be active so that it can barge in front of whatever is currently happening and doesn't affect the background loop.

If the mode is background, pressing the off button will toggle the state of the lights. If the mode is active, the button will cancel the mode, but some modes will block cancelling during their time window.

The modes are intended to be reusable, so that the same modes with the same behaviours can be called at different times of the day. They are also intended to be unique system-wide, so that multiple devices with different LED colours can call the same events with the same modes. This should (hopefully) help the interface UX, for example multiple devices can share an alarm that can be edited once for all of the devices, and the same modes can be called by different events at different times of day with the exact same behaviour. If your weekday alarm is for 6:45am, you don't want to have to change the same alarm on every single device every time there's a bank holiday!

The mode data seperates overall brightness from relative colour brightnesses. Setting the colours individually would make overall brightness an N-dimensional problem, and seperating them reduces it down to ~1D (i.e. just a brightness level knob). When setting the colours, at least one of the channels must be the max brightness of 255. There will still be a change in brightness during interpolation, as RGB[255, 255, 255] is obviously going to be brighter than RGB[255, 0, 0] and interpolating between them will see a change in power output (a non-linear change if overall brightness also changes). ModalLightsController will not normalise the colours to a constant power output because *I* don't want to limit brightness to 1/3 of the hardware maximum, but if *you* want to, the lights HAL class will be the place to do it.

### 1 Constant Brightness

Brightness does not change with time. Just a normal, dimmable light.

**Active Behaviour:**
Just forces itself on. Pressing the off button instead cancels the mode and loads the background mode. Brightness can't be set below the hardware minimum. The minimumActive brightness will be added to the hardwareMinimum.

**Background Behaviour:**
If a minimum brightness is given but the mode is triggered as background, the minimum brightness is ignored.

### 2 Sunrise Mode

The minimum brightness increases at a constant rate. The brightness can be increased above the minimum and won't change until the minimum catches up, but the brightness can't be dimmed below the minimum. If the brightness is somehow below the minimum (i.e. the device turns on mid-mode), the brightness should increase rapidly (i.e. r = 255/[10 seconds]) to catch up with the min.

With it being a sunrise, the end device brightness should be 100%, but because the light channels are split, some channels may not be 100% (i.e. for RGB lights you may want a blueer light with less red and green) so no checking is done on the device. The interface app should have a colour picker that determines the ratio of the channels, and always sets the most dominant channel to 255.

// TODO: block switching off until window ends
// TODO: do I want the dimmable limits to be set by the mode or by the event?
// TODO: should the rate be adjusted to a minimum window of 5 minutes?

**Active Behaviour:**

Blocks cancelling until the window is finished. Can't be turned off or dimmed below min line. If a active new mode is loaded mid-window, it'll suggest a quick change to its maximum settings.

**Background Behaviour:**

Can be turned off or dimmed during window, but doing so resets the interpolation until the window is finished. After it's finished, it can be turned off. This is a "waking up" mode; there will be no escaping the waking up. If a new mode is loaded mid-window, it'll suggest the current values to the next mode.

### 3 Sunset Mode

The set brightness decays at a constant rate until it reaches a target brightness. The target can be set to any value including 0, so that this mode can be called several times over the evening and night to create a gradual dim and colour change over the night. If the light is dimmed to 0 and switched back on, the on brightness will be the target brightness.

When triggered, it will take the initial colour channel brightnesses and rapidly (i.e. over 10 seconds) change the colour channels to the ratio set in the mode data packet.

**Active Behaviour:**

Brightness can't be increased above the target slope. // TODO: this should be bound by hardware minimum brightness
// TODO: block cancelling for a period of time?
// TODO: block cancelling until ambient light level is below a threshold?

**Background Behaviour:**

### 4 Pulse

Flashes between a min and max brightness at a constant rate. Can be used to make cool colour patterns or as an alarm. Adjusting the brightness will shift the min and max brightnesses of the pulse.

// TODO: when cancelled, it should finish rising to it's max brightness before handing over to the background mode. Dimming the light should still work, but it will have to finish the colour change.
// TODO: should the final rise match the period, or happen quickly (i.e. 10 seconds)?

**Active Behaviour:**

Brightness cannot be adjusted.
// TODO: option to block cancelling until room is empty?

**Background Behaviour:**

Brightness can be adjusted. This mode could be combined with an ambient light sensor to flash late at night if other lights are on.

**Quick Change Behaviour:**

// TODO: do i want this? If the half period is less than the window, quick change should occur over the period.

### 5 Chirp

Pulse, but with increasing (or decreasing) frequency. This is meant as an alarm. It doesn't *have* to be an alarm, but it's not exactly a "chill vibes" mode.

**Active Behaviour:**

// TODO: option to block cancelling until room is empty?

**Background Behaviour:**

Max brightness can be adjusted.

### 6 Changing

The brightness and/or colours change over the time window. It's intended for longer-term changes, eg turning from green to red over the course of a week, but can be set to a minute or two. This is actually a sneaky addition for a different project idea: a chore chart that changes colour depending on deadline. Some chores are weekly or more, hence the long time window.

**Active Behaviour:**

**Background Behaviour:**

## Configs

The ConfigManager needs the Hardware Abstraction Layer to work, which will access a partitioned file system or EEPROM or something. However, setting and reading configs should be done via the concerned classes, i.e. get the modal lights configs by `ModalLightsInstance->getConfigs()`.

### Modal Lights

- **minOnBrightness:** Default = 1. The minimum meaningful On brightness. if the light is an indicator, this could be 1, but if the light is a source of illumination, 1 is probably too low to see anything.
- **softChangeWindow:** Default = 1. If brightness is set or lights are turned on, it will interpolate to the On brightness over softChangeWindow. If lights are adjusted (i.e. with an encoder dial), this value is ignored and changes are instant

## Important Defines

```c++
#define NUMBER_OF_LIGHTS_CHANNELS 1 // the number of light channels, i.e. RGB = 3, but a single led type = 1
// TODO: define NUMBER_OF_LIGHTS_CHANNELS in pre-compile script based off ACTIVE_CHANNELS
// #define ACTIVE_CHANNELS 0b00011100  // flag of the channels being used

#define DataPreloadChunkSize 5      // the number of events or modes to preload from storage. smaller number means more calls to FRAM/EEPROM/whatever, but reduces the risk of memory overflow
```

## Interface App / Network System Requirments

- the app needs to check if two active (or two background) modes have the same activation times, otherwise the mode switching will be indeterminate
- needs to have a force-cancel alarms option. updating a triggering alarm will not cancel it.

## Usage

### Events

#### Adding Events

events should be added to EventManager first as it will perform checks. If the event is good, then add it to DataStorageClass.

#### Removing and Updating Events

Events should be removed/updated from EventManager and DataStorageClass independantly. EventManager shouldn't need to access storage for removal and updates

### DeviceTime

DeviceTime primarily uses an onboard register to manage device time. The register uses UTC time in microseconds since 2000. The 2000 epoch is compatible with the BLE-SIG specified Device Time Service and 1970 isn't, and I started this project intending to use BLE (I probably won't, but might reuse DeviceTime in projects that do). This project doesn't need a resolution greater the milliseconds, but the esp32 timer peripheral can only do microseconds and the time buffer shouldn't overflow until we're all dead anyway.

The time should default to the BUILD_TIMESTAMP when the program starts. DeviceTime has a timeFault flag that's raised when time is under BUILD_TIMESTAMP, or it's just been a while since the last time sync.

DeviceTime implements the observer pattern through [Embedded Template Library](https://www.etlcpp.com/home.html). This allows any class dependant on knowing the time, to handle changes properly. i.e. EventManager needs to know about DST changes, ModalController needs to know UTC changes to keep the interpolations nice and smooth. Remember to add references to observer classes to the DeviceTime instance!

```c++
#define MAX_TIME_OBSERVERS 2  // the maximum number of time observers

/**
 * @brief passed to observers when time is set. newTime = oldTime + change
 * 
 */
struct TimeUpdateStruct{
  int64_t utcTimeChange_uS = 0;
  int64_t localTimeChange_uS = 0;
};

typedef etl::observer<const TimeUpdateStruct&> TimeObserver;  // observers need to publicly inherent TimeObserver
```

When setting DST or timezone values, a timestamp must also be provided. The reason is i won't be implementing storage for the MVP. This means the DST and timezone values will always default to 0 on boot. If the RTC chip holds the local timestamp (and it should), DeviceTime will have the correct times on boot, and setting the correct utc timestamp with the correct offsets will result in a small change in local time (max<=0.5 seconds).

### Data Storage

Data storage will ultimately happen in 3 locations: (1) on the phone app, which then syncs with (2) the homeserver as soon as possible. The home server will then sync the changes with (3) the relavent devices.

#### Device Storage

Ideally, this will be FRAM or EEPROM, but if there's space maybe a local filesystem for less involved lights (i.e. a UV lamp isn't going to be bright enough for most of the modes to be useful, so it probably won't get a lot of writes and changes).

#### Data Validation

Data can get corrupted, so it's important that the server can check that the information stored on the devices is still correct.

// TODO: since the storage headers can also be corrupted, maybe the server should store the expected locations as well?

#### Corrupt stored data

CRC will be used to validate individual mode packets recieved from network and after writing to storage, and maybe when loading from storage if it's not too slow. The device should alert the server, which should then request a data validation.
