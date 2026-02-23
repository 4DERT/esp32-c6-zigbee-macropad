# ESP32-C6 Zigbee Macropad

A compact, 3D-printable 6-key mechanical macropad powered by the ESP32-C6. This project functions as a stylish desk gadget that allows you to easily control your smart home devices via Zigbee. 

The custom 3D-printed enclosure features a dedicated mounting point specifically designed for the **DFRobot Beetle ESP32-C6** development board and supports standard mechanical keyboard switches (Cherry MX, Outemu, etc.).

<p align="center">
  <img src="images/Macropad_1.png" alt="Macropad Main View" width="800"/>
</p>

<img src="images/macropad_demo.gif" width="50%" height="50%"/>

## Features

* **6 Customizable Keys:** Supports up to 6 mechanical switches.
* **Multi-Action Support:** Every key independently supports **Single Press**, **Double Press**, and **Long Press** actions.
* **Zigbee Router Capability:** Can act as a Zigbee Router to extend your mesh network (configurable via `menuconfig` -> *Component config* -> *Zigbee* -> *Configure the Zigbee device type*).
* **RGB Lighting:** Support for WS2812 addressable LED strips with various animations. It also supports standard, non-addressable LEDs (like the board's built-in LED).
* **Function (FN) Layer:** A secondary layer is available for device management (accessed by holding a designated key, default is **Key 5**).
  * **Lighting Control:** Use the FN layer to change RGB effects, brightness, and colors.
  * **Configurable Activation:** You can set the FN layer to trigger via a standard *Classic Hold* (active while pressed) or via a *Long Press* (which frees up the single/double press actions of that key for additional Zigbee commands).
  * **Factory Reset:** Hold **FN (Key 5)** and press **Key 2** (the key directly above it) to factory reset the device and force it to leave the current Zigbee network.

## Gallery

<p align="center">
  <table>
    <tr>
      <td><img src="images/Macropad_2.png" alt="Macropad View 2" width="400"/></td>
      <td><img src="images/Macropad_3.png" alt="Macropad View 3" width="400"/></td>
    </tr>
    <tr>
      <td><img src="images/Macropad_5.png" alt="Macropad View 5" width="400"/></td>
      <td><img src="images/Macropad_6.png" alt="Macropad View 6" width="400"/></td>
    </tr>
  </table>
</p>

## 🛠️ Hardware & Components
* 1x [DFRobot Beetle ESP32-C6](https://www.dfrobot.com/product-2778.html)
* 6x Mechanical Keyboard Switches - pick your favorite color/feel - [Example Link](https://aliexpress.com/item/1005005876614581.html)
* *(Optional)* RGB Strip: 5mm WS2812B 100 LEDs/m. You only need 10 LEDs total (5 for each side) - [Example Link](https://aliexpress.com/item/1005006739134777.html)
* *(Optional)* 1x 100–470 µF Low ESR Capacitor (Highly recommended if using the WS2812 RGB strip)
* Hookup wire (silicone wire recommended for flexibility)
* 2x M2 20mm screws
* 1x M2 4mm screw (or 5mm with a washer)
* 3x M2 Knurled threaded inserts (M2\*3\*3.5)

## 3D Printing & Assembly
The `STL` folder contains all the core enclosure files (`Base`, `Plate`, etc.).

> **Note about Keycaps:** Keycap files are not included in the repository. You can use any standard keycaps that fit your chosen mechanical switches (e.g., Cherry MX compatible), or you can easily 3D print your own! If you want to print them, here is a great [DSA-like MX Keycap model on MakerWorld](https://makerworld.com/en/models/1412642-keycap-mx-dsa-like) that fits perfectly.

> **Printing Tip:** If you are using the addressable RGB strip, it is highly recommended to print the `Base` in **Black** (to prevent light bleed) and the `Plate` in **White** (to nicely diffuse the LED light).

1. Insert the knurled threaded nuts into the designated mounting posts in the base using a soldering iron.
2. Snap your mechanical switches into the printed `Plate`.
3. Solder wires to the switch pins and route them to the ESP32-C6. 

### Wiring Guide

| Button | ESP32-C6 GPIO |
| :---: | :---: |
| **0** | `GPIO 21` |
| **1** | `GPIO 22` |
| **2** | `GPIO 20` |
| **3** | `GPIO 7` |
| **4** | `GPIO 19` |
| **5** | `GPIO 6` |

<img src="images/Macropad_8.png" alt="Wiring Diagram" width="600"/>

**RGB Strip (WS2812):** 
* Connect the data line to **GPIO 17**.
* Connect Power to **VIN** (5V) and Ground to **GND**. 
* *Note: It is highly recommended to solder a 100–470 µF Low ESR capacitor across VIN and GND to stabilize power for the LEDs.*

*(You can easily change the pinout configuration in `menuconfig`. The configuration above matches the pre-compiled binary available in the Releases tab).*

Once wired, carefully tuck the wiring and components into the enclosure and screw everything together.

<img src="images/Macropad_7.png" alt="Internal Assembly" width="600"/>

## Building & Flashing

### Quick Flash (Pre-compiled Binary)
If you don't want to set up a development environment, you can flash the pre-compiled firmware directly from your browser!

1. Download the latest firmware image (`.bin` file) from the `Releases page` of this repository.
2. Open the [ESP Web Flasher by Spacehuhn](https://esptool.spacehuhn.com/) in a Web Serial-compatible browser (e.g., Google Chrome or Microsoft Edge).
3. Connect your ESP32-C6 to your computer via USB and click **Connect** on the webpage.
4. Select the correct serial port for your device.
5. Upload the downloaded `.bin` file, set the flash address to `0x0`, and click **Program** to flash the device.

### Build from source code
1. Clone the repository and open it in **VS Code**.  
   A ready-to-use [Dev Container](https://code.visualstudio.com/docs/devcontainers/containers) configuration is provided. Once you open the project in VS Code with the *Dev Containers* extension installed, it will automatically pull an environment with **ESP-IDF**, Python tools, and all necessary dependencies preinstalled.  

2. If you prefer building locally (without Docker), install [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/get-started/) on your machine.  

3. Configure your build options using `menuconfig`. Here you can customize GPIO pins, change the `MANUFACTURER_NAME`, `MODEL_IDENTIFIER`, and more.
    ```bash
    idf.py menuconfig
    ```

4. Set the target to ESP32-C6:
    ```bash
    idf.py set-target esp32c6
    ```

5. Build the project, flash it to your board, and open the serial monitor:
    ```bash
    idf.py build flash monitor
    ```

## First Boot & Zigbee Pairing

After flashing the firmware:

1. Power the device via USB.
2. The device will automatically enter Zigbee pairing mode.
3. In Home Assistant (ZHA), enable pairing and wait for the device to appear.

## Home Assistant Integration (ZHA)
To make integrating the macropad with Home Assistant as easy as possible, a pre-made automation template is included in this repository.

Instead of creating triggers from scratch, you can use the provided template and simply swap out the placeholder device address with your own.

**1. Find your Device IEEE Address:**
Every Zigbee device has a unique MAC address. To make the automation work, you need to find yours:
* In Home Assistant, navigate to **Settings** -> **Devices & Services**.
* Open **Zigbee Home Automation** integration.
* Find and select your Macropad from the device list.
* Under the **Device info** panel copy the `IEEE` address (it will look something like `7c:2c:67:ff:fe:51:a0:00`).

**2. Import the Automation:**
* Open the [`ha_automation_template.yaml`](ha_automation_template.yaml) file located in this repository and copy its contents.
* In Home Assistant, go to **Settings** -> **Automations & Scenes** and create a new automation.
* Click the three dots in the top right corner and select **Edit in YAML**.
* Paste the copied code.
* Use the search and replace function to replace the placeholder `7c:2c:67:ff:fe:51:a0:00` with the **Device IEEE** you copied in step 1.
* Save the automation, switch back to the Visual Editor, and assign your desired actions to each button press!

## Contributing

Contributions, issues, and feature requests are highly welcome! 

If you find a bug or have an idea for a new feature, please feel free to open an issue on the Issues page. 

If you want to contribute directly to the code or documentation, simply fork the repository, make your changes, and submit a **Pull Request**. 

## License

This project is open-source and available under the [MIT License](LICENSE).`