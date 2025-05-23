# Polling

Functions, methods, and processes are polled by writing a pin high at the beginning, and writing it low at the end. The pin is watched and measured with an oscilloscope (10-year old £100 picoscope coming in clutch). The measurement overhead is really easy as it's just the period without any measurement.

```c++
loop(){
  digitalWrite(outPin, HIGH);
  // operation to measure goes here
  digitalWrite(outPin, LOW);
}
```

## ESP32S3

The measurement offset is about 630 nS.

### touch

* reading raw touch data: 2.226 uS
* deviceTime.getUTCTimestampMicros(): 1.90 uS
* deviceTime->getUTCTimestampMicros(): 1.90 uS

### first installation

First installation for the curtain lights, has no event manager and no modes. It polls the touch peripheral, and drives a complimentary square wave. the entire loop below, takes 13uS when idle and a maximum of 28uS at the moment the lights are turned on (sets up a new interpolation).

The first loop takes 1.8mS, which is when constant brightness mode is actually constructed and initiated. I was hoping for a 50mS RTOS interval, so it's looking very promising!

```c++
while(true){
  digitalWrite(pollPin, HIGH);
  touchSwitch.update();
  modalLights->updateLights();
  digitalWrite(pollPin, LOW);

  delay(20);
}
```

The ram and flash usage at build time is:
```
RAM:   [=         ]   5.6% (used 18356 bytes from 327680 bytes)
Flash: [=         ]   7.9% (used 262601 bytes from 3342336 bytes)
```