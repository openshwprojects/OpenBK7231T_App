# Rotary Encoder Driver Documentation

## Overview

The rotary encoder driver (`drv_rotaryEncoder.c`) provides support for rotary encoders with optional integrated push buttons. It implements quadrature signal decoding to detect rotation direction and supports debouncing for stable operation.

## Features

- **Quadrature Decoding**: Accurately detects clockwise and counter-clockwise rotation
- **Debouncing**: 10ms debounce period to filter noise
- **Position Tracking**: Maintains an internal position counter
- **Button Support**: Optional integrated push button handling
- **Event Generation**: Fires events on rotation and button press/release
- **Channel Integration**: Updates channels with position and button state
- **Logging**: Debug logging for troubleshooting

## Hardware Setup

### Pin Configuration

A rotary encoder requires 2-3 GPIO pins:

1. **CLK (Clock) Pin**: Set role to `DigitalInput`
   - Connected to encoder clock output
   - Generate UP events on rotation

2. **DT (Data) Pin**: Set role to `DigitalInput`
   - Connected to encoder data/direction output
   - Determines rotation direction

3. **Button Pin** (Optional): Set role to `Button`
   - Connected to encoder push button
   - Active low (button pressed = LOW)

### Typical Wiring

```
| Encoder Pin | Microcontroller Pin             |
| ----------- | ------------------------------- |  |
| GND         | GND                             |
| +5V         | 5V or 3.3V                      |
| CLK         | GPIO (set as DigitalInput)      |
| DT          | GPIO (set as DigitalInput)      |
| SW          | GPIO (set as Button) [optional] |
```

All encoder pins should have pull-up resistors (enabled internally by the driver).

## Channel Configuration

### Required Channels

1. **CLK Pin Channel**: Associated with the CLK pin
   - Updated when rotation is detected
   - Read-only for normal operation

2. **DT Pin Channel**: Associated with the DT pin
   - Updated when rotation is detected
   - Read-only for normal operation

### Optional Channels

3. **Position Channel**: Secondary channel for CLK pin (`channels2`)
   - Stores the absolute position of the encoder
   - Can be set externally to reset position

4. **Button Channel**: Associated with button pin
   - Value: 1 = pressed, 0 = released
   - Only updated if button pin is configured

## Quadrature Decoding

The driver uses quadrature decoding to determine rotation direction. A typical rotary encoder produces signals in this sequence:

```
Clockwise rotation:
CLK: 0 -> 1 -> 1 -> 0 -> 0 -> 1...
DT:  0 -> 0 -> 1 -> 1 -> 0 -> 0...

Counter-clockwise rotation:
CLK: 0 -> 1 -> 1 -> 0 -> 0 -> 1...
DT:  0 -> 1 -> 0 -> 0 -> 1 -> 1...
```

The driver detects rotation by monitoring the CLK signal's rising edge:
- If DT follows CLK: Clockwise rotation (UP event)
- If DT opposes CLK: Counter-clockwise rotation (DOWN event)

## Events Generated

### Rotation Events

- **CMD_EVENT_CUSTOM_UP**: Fired on clockwise rotation
  - Argument: Current position value
  
- **CMD_EVENT_CUSTOM_DOWN**: Fired on counter-clockwise rotation
  - Argument: Current position value

### Button Events (if button configured)

- **CMD_EVENT_PIN_ONPRESS**: Fired when button is pressed
  - Triggered on falling edge (GPIO goes LOW)
  
- **CMD_EVENT_PIN_ONRELEASE**: Fired when button is released
  - Triggered on rising edge (GPIO goes HIGH)

## Usage Example

### Setup Pins

```
// Configure encoder pins
setPinRole 10 DigitalInput
setPinChannel 10 0
setPinRole 11 DigitalInput
setPinChannel 11 1

// Configure optional button
setPinRole 12 Button
setPinChannel 12 2

// Set secondary channel for position tracking
setPinChannel2 10 3
```

### Start Driver

```
startDriver RotaryEncoder
```

### Create Event Handlers

```
// Brightness control on rotation
addEventHandler ROTARY_BRIGHTNESS_UP "onCMD_EVENT_CUSTOM_UP" "setChannel 5 min($CH5 + 5, 100)"
addEventHandler ROTARY_BRIGHTNESS_DOWN "onCMD_EVENT_CUSTOM_DOWN" "setChannel 5 max($CH5 - 5, 0)"

// Button press action
addEventHandler ROTARY_BUTTON_PRESS "onCMD_EVENT_PIN_ONPRESS" "toggleChannelAll"
```

### Monitor Position

```
// Read current position
getChannel 3

// Set position to specific value
setChannel 3 0
```

## Debouncing

The driver implements a 10ms debounce period for both encoder and button signals. This:
- Filters out electrical noise
- Requires signal stability for 10ms before processing
- Prevents spurious rotation detection
- Ensures reliable button press detection

## Implementation Details

### State Machine

The driver maintains these state variables:
- `g_position`: Current encoder position (integer)
- `g_lastClkState`: Previous CLK pin state
- `g_lastDtState`: Previous DT pin state
- `g_lastButtonState`: Previous button state
- `g_debounceCounter`: Time accumulator for debouncing

### Frame Rate Operation

The `RotaryEncoder_RunFrame()` function is called every millisecond (approximately) by the main driver loop. It:
1. Reads current pin states
2. Detects changes with debouncing
3. Decodes quadrature signals
4. Updates position and fires events
5. Handles button input

### Logging

Log messages are generated at INFO and DEBUG levels:
- **INFO**: Initialization messages
- **WARNING**: Missing pin configuration
- **DEBUG**: Rotation and button press events (enable with debug logging)

## Troubleshooting

### Encoder Not Responding

1. Verify pin roles are set correctly:
   - CLK and DT must be `DigitalInput`
   - Button must be `Button` (if used)

2. Check channel assignments:
   - CLK and DT should have different channels
   - Position channel should be secondary channel of CLK

3. Verify pull-up resistors:
   - Check hardware pull-ups or GPIO configuration
   - Driver enables internal pull-ups automatically

4. Test pins independently:
   ```
   getChannel 0  // Read CLK state
   getChannel 1  // Read DT state
   ```

### Erratic Rotation Detection

1. Verify 10-15k pull-up resistors on encoder pins
2. Check for EMI/noise near encoder wires
3. Reduce wire length if possible
4. Ensure proper power supply decoupling

### Button Not Responding

1. Verify button pin is set to `Button` role
2. Check if button channel is associated correctly
3. Test button independently

## API Functions

### RotaryEncoder_Init()
Initializes the rotary encoder driver. Called once at startup.

### RotaryEncoder_RunFrame()
Main loop function. Called every frame (~1ms) to process encoder input.

### RotaryEncoder_OnChannelChanged(chIndex, oldValue, newValue)
Callback when a channel is modified externally. Used to reset position.

## Performance

- CPU Usage: Minimal (simple GPIO reads and state machine)
- Memory Usage: ~500 bytes for state variables
- Latency: 10-20ms (including debounce period)

## Compatibility

This driver works with:
- Standard rotary encoders (KY-040 compatible)
- Rotary encoders with integrated push buttons
- Both 3.3V and 5V systems (with appropriate pull-up resistors)

## Notes

- Position counter is not persistent across power cycles
- Debounce time (10ms) is fixed; adjust DEBOUNCE_TIME_MS define to change
- Each device can have only one rotary encoder (currently)
- For high-speed rotation, consider adjusting MIN_TIME_BETWEEN_EVENTS_MS

## Future Enhancements

Possible improvements:
- Support for multiple simultaneous encoders
- Configurable debounce time
- Acceleration for fast rotation
- Position limits/wrapping options
- EEPROM position persistence
