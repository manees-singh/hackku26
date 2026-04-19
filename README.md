## Inspiration
As college kids, we all struggle with being productive. With so many things moving around and so many distractions, it always feels hard to concentrate on one task. You know you sit down to study, tell yourself this time will be different, and twenty minutes later, you're deep in your phone watching reels, with no memory of how you got there. With AI models writing our code, we tell ourselves that we will take a break while AI does the job. That _just a few minutes_ turns into hours of mindless scrolling.

Online I learned about this technique, _Pomodoro_, but setting a timer on my phone only brought me closer to scrolling than staying far from it. I had an M5Go IoT kit so I decided — why not build my own Pomodoro timer that will be rude and snitch on me if I don't lock in.

## What it does
**WatchDog** is a physical desk companion that guards your focus. A creature lives on your **M5Go** display as you complete Pomodoro work sessions. While your phone stays on your desk, you are safe. The moment you pick it up during a focus block, WatchDog stares at you with a blaring alarm, forcing you to put the phone back.

The core feedback loop is simple:

- **Tilt** the M5Go to select your session length (25 min FOCUS, 45 min DEEP WORK, 5 min BREAK, 2 min RELAX)
- The session starts the moment you tilt it — no buttons needed
- Complete the session → your level increases, reflected by Pokéballs on screen
- Pick up your phone → WatchDog pauses your timer, shows an angry face, and beeps at you
- Your friend's level is always visible on the opposite corner — turning solo discipline into a shared competition

The **multiplayer layer** is what turns a productivity tool into a game. Two M5Gos communicate over **ESP-NOW**, a peer-to-peer protocol requiring no WiFi router, with latency under ~1 ms. You and a friend compete in real time — your WatchDogs can see each other's level, turning solo discipline into a shared ritual.

---

## How we built it

### Hardware stack
- 2× M5Go (ESP32-based, built-in IMU MPU6886, speaker, RGB LEDs, 320×240 LCD)
- No external sensors — all detection runs on the M5Go's built-in IMU

### Edge AI — real-time on-device inference
The phone detection does not use a simple threshold. We trained a **TinyML gesture classifier** using **Edge Impulse** on the M5Go's MPU6886 IMU, collecting three classes of motion data:

- `picked_up` — deliberate phone grab gestures
- `idle` — normal desk activity (typing, writing, bumping)

The model runs **entirely on the ESP32** — no cloud, no WiFi required for inference. A **sliding window approach** (25% overlap) feeds fresh IMU data into the classifier every ~375ms, giving near-instant response to pick-up events without the 1.5s blocking delay of a naive collect-then-classify loop.

```cpp
// Slide 25% of buffer — collect only fresh quarter each cycle
size_t quarter = (samples_required / 4) * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
memmove(buffer, buffer + quarter,
        (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - quarter) * sizeof(float));
run_classifier(&signal, &result, false);
```

### Firmware
The M5Go runs on **M5Unified + M5GFX**. The IMU's accelerometer determines tilt orientation to select Pomodoro duration — no buttons needed for mode selection. Game state is managed as a finite state machine:

```
IDLE → FOCUS_ACTIVE → DANGER → (RESUME | HURT)
          ↑                          ↓
          └──────── BREAK ───────────┘
```

### Display & audio
All graphics are drawn procedurally using M5GFX primitives — no bitmap storage needed, keeping heap usage low. Each game event has a corresponding speaker tone: a three-note rising fanfare on level up, a hurt chirp on phone lift, a gentle chime on session complete.

---

## Challenges we ran into

**Real-time inference latency.** A naive collect-then-classify approach blocked for 1.5 seconds per window — too slow for responsive detection. We implemented a sliding window with 25% overlap so only 37 fresh samples are collected per cycle, reducing response time to ~375ms while maintaining classification accuracy.

**IMU noise vs genuine pick-up.** The MPU6886 sees vibrations from typing, book drops, and chair bumps — all of which look similar to a pick-up at the raw signal level. Training the Edge AI model with diverse `idle` samples (typing, writing, desk bumps) taught the classifier to ignore these, something a hardcoded threshold cannot do.

**ESP-NOW MAC address pairing.** ESP-NOW requires knowing the receiver's MAC address at compile time. We added auto-targeting logic — each device reads its own MAC on boot and automatically sets the other device as its peer, so the same firmware runs on both M5Gos without modification.

**Alarm false triggers.** A 2.5-second alarm buffer prevents repeated alarms from a single pick-up event, giving the user time to put the phone back without being spammed.

---

## Accomplishments that we're proud of

- **Real-time Edge AI inference on a microcontroller** — the entire TinyML pipeline from data collection to on-device inference runs on the ESP32's 240MHz dual-core processor with no cloud dependency
- A fully wireless system — the M5Go sits on your desk with zero cables attached
- Sub-millisecond peer-to-peer communication over ESP-NOW, without any network infrastructure
- A creature that feels genuinely expressive — animated face, LED color shifts, and audio feedback give WatchDog a personality that makes you actually care about your level
- The tilt-to-select interaction for Pomodoro duration feels surprisingly natural and required no extra hardware beyond the built-in IMU

---

## What we learned

- Training a gesture classifier with Edge Impulse and deploying it for **real-time inference on a microcontroller** was a first — doing the entire pipeline from data collection to embedded deployment from scratch was the most valuable technical experience of this project
- ESP-NOW is dramatically underused for local multiplayer hardware projects. The zero-infrastructure, low-latency peer link is a perfect fit for desk devices
- Physical feedback loops change behavior in ways that software alone doesn't. When WatchDog flashes red and beeps, the instinct to put the phone down is immediate — something a phone notification can never replicate
- Finite state machines are the right abstraction for embedded game logic. Every attempt to handle game states with nested `if` blocks became unmaintainable within hours

---

## What's next for WatchDog

- **Mobile companion app** — syncs your guardian's stats to your phone (when the session is *over*), showing weekly level history and milestones
- **Cloud leaderboard** — expanding multiplayer beyond two local devices to a dorm-wide or campus-wide leaderboard
- **Adaptive difficulty** — using session completion history to dynamically adjust the alarm sensitivity, making WatchDog harder as your streak grows
- **Sleep mode integration** — using the PIR sensor (included with the M5Go kit) to detect when you leave your desk and automatically pause the timer, preventing unfair penalties during legitimate breaks