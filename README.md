# visionguard-smart-cane
VisionGuard is an Arduino Nano-based smart assistive cane for visually impaired people. An HC-SR04 ultrasonic sensor detects obstacles within 50 cm, and the buzzer beeps faster as they get closer. A moisture probe at the tip detects water or wet mud and gives a continuous warning. Vibration feedback is planned but not fitted yet.

## Contents
- `firmware/visionguard/visionguard.ino`: current sketch (provisional; pins still to be verified on the hardware)
- `firmware/original/visionguard_original/`: original sketch, kept unchanged
- `docs/PROJECT_REPORT.md`: project documentation (abstract, block diagram, pin table, algorithm, limitations)
- `docs/REVIEW1_PREP.md`: Review 1 checklist, speech, demo plan and Q&A
