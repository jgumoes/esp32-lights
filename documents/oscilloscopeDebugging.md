# Oscilloscope Debugging

Oscilloscopes can be create for debugging embedded software! This document is will show some ways that I used an oscilloscope to fix errors, and therby prove I'm a great problem solver who will definitely add value to your company please hire me i need a job.

## Complimentary Square Wave

One of my lights is a string of LEDs wired in parallel with alternating polarity. The manufacturer used this to create various flashing modes and I hated all of them. To get a uniform brightness, you need two PWMs 180&deg; out of phase with a max duty of 50%. I did this by setting the resolution to 9 bits (this project has resolution of 8 bits, which is already 50% of 9 bits), setting both PWMs to come on at the start of the count cycle, and inverting the B output. The duty cycle for the A pin is `duty` and the B pin is then `MAX_DUTY - duty`.

```c++
// B channel config
ledc_channel_config_t channel_config_1 = {
  /* other config values */
  .hpoint = 0,                      // turn "on" at the start of the count cycle
  .flags{
    .output_invert = 1              // but flipped so "on" is low and "off" is high
  }
};

// setting the brightness
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, maxValue_9bits - newDuty);
```

And this worked beautifully! I went to bed happy that I'd reached the first major milestone, and turned off the lights. Except, these lights were still on. Just a tiny bit, too dim to tell when literally any other light is on, but on nonetheless.

![oscilloscope trace of A and B outputs](./images/look%20at%20this%20naughty%20little%20bump.png)

Right. So when the lights are 0, the B channel is high for the final (511<sup>th</sup>) bit. I know this now.
The ledc channel has a high point (when the output goes high), a low point (when the output goes low), and a duty cycle, but the docs and technical reference manual are pretty shy about how they interplay within the API. Luckily, a comment in the `driver/ledc.h` header made it clear that setting the duty cycle sets the low point with respect to the high point, without changing the high point. So, set the B channel high point to half way, and everything else same as the A channel, and now there are two identical, perfectly out-of-phase square waves. Perfect, dimmable, complimentary PWMs.

The ringing is interesting, though. The initial direction in the A channel opposes the edge of the B channel, but then both channels seam to match in phase and amplitude. The current from the rising edge sucks pixies away from the off channel (and vice versa), which makes sense as transister gates are capacitive, but I guess the level shifting causes instability to the supply of the entire peripheral.

```c++
// B channel config
ledc_channel_config_t channel_config_1 = {
  /* other config values */
  .hpoint = UINT8_MAX+1,          // turn on halfway through PWM
  .flags{
    .output_invert = 0            // "on" actually is on
  }
};

// setting the brightness
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, newDuty);
```
