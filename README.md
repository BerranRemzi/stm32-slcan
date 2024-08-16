# USB to Virtual Serial Port with SLCAN Communication Protocol

## Overview

This project is a USB-to-virtual serial port device that supports the SLCAN communication protocol. Using a virtual serial port interface allows seamless communication between a computer and a CAN bus.

## Features

- **USB to Virtual Serial Port**: Converts USB communication to a virtual serial port.
- **SLCAN Protocol**: Supports the SLCAN (Serial Line CAN) protocol for CAN bus communication.
- **Cross-Platform**: Compatible with Windows, Linux, and macOS.
- **Open Source**: The project is open source and contributions are welcome.

## Getting Started

### Prerequisites

- A computer with a USB port.
- A CAN bus to connect to.
- VS Code
- PlatformIO

### Usage

Once the device is connected and recognized by your computer, it will appear as a virtual serial port. You can use standard serial communication tools to interact with the CAN bus.

### Commands

The following commands are supported by the device. Each command is followed by a TODO mark indicating that the implementation is pending.

- [ ] S: Set the CAN bitrate
- [ ] s: Set the CAN bitrate with extended options
- [ ] O: Open the CAN channel
- [ ] L: Open the CAN channel in listen-only mode
- [ ] C: Close the CAN channel
- [x] t: Transmit a standard CAN frame
- [ ] T: Transmit an extended CAN frame
- [ ] r: Transmit a standard remote frame
- [ ] R: Transmit an extended remote frame
- [ ] P: Poll the CAN channel status
- [ ] A: Set the acceptance code
- [ ] F: Set the acceptance mask
- [ ] X: Sets Auto Poll/Send ON/OFF for received frames.
- [ ] W: Filter mode setting
- [ ] M: Sets Acceptance Code Register
- [ ] m: Sets Acceptance Mask Register
- [ ] U: Set the UART bitrate
- [ ] V: Get the firmware version
- [x] N: Get the serial number
- [ ] Z: Sets Time Stamp ON/OFF for received frames only.
- [ ] Q: Query the device status

Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your changes.

License
This project is licensed under the MIT License - see the LICENSE file for details.

Acknowledgments
Thanks to the open-source community for their contributions and support.
