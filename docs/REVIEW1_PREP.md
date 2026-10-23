# Review 1 Prep: VisionGuard Smart Cane

## A. Tonight's checklist (in this order)

1. **Safety first.** Switch off. Make sure no bare wires or solder joints touch each other,
   especially the battery + and –. Li-ion cells can get very hot if shorted.
2. **Power test.** Switch on. Does the Nano's PWR LED light up? Does the HC-SR04 work?
3. **Function test with the Serial Monitor** (Nano on USB, battery switch OFF):
   - Upload `firmware/visionguard/visionguard.ino`, open the Serial Monitor at **9600**.
   - Move your hand toward the HC-SR04. If the distance changes, Trig/Echo on D5/D6 are correct.
   - The dry probe should read about 1000+. Dip the tip in a cup of water: the value should drop
     a lot and print `ALERT: WATER`. That confirms A0.
   - The buzzer beeps when your hand is under 50 cm. That confirms D2 and an active buzzer.
   - Note your real dry and wet numbers. Set `WET_SOIL_THRESHOLD` halfway between them.
4. **Battery test.** Unplug USB, switch on the battery. Does everything still work?
5. Read `PROJECT_REPORT.md` once fully, then practise the speech below out loud twice.
6. Charge the battery (with a proper Li-ion charger) and pack the items below.

**Carry tomorrow:** the cane, charged 18650 cell (plus the spare), USB mini-B cable, laptop
with the Arduino IDE and code, a small cup or bottle of water and a tissue, the printed report
and block diagram, and the bill (for cost questions).

## B. How to upload

1. Arduino IDE → Tools → Board → *Arduino Nano*; Processor → *ATmega328P*.
2. Tools → Port → the COM port that appears when you plug the Nano in.
3. Click Upload. If it fails with `not in sync`, change the Processor to *ATmega328P (Old Bootloader)*.
4. If there is no port at all, install the **CH340 driver** (clone Nanos use it).
5. Keep the battery switch **OFF** while USB is connected.

## C. 1–2 minute speech

> "Good morning. Our project is **VisionGuard, a smart cane for visually impaired people**.
>
> A normal white cane only detects an obstacle when it touches it, and it can't warn
> about water, puddles or open drains. Smart canes on the market are expensive.
>
> Our solution is a low-cost cane built on an **Arduino Nano**. An **HC-SR04 ultrasonic sensor**
> on the shaft measures the distance to anything in front by timing a sound echo. A
> **moisture probe** at the tip detects water: water conducts electricity, so the reading
> on the analog pin drops.
>
> The Nano checks both sensors continuously. If the tip touches water, the **buzzer sounds
> continuously**. That is the highest priority. If an obstacle is closer than **50 cm**, the
> buzzer **beeps, and beeps faster as the obstacle gets closer**, so the user can judge the
> distance. The cane runs on an **18650 lithium-ion cell** with an on/off switch.
>
> At this stage the prototype and code are complete. Next we plan to add vibration feedback,
> improve the power circuit and test it in real conditions."

## D. Demo plan (about 2 minutes)

1. Show the parts on the cane: sensor, Nano, buzzer, battery, switch, probe at the tip.
2. Switch on. Silence means the path is clear.
3. Slowly bring a notebook from 60 cm to 5 cm. The beeps get faster.
4. Dip the tip in the cup of water. Continuous tone, even with the notebook away.
5. Optional: show the live readings on the laptop's Serial Monitor.
6. **Backup plan if the battery fails:** power the cane from the laptop over USB.

## E. Code explained in four lines

- `readDistanceCm()`: sends a 10 µs trigger, measures the echo with `pulseIn`, then calculates distance = time × 0.034 / 2.
- `analogRead(A0)`: 0–1023. Dry is about 1023, wet is lower.
- `if moisture < 500` → water alarm; `else if distance < 50` → beep with gap = `map(distance, 2..50 → 50..400 ms)`; `else` → off.
- `delay(30)` between readings gives the old echo time to die out before the next ping.

## F. 24 likely questions with answers

**Basic**
1. *What does your project do?* It warns a blind user about obstacles within 50 cm (the
   beep rate shows distance) and about water at the cane tip (continuous tone).
2. *Why Arduino Nano?* It is small and fits on a cane, cheap (about ₹212), runs on 5 V, has
   enough I/O and a 10-bit ADC for the moisture probe, and programs easily over USB.
3. *How does the ultrasonic sensor work?* It sends 40 kHz sound and times the echo.
   Distance = time × speed of sound ÷ 2.
4. *Why divide by 2?* The sound travels to the object and back, so the measured time covers twice the distance.
5. *Where does 0.034 come from?* The speed of sound is about 340 m/s = 0.034 cm/µs.
6. *How does the moisture sensor work?* Two copper tracks. Water conducts and lowers the
   resistance between them. The LM393 module turns that into a voltage on A0.
7. *What is the blue board with the potentiometer?* An LM393 comparator module. The pot sets the
   threshold for the digital D0 output. We use the analog A0 output, so the threshold is set in code instead.
8. *What is the range of HC-SR04?* Specified 2 cm to 400 cm, accuracy about ±3 mm in ideal conditions.
   We use 50 cm as the alert distance.
9. *What is the battery?* An 18650 Li-ion cell, 3.7 V nominal, 1200 mAh.
10. *Total cost?* About ₹1,735 for all purchases, including the soldering iron and tools. The
    parts actually on the cane cost much less.

**Technical**
11. *Why does water get priority?* Stepping into water or an open drain can cause a fall, and the
    tip is already at that spot, so the danger is immediate.
12. *What does `map()` do?* It scales a number from one range to another, here distance 2–50 cm
    into a beep gap of 50–400 ms.
13. *What is `pulseIn()`?* It measures how long a pin stays HIGH, in microseconds. We give it a 30 ms
    timeout so a missing echo doesn't stall the loop.
14. *What is the ADC resolution?* 10-bit: 0–1023 for 0–5 V, so about 4.9 mV per step.
15. *Active vs passive buzzer?* An active buzzer has its own oscillator and beeps on plain DC (HIGH).
    A passive one needs a tone signal (`tone()`). Ours is driven with `digitalWrite`, so it is active.
16. *How long does the battery last?* Roughly: the Nano and sensor draw about 30–40 mA, so
    1200 mAh ÷ ~40 mA ≈ 25–30 h in theory. Real life will be less. We haven't measured it yet.
17. *Why the capacitor?* To smooth the supply so the buzzer switching doesn't cause voltage dips
    that reset the Nano.
18. *Why didn't you use the D0 output?* A0 gives the actual moisture level, so we can tune the
    threshold in software and see the values on the Serial Monitor.

**Challenging**
19. *Can it detect a staircase going down or a pothole?* Not reliably. The sensor faces forward.
    A downward-angled second sensor is a future enhancement.
20. *Head-level obstacles like branches?* Not covered. The sensor is on the shaft and its cone
    is about 15°. An upward-angled sensor is planned.
21. *Is 50 cm enough?* At walking speed (about 1 m/s) it gives about half a second, which is short.
    The value is a constant in the code and can go up to 100–150 cm after testing.
22. *What about false alarms?* Soft surfaces (cloth) absorb sound and angled surfaces deflect
    it, so they can be missed. Rain on the probe can trigger the water alarm. Averaging several
    readings and calibrating would help.
23. *The probe corrodes. How will you fix it?* Power the probe only while reading (from a digital
    pin), or switch to a capacitive sensor with no exposed metal.
24. *Can the Nano run on 3.7 V?* It is designed for 5 V. A single Li-ion cell (3.0–4.2 V) is
    marginal for 16 MHz and for the HC-SR04. The proper fix is a 5 V boost converter or two cells
    in series into VIN. *(Answer based on what you find in checklist step 4.)*

**Other questions to expect:** why PVC pipe (light, cheap, hollow for wires); why not an IR
sensor (sunlight interferes, shorter range, depends on colour); why not a Raspberry Pi
(cost, power and size; not needed for simple sensing); what the MOSFET and fuse on the bill are for (MOSFET
to drive a vibration motor later, fuse to protect the battery wiring); and what the novelty is (combining
obstacle *and* water detection at very low cost).

## G. Words to use carefully

- Say "prototype works in our bench tests", not "works reliably". Only say this after tonight's tests pass.
- Vibration feedback is **planned**, not working. Don't claim it.
- Don't claim GPS, Bluetooth or GSM. Those are future scope.
