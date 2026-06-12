# Pi Zero Logic Analyser

A standalone logic analyser built around a Raspberry Pi Zero, ILI9488 LCD, M5Stack CardKB keyboard, and a Raspberry Pico-based GusmanB LogicAnalyzer capture device.

The goal of the project is to create a portable logic analyser with local waveform viewing, protocol decoding, measurements, file export, and future AI-assisted signal recognition.

---

# Hardware

## Host

* Raspberry Pi Zero W
* Raspberry Pi OS Lite
* 480x320 ILI9488 SPI LCD
* M5Stack CardKB I2C keyboard

## Capture Device

* GusmanB LogicAnalyzer
* Raspberry Pi Pico based
* USB CDC connection
* 8, 16 or 24 channel capture modes

---

# Features

## Implemented

### USB Capture

* Direct communication with GusmanB LogicAnalyzer
* Binary capture request protocol implemented
* Binary sample transfer implemented
* Capture status handling implemented

### Waveform Display

* Multi-channel waveform rendering
* Automatic scaling to fit capture width
* Zoom in/out
* Horizontal panning
* Cursor positioning
* Channel selection

### Event Conversion

Raw samples are converted into transition events:

```cpp
struct LogicEvent
{
    uint32_t sampleIndex;
    uint32_t state;
};
```

Benefits:

* Reduced memory usage
* Fast navigation
* Transition searching
* Efficient rendering
* Foundation for protocol decoding

### Measurements

* Cursor position display
* Marker A
* Marker B
* Delta samples
* Delta time
* Frequency calculation

### Navigation

* Jump to next transition
* Jump to previous transition
* Cursor-centred zooming

---

# Project Structure

```text
apps/
└── la01/
    └── main.cpp

drivers/
├── cardkb.*
├── gusman_logic.*
└── ili9488.*

gfx/
├── font8x16.*
├── palette.*
├── renderer.*
└── text.*

hal/
├── gpio.*
└── spi.*

logic/
├── logic_events.*
└── logic_view.*

terminal/
├── rect.h
└── screenbuffer.*
```

---

# Capture Pipeline

```text
USB Capture
        │
        ▼
GusmanCapture
        │
        ▼
LogicEventCapture
        │
        ▼
LogicView
        │
        ▼
ScreenBuffer
        │
        ▼
ILI9488 Display
```

---

# Current Controls

| Key   | Function                    |
| ----- | --------------------------- |
| +     | Zoom in                     |
| -     | Zoom out                    |
| Left  | Pan left                    |
| Right | Pan right                   |
| Up    | Select previous channel     |
| Down  | Select next channel         |
| A     | Set marker A                |
| B     | Set marker B                |
| N     | Jump to next transition     |
| P     | Jump to previous transition |
| Space | Recapture                   |

---

# Future Work

## Display

* Hide/show channels
* Minimise channels
* Expand channels
* Channel colours
* Time scale
* Trigger indicators
* Improved measurement display

## Measurements

* Pulse width
* Frequency
* Duty cycle
* Rise time
* Fall time

## Protocol Analysis

Planned architecture:

```text
Capture
    │
    ▼
Event Conversion
    │
    ▼
Protocol Analysis
    │
    ▼
Protocol Candidates
    │
    ▼
User Assignment
    │
    ▼
Protocol Decode
    │
    ▼
Waveform Labels
```

Planned protocols:

* PWM
* UART
* SPI
* I2C
* 1-Wire
* Custom protocols

## Export

* Raw capture
* CSV
* VCD

## Live Mode

* Continuous acquisition
* Scrolling waveform view
* Triggered capture mode

## AI Assisted Analysis

Future versions may optionally use cloud-based AI services over WiFi.

Possible capabilities:

* Guess unknown serial protocols
* Detect UART baud rates
* Identify SPI/I2C activity
* Recognise PWM signals
* Suggest protocol candidates
* Explain captured traffic

AI analysis will be optional and used in addition to deterministic protocol decoders.

---

# Design Philosophy

The project is designed around a simple layered architecture:

```text
Hardware Drivers
        │
        ▼
Capture Layer
        │
        ▼
Event Layer
        │
        ▼
Protocol Layer
        │
        ▼
Presentation Layer
```

Keeping these layers separate allows new protocol decoders, display modes, export formats and capture devices to be added without major redesign.

---

# License

Open source.

Built for experimentation, learning and embedded systems development.
