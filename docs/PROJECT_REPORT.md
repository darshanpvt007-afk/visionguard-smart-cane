# VisionGuard: Smart Cane for Visually Impaired People (Obstacle and Wet-Surface Warning)

> **Status at Review 1:** the prototype is built (photos, Oct 2026). The firmware exists.
> The items marked **[to verify]** still have to be checked on the real hardware before
> anyone claims they work. Anything under *Future Enhancements* is **not** built.

---

## 1. Abstract

Visually impaired people usually rely on a white cane, which detects only what it physically
touches and gives no warning about water or wet mud on the path. VisionGuard is a low-cost
smart cane built on an Arduino Nano. An HC-SR04 ultrasonic sensor on the shaft measures the
distance to obstacles ahead, and a resistive moisture probe at the tip detects water or wet
ground. The Nano processes both readings in real time and warns the user with a buzzer. The
closer the obstacle, the faster it beeps, and if the tip touches water it gives a continuous
tone. The cane runs on an 18650 Li-ion cell with a rocker on/off switch and cost about ₹1,735
in parts, including tools.

## 2. Problem Statement

A standard white cane:
- detects obstacles only on contact, within about 1 m, so the user gets very little warning
- gives no warning before the user steps into puddles, open drains or wet, slippery mud
- gives no idea of *how far* away an obstacle is

Commercial electronic travel aids exist, but they are expensive for most users in India.
The need is a cheap, simple, battery-powered add-on that gives early warnings.

## 3. Objectives

1. Detect obstacles in front of the cane within about 50 cm using ultrasonic sensing.
2. Show how close the obstacle is through the beep rate (closer means faster).
3. Detect water or wet ground at the tip of the cane with a moisture probe.
4. Give wet-surface warnings priority over obstacle warnings.
5. Keep the design low-cost, lightweight and battery-powered, with an on/off switch.

## 4. Existing System and Its Limitations

| Existing system | Limitation |
|---|---|
| Plain white cane | Contact-only detection, no water detection, no distance information |
| Guide dog | Expensive, long training, not available to most people |
| Commercial smart canes | High cost, often imported, hard to repair locally |

## 5. Proposed System

A PVC-pipe cane fitted with:
- an **ultrasonic sensor** facing forward to measure distance
- a **moisture probe** at the bottom tip to sense water or wet mud
- an **Arduino Nano** that reads both sensors and decides which alert to give
- a **buzzer** for audio alerts (vibration output is reserved in the code but **not fitted yet**)
- an **18650 Li-ion cell + rocker switch** for portable power

## 6. Hardware Components

| # | Component | Function in this project | Confirmed from photos? |
|---|---|---|---|
| 1 | Arduino Nano (ATmega328P) | Main controller: reads sensors, runs the logic, drives the buzzer | Yes |
| 2 | HC-SR04 ultrasonic sensor (newer 4-pin version marked Trig/RX/SCL, Echo/TX/SDA) | Measures distance to obstacles using sound echoes | Yes |
| 3 | Soil-moisture probe (fork type, marked 土壤湿度检测, "soil moisture detection") | Two exposed copper tracks at the cane tip. Water between them lowers the resistance | Yes |
| 4 | LM393 comparator module (A0, D0, GND, VCC, potentiometer) | Converts the probe's resistance into a voltage. Only the **A0 analog output** is wired | Yes |
| 5 | Buzzer (black, round, 2-pin) | Audio alert. The code assumes an **active** buzzer | Yes; type **[to verify]** |
| 6 | 18650 Li-ion cell, 3.7 V 1200 mAh (HONGLI) in a 1-cell holder | Power source | Yes |
| 7 | Rocker switch (I/O) | Turns power on and off | Yes |
| 8 | Electrolytic capacitor, 220 µF 50 V, soldered across the Nano's 5V and GND | Smooths the supply voltage during buzzer and sensor current spikes | Likely |
| 9 | PVC pipe, jumper wires, berg strips, hot glue, cable tie | Body and mounting | Yes |
| – | IRLZ44N MOSFET, 1 A fuse + holder, JST connector | On the purchase bill but **not visible** in the photos | **[to verify]** |
| – | Vibration motor | Mentioned in the code and the old README, but **not on the bill and not visible** | **Not fitted** |

## 7. Software and Tools

- Arduino IDE 2.x (C/C++ for Arduino)
- Board: *Arduino Nano*, Processor: *ATmega328P* (try *ATmega328P (Old Bootloader)* if upload fails)
- Clone Nano boards use the CH340 USB chip, so the CH340 driver may be needed on Windows
- No external libraries are needed
- Serial Monitor at 9600 baud for testing and calibration

## 8. Block Diagram

```
                   +----------------------+
 18650 cell ──► Rocker ──►  Power to Nano   |
 (3.7 V)        switch     (5V pin)        |
                   |                        |
 HC-SR04  ──Trig(D5)/Echo(D6)──►            |
 ultrasonic                 |  ARDUINO NANO |──D2──► Active buzzer
                            |  (ATmega328P) |
 Moisture probe ─► LM393 ─A0─►              |──D3──► (Vibration: reserved, not fitted)
 (cane tip)      module     +---------------+
```

## 9. Pin-Connection Table

Pins are taken from the firmware. Each one is **[to verify]** by tracing its wire or by the
Serial Monitor test in `REVIEW1_PREP.md`.

| Device pin | Nano pin | Wire colour seen in photos |
|---|---|---|
| HC-SR04 VCC | 5V | white (likely) |
| HC-SR04 Trig | D5 | orange (likely) |
| HC-SR04 Echo | D6 | yellow (likely) |
| HC-SR04 GND | GND | black (likely) |
| LM393 A0 | A0 | grey |
| LM393 D0 | not connected | – |
| LM393 VCC / GND | 5V / GND | red / brown |
| Probe ↔ LM393 | 2-pin | purple / grey |
| Buzzer + / – | D2 / GND | [to verify] |
| Battery + | via rocker switch → **5V pin** (confirmed) | red |

## 10. Working Principle

**Ultrasonic distance.** The Nano sends a 10 µs pulse to Trig. The sensor emits a 40 kHz sound
burst and holds Echo HIGH until the reflection returns. Sound travels about 0.034 cm/µs, and the
pulse goes out and back, so:

> distance (cm) = echo time (µs) × 0.034 ÷ 2

**Moisture detection.** The probe is two copper tracks. When dry, there is a very high
resistance between them, so A0 reads about 1023 (the maximum of the 10-bit ADC). Water conducts,
so the resistance drops and the reading falls. Below the threshold (500), the code treats the
surface as wet.

**Alert priority.**
1. Wet surface: continuous buzzer, because stepping into water or a drain is the bigger danger.
2. Obstacle closer than 50 cm: short 50 ms beeps. The gap between beeps runs from 50 ms when
   very close to 400 ms at 50 cm.
3. Otherwise: silent.

## 11. Algorithm

1. Start. Set up the pins and Serial (9600 baud).
2. Send a trigger pulse and measure the echo time (30 ms timeout).
3. Calculate the distance in cm.
4. Read the moisture value from A0.
5. Print the distance, moisture value and state to the Serial Monitor.
6. If moisture < 500, turn the buzzer ON continuously and go to step 9.
7. Else if 0 < distance < 50 cm, beep for 50 ms, then wait map(distance, 2–50 → 50–400 ms).
8. Else turn the buzzer OFF.
9. Wait 30 ms and repeat from step 2.

### Flowchart

```
        ┌───────┐
        │ START │
        └───┬───┘
    ┌───────▼────────┐
    │ Init pins,     │
    │ Serial 9600    │
    └───────┬────────┘
    ┌───────▼────────┐◄───────────────────┐
    │ Measure dist.  │                    │
    │ Read moisture  │                    │
    └───────┬────────┘                    │
     ┌──────▼──────┐  yes  ┌───────────┐  │
     │moisture<500?├──────►│Buzzer ON  ├──┤
     └──────┬──────┘       │continuous │  │
            │ no           └───────────┘  │
     ┌──────▼──────┐  yes  ┌───────────┐  │
     │0<dist<50cm? ├──────►│Beep, gap  ├──┤
     └──────┬──────┘       │∝ distance │  │
            │ no           └───────────┘  │
     ┌──────▼──────┐                      │
     │ Buzzer OFF  ├──── delay 30 ms ─────┘
     └─────────────┘
```

## 12. Applications

- Mobility aid for blind and low-vision people, indoors and outdoors
- Monsoon or rainy-season walking (puddles, waterlogged roads)
- Elderly people with poor eyesight
- A teaching demo of sensor interfacing on a microcontroller

## 13. Advantages

- Low cost (about ₹1,735 including tools; the electronics on the cane cost less)
- Warns about obstacles before contact and gives distance through the beep rate
- Water detection, which most basic smart canes do not have
- Simple, repairable, locally available parts; runs on a rechargeable battery

## 14. Limitations (be honest about these at the review)

- The ultrasonic sensor sees only a cone in front (about 15°). It can miss thin poles, overhanging
  branches at head height, and soft or angled surfaces that absorb or deflect sound.
- Detection range is set to 50 cm, which is a short warning distance at walking speed.
- The moisture probe detects water only when the tip **touches** it, not before.
- An exposed resistive probe corrodes over time because DC current flows through it in water.
- A buzzer alert can be hard to hear in noisy traffic, and it is not private.
- A single 3.7 V cell feeds the Nano's 5V pin directly (no regulator). This works in our test, but the Nano
  and the sensor are designed for 5 V, so as the cell drains below about 3.3 V the readings can become
  unreliable. There is no on-board charging, low-battery warning or over-discharge cut-off.
- USB and the battery must never be connected at the same time: USB 5 V would flow back into the
  cell through the 5V rail.
- Not tested yet with real users or outdoors.

## 15. Future Enhancements (not built)

- Vibration motor driven through the IRLZ44N MOSFET, for silent haptic feedback
- Second ultrasonic sensor angled up (head-level obstacles) or down (steps and potholes)
- 3.7 V → 5 V boost converter + TP4056 charging module + low-battery beep
- Capacitive moisture sensor (no exposed metal, so no corrosion)
- GPS + GSM/Bluetooth SOS message to family
- Waterproof enclosure; PCB instead of jumper wires

## 16. Conclusion

VisionGuard shows that an Arduino Nano, an ultrasonic sensor and a moisture probe can turn a
plain cane into a low-cost electronic travel aid that warns about obstacles before contact and
about water underfoot. Review 1 covers the working prototype and its design. Next steps are
verifying the power circuit, calibrating the thresholds, adding haptic feedback and field testing.

## 17. References

1. HC-SR04 Ultrasonic Ranging Module datasheet (Elecfreaks / Cytron).
2. ATmega328P datasheet, Microchip Technology.
3. Arduino Nano documentation, docs.arduino.cc.
4. Arduino language reference: `pulseIn()`, `analogRead()`, `map()`.
5. World Health Organization, *World Report on Vision*, 2019.
