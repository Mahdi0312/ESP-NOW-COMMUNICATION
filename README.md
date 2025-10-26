# ESP-NOW Communication Between Ventilation Units

## Overview
This module implements a communication protocol between multiple ventilation units using **ESP-NOW**.  
The system operates on a **master–slave hierarchy**, where one master coordinates the communication with several slave units in the same building.

ESP-NOW enables fast and low-power data transfer between ESP32/ESP8266 boards without requiring a Wi-Fi router or access point.

---

## 1. System Roles

### Master Unit
- Central controller of the network.
- Handles data synchronization and command transmission.
- Assigns a unique ID to each connected slave.

### Slave Units
- Send measurement data (temperature, humidity, etc.).
- Receive control commands from the master.
- Automatically reconnect to the master after power loss.

---

## 2. Button and LED Functionality

Each unit includes:
- **Push button:** used to change modes and trigger pairing.
- **LED indicator:** shows the current state (mode change, pairing, communication OK, etc.).

| Action | Click Count | Description |
|---------|--------------|-------------|
| Switch from Master → Slave | 3 clicks | Changes the role and ID of the unit |
| Enter Pairing Mode | 5 clicks | Starts manual pairing |
| Auto Reconnection | — | Triggered automatically after reboot |

---

## 3. ID Management

- All units start as **master** with `id = 0` by default.  
- When switched to **slave mode**, the ID becomes `id = 1`.  
- The master assigns incremented IDs (`id = 2`, `id = 3`, etc.) to each new slave added.  
- Each assigned ID is saved in **EEPROM** to remain valid after a power cycle.

---

## 4. ESP-NOW Frame Types

Three types of message frames are defined for communication:

| Frame Type | Description |
|-------------|-------------|
| `PAIRING` | Used during manual pairing between master and slave |
| `DATA` | Used for exchanging regular data (measurements, status, control) |
| `PAIRINGSlaveConnect` | Used for automatic reconnection after power loss |

---

## 5. Manual Pairing Procedure

1. Switch one unit to **slave mode** (3 clicks).  
2. Press the button **5 times** to enter **pairing mode**.  
3. The slave sends a `PAIRING` frame to the broadcast MAC address:  
