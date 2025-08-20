# LEDControlSystem

## Overview
The **LEDControlSystem** is an integrated solution designed to control up to six independent LEDs / LED strips. This project consists of four main components: two PCB projects called **LEDcontroller** and **OfflineFlyback**, **Firmware** for the LEDcontroller and a **Web Application** for controlling the system, providing users with a seamless and intuitive way to manage their LED lighting.

---

## Components

### 1. LEDcontroller
- **Description**: An LED controller based on the Espressif ESP-WROOM-32 module with six independent output channels.
  <details>
  <summary><strong>Characteristics:</strong></summary>

  - **Input voltage range:** 5 V<sub>DC</sub> - 36 V<sub>DC</sub>
  - **Output voltage:** same as input
  - **Number of independent channels:** 6
  - **Maximum output current per channel:** 3 A
  - **Maximum total output current:** 6,5 A
  </details>

  <details>
  <summary><strong>Features:</strong></summary>

  - Dedicated mode button
  - LED indicators: power, WiFi state, error
  - Connector for programming and debugging via UART
  </details>

### 2. OfflineFlyback
- **Description:** A 24 V / 55 W SMPS based on the STM VIPERGAN50 switcher and Wurth 750343068 transformer.
- **Topology:** Isolated quasi-resonant flyback with secondary side regulation.
  <details>
  <summary><strong>Characteristics:</strong></summary>

  - **Input voltage range:** 185 V<sub>AC</sub> - 265 V<sub>AC</sub>
  - **Output voltage:** 24 V<sub>DC</sub>
  - **Maximum output power:** 55 W
  - **Efficiency at full load:** Up to 90%
  - **Idle power consumption:** < 100 mW (@ 230 V<sub>AC</sub>)
  - **Output ripple and noise:** < 70 mV<sub>pp</sub>
  </details>

  <details>
  <summary><strong>Features:</strong></summary>

  - Compact design
  - Input overvoltage protection
  - Brown-in and brown-out
  - Output overvoltage protection
  - Output overload protection
  - Embedded thermal shutdown
  </details>

### 3. Firmware
- **Description**: Custom firmware based on the ESP-IDF framework 5.4.0 and FreeRTOS designed for the LEDcontroller.
  <details>
  <summary><strong>Features:</strong></summary>

  - **Supports various LED configurations**: single-color LEDs and RGB LEDs
  - **Channel dimming method:** PWM (10 bit, 5 kHz)
  - **Smooth transitions**: seamless changes when altering the state of the output channels
  - **Supports three different WiFi modes**: disabled, client mode and access point mode
  - **Custom web API**: used to change device settings and control output channels
  </details>

### 4. Web Application
- **Description**: A user-friendly web application that allows users to interact with the LEDcontroller.
  <details>
  <summary><strong>Features:</strong></summary>

  - Real-time control of output channels
  - Real-time control of device settings via the "admin" webpage
  - Compatible with various devices (PC, tablet, smartphone)
  </details>

---

## Web API

#### GET method: 

<details>
<summary><strong>Reading device configuration:</strong></summary>

| Description | URL | Query string supported names |
|-|-|-|
| Full device configuration          | http://\<_IP_>/api/conf   | /           |
| Number of active RGB LEDs          | http://\<_IP_>/api/conf   | rgb_outputs |
| Number of active single-color LEDs | http://\<_IP_>/api/conf   | w_outputs   |
| WiFi enable flag at device startup | http://\<_IP_>/api/conf   | enable_wifi |

</details>

<details>
<summary><strong>Reading output channel status and duty cycle:</strong></summary>

| Description | URL | Query string supported names |
|-|-|-|
| Enable status and duty cycle of all active channels | http://\<_IP_>/api/ch     | /                |
| Enable status and duty cycle of all active channels | http://\<_IP_>/api/ch/all | /                |
| Enable status of all active channels                | http://\<_IP_>/api/ch/all | ch_enable        |
| Duty cycle of all active channels                   | http://\<_IP_>/api/ch/all | ch_duty          |
| Enable status of individual active channel          | http://\<_IP_>/api/ch     | ch<_0-5_>_enable |
| Duty cycle of individual active channel             | http://\<_IP_>/api/ch     | ch<_0-5_>_duty   |

</details>

#### POST method: 

<details>
<summary><strong>Changing device configuration:</strong></summary>

| Description | URL | JSON key | Supported value |
|-|-|-|-|
| Number of active RGB LEDs          | http://\<_IP_>/api/conf   | rgb_outputs | 0 - 2 |
| Number of active single-color LEDs | http://\<_IP_>/api/conf   | w_outputs   | 0 - 6 |
| WiFi enable flag at device startup | http://\<_IP_>/api/conf   | enable_wifi | 0, 1  |

</details>

<details>
<summary><strong>Changing WiFi credentials for client mode:</strong></summary>

| Description | URL | JSON key | Supported value |
|-|-|-|-|
| SSID     | http://\<_IP_>/api/cred | WIFI_SSID     | String up to 30 characters |
| Password | http://\<_IP_>/api/cred | WIFI_PASSWORD | String up to 30 characters |

</details>

<details>
<summary><strong>Changing output channel status and duty cycle:</strong></summary>

| Description | URL | JSON key | Supported value |
|-|-|-|-|
| Enable status of all active channels                | http://\<_IP_>/api/ch/all | ch_enable        | 0, 1     |
| Duty cycle of all active channels                   | http://\<_IP_>/api/ch/all | ch_duty          | 0 - 1023 |
| Enable status of individual active channel          | http://\<_IP_>/api/ch     | ch<_0-5_>_enable | 0, 1     |
| Duty cycle of individual active channel             | http://\<_IP_>/api/ch     | ch<_0-5_>_duty   | 0 - 1023 |

</details>

---

**Disclaimer:** This project is a work in progress. Use the files, hardware and software given in this repository **at your own risk**.

---

## License
This project (LEDControlSystem) is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.
