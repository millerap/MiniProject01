My MiniProject is essentially an interrupt driven finite state machine. It takes an input gpio and an output gpio as commandline arguments. The input gpio will be a switch or button to trigger the interrupts and progress the finite state machine. The output gpio is an LED that turns on and off with every falling edge of the input gpio. The system runs a 100ms poll, which provides a clock rate for the fsm to update.

In state 0, the fsm reads an analog potentiometer (ain5). Based on the value across the potentiometer, a percentage is calculated by doing ain/4095 * 100. This percentage is used to set the duty_percent of a PWM output(ehrpwm.1:0), which is connected to an LED.

In state 1, the fsm prints the 12 bit value read from ain5. 

In state 2, the fsm does nothing.

In state 3, the fsm reads a TC74a I2C temperature sensor(i2c2_sda, i2c2_scl). The result is reported as both a hex value and a decimal value in degrees C.

In each of the states, there are two LEDS connected to gpio_30 and gpio_31 that show the state number in binary. I did this by doing a bitwise and with each of the bits and using that value to set the LED values.

