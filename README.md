# FART MACHINE ADV - cloud build edition

This project includes a GitHub Actions workflow that builds the firmware in the cloud.
You can do the upload and download steps from an iPhone browser.

## Build from an iPhone

1. Create a GitHub account if you do not already have one.
2. In Safari, open GitHub and create a **new empty repository**.
3. Upload the *contents* of this project folder to the repository (not the ZIP file itself).
   Make sure `.github/workflows/build.yml`, `platformio.ini`, and `src/main.cpp` are included.
4. Open the repository's **Actions** tab.
5. Select **Build FART MACHINE ADV firmware**.
6. Tap **Run workflow** and confirm.
7. When the run finishes with a green check, open it and download the
   `FartMachineADV-firmware` artifact.
8. Unzip the artifact. It contains `firmware.bin`.

## Important hardware note

This is an initial cloud-build configuration for an ESP32-S3 / 16 MB flash / OPI PSRAM
Cardputer-Adv target using M5Unified. Exact board revisions and library releases can
differ. A successful compile does not prove the sound output or Wi-Fi behavior on
your individual device; test before relying on it.

## First boot

If no Wi-Fi credentials have been saved, the device creates:
- SSID: `FART-MACHINE-SETUP`
- Password: `prutt1234`
- Setup page: `http://192.168.4.1`

After saving your home Wi-Fi credentials, the screen displays the device IP address.
Open that address from a phone connected to the same home network.

The fart-style effects are synthesized in firmware. No external sound files are bundled.
