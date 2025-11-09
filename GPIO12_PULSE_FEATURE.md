# GPIO 12 PWM Change Pulse Feature

**Date**: 2025-11-09
**Purpose**: Add debug/test pulse output on GPIO 12 to observe PWM parameter changes and detect glitches

## Overview

Added a pulse output feature on GPIO 12 that generates a 10µs HIGH pulse whenever the UART1 PWM frequency or duty cycle is changed. This allows you to use an oscilloscope to correlate PWM parameter changes with any glitches or discontinuities on the PWM output (GPIO 17).

## Implementation Details

### Hardware
- **Pin**: GPIO 12
- **Function**: Debug pulse output (10µs HIGH pulse)
- **Trigger**: PWM frequency or duty cycle change

### Software Changes

#### 1. PeripheralPins.h
Added GPIO 12 definition:
```cpp
#define PIN_PWM_CHANGE_PULSE        12  // GPIO Output - Pulse on PWM parameter change (for glitch observation)
```

#### 2. UART1Mux.h
Added two private methods:
```cpp
void initPWMChangePulse();    // Initialize GPIO 12 for pulse output
void outputPWMChangePulse();  // Output pulse on GPIO 12 (for glitch observation)
```

#### 3. UART1Mux.cpp

**Constructor**:
- Added `initPWMChangePulse()` call to initialize GPIO 12

**initPWMChangePulse()**:
- Configures GPIO 12 as output
- Sets initial state to LOW
- Disables pull-up/pull-down resistors

**outputPWMChangePulse()**:
- Generates 10µs HIGH pulse on GPIO 12
- Called BEFORE PWM parameter changes

**setPWMFrequency()**:
- Added `outputPWMChangePulse()` call before `mcpwm_set_frequency()`

**setPWMDuty()**:
- Added `outputPWMChangePulse()` call before `mcpwm_set_duty()`

## Usage

### Oscilloscope Setup
1. **Channel 1**: Connect to GPIO 17 (PWM output)
2. **Channel 2**: Connect to GPIO 12 (pulse trigger)
3. **Trigger**: Set trigger on Channel 2 (GPIO 12) rising edge
4. **Time Base**: Set to 10-100µs/div to capture the pulse and PWM changes

### Testing Procedure

1. **Set initial PWM parameters**:
   ```
   UART1 MODE PWM
   SET PWM_FREQ 1000
   SET PWM_DUTY 50
   ```

2. **Change frequency and observe**:
   ```
   SET PWM_FREQ 10000
   ```
   - GPIO 12 pulse should appear on oscilloscope
   - Observe GPIO 17 for any glitches during frequency change

3. **Change duty cycle and observe**:
   ```
   SET PWM_DUTY 75
   ```
   - GPIO 12 pulse should appear on oscilloscope
   - Observe GPIO 17 for any glitches during duty change

### What to Look For

**Normal Behavior (MCPWM)**:
- Clean transition with minimal glitches
- PWM duty cycle percentage maintained during frequency change
- No unexpected pulse width variations

**Potential Issues**:
- Transient glitches on GPIO 17 during parameter changes
- PWM output momentarily stopping
- Unexpected duty cycle variations
- Phase discontinuities

## Technical Notes

### Pulse Timing
- **Pulse Width**: 10µs (fixed)
- **Timing**: Pulse occurs BEFORE the parameter change
- This allows the oscilloscope to trigger and capture the moment of change

### MCPWM Behavior
Unlike LEDC (which requires stopping and restarting), MCPWM can change parameters on-the-fly:
- `mcpwm_set_frequency()`: Changes frequency without stopping PWM
- `mcpwm_set_duty()`: Changes duty cycle without stopping PWM
- Duty cycle percentage is automatically maintained when frequency changes

### Comparison with Old Motor Control
The old motor control used separate GPIO pins (10, 11, 12) with LEDC PWM. GPIO 12 was used for pulse output. Now:
- GPIO 10, 11 are free (deprecated)
- GPIO 12 repurposed for debug pulse
- GPIO 17, 18 handle motor control via MCPWM

## Example Oscilloscope Captures

### Expected Waveform
```
GPIO 12 (Trigger):
    ____┐     ┐____
        │10µs│
        └─────┘

GPIO 17 (PWM):
Before: ┌─┐  ┌─┐  ┌─┐  ┌─┐
        │ │  │ │  │ │  │ │
        └─┘  └─┘  └─┘  └─┘  (1kHz, 50%)

After:  ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐
        ││ ││ ││ ││ ││ ││
        └┘ └┘ └┘ └┘ └┘ └┘  (10kHz, 50%)
```

## Related Files
- `src/PeripheralPins.h` - Pin definitions
- `src/UART1Mux.h` - Class declaration
- `src/UART1Mux.cpp` - Implementation

## Version
This feature is part of v3.0.0+ (motor control merge to UART1)
