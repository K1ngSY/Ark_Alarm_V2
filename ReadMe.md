# Ark Alarm Bot V2  

Ark Alarm Bot V2 is an automated monitoring and alerting tool for Windows designed for *ARK: Survival Ascended*.  
The project is fully rewritten in C++17 with Qt, leveraging OCR and image processing to provide unattended game protection.

## Features Overview

- **Automated Monitoring**:  
  The `Scanner` module periodically captures the game window, uses Tesseract OCR to detect Desmodus-related prompts and tribe logs, and triggers text, image, or phone-call alerts depending on user configuration.

- **Crash Handling**:  
  `CrashHandler` detects crash pop-ups, automatically closes them, and restarts the game.

- **Auto Rejoin**:  
  After a crash, `Rejoiner` automatically rejoins the server.  
  It detects whether the server uses mods, waits for the mod download to complete, then continues entering the server.

- **Message Sending**:  
  `Sender` interacts with third-party communication apps, simulating text or image input and dispatching alert messages.

- **Utility Tools**:
  - `motion.*` simulates mouse and keyboard actions.  
  - `visual.*` handles screenshots and image processing.  
  - `utility.*` provides window searching, crash window detection, and other helpers.

- **Multilingual Support**:  
  The project includes the translation file `Ark_Alarm_V2_zh_CN.ts`.  
  The program loads the corresponding translation at startup.

## Project Structure

- `dashboard.*` – Main UI and logic.  
- `scanner.*` – Core monitoring and alert implementation.  
- `crashhandler.*` – Game crash detection & restart workflow.  
- `rejoiner.*` – Automated server rejoin logic.  
- `sender.*` – Message dispatching to communication app windows.  
- `motion.*` – Input simulation utilities.  
- `visual.*` – Screenshot, OCR, and image recognition.  
- `utility.*` – Window-related helper functions.  
- `util/` – Contains sound effects (`Log_Alarm.wav`, `P_Alarm.wav`).

## Dependencies

- Qt (as defined in `Ark_Alarm_V2.pro`: `core`, `gui`, `network`, `widgets`, `multimedia`)  
- OpenCV  
- Tesseract OCR & Leptonica  
- Windows SDK (Win32 API)

After configuring header and library paths, the project can be compiled using `qmake` or Qt Creator.

## Usage Instructions

1. Ensure the `tessdata` directory is placed alongside the executable, containing both `chi_sim` and `eng` trained data files.  
2. In the interface, set parameters such as the game window title, communication app window title, and coordinate-related settings.  
3. After starting monitoring, the program periodically captures screenshots and sends alerts based on detected log triggers.

## License

No open-source license is included in the repository; all rights are reserved by default.