# LED Matrix Wordle

A C++ implementation of the popular game Wordle, designed to run on a 64x128 RGB LED Matrix connected to a Raspberry Pi.

## Features
* **Real-time Animations:** Smooth tile flips, pop effects on key press, and shake animations for invalid words.
* **Persistent Stats:** Tracks wins, losses, current streak, and max streak (saved locally).
* **Dynamic Victory Screen:** Displays ranking ("Genius", "Splendid", etc.) based on performance.
* **Hardware Acceleration:** Utilizes the `rpi-rgb-led-matrix` library for high-refresh-rate driving.

## Hardware Requirements
* Raspberry Pi (3B+, 4, or Zero 2 W recommended)
* 64x128 RGB LED Matrix (P2.5 or P3)
* Adafruit RGB Matrix HAT + RTC (or similar interface board)

## Software Dependencies
* [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) library by Henner Zeller.
* Python 3 (for word list generation).

## How to Run
1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/mmccoy13/rpi-led-wordle.git](https://github.com/mmccoy13/rpi-led-wordle.git)
    cd rpi-led-wordle
    ```

2.  **Compile the game:**
    ```bash
    make
    ```

3.  **Run the game (requires root):**
    ```bash
    sudo ./wordle_game --led-rows=64 --led-cols=128 --led-slowdown-gpio=4 --led-gpio-mapping=adafruit-hat --led-no-drop-privs
    ```
    *(Note: flags may vary depending on your specific matrix hardware).*

## Controls
* **Keyboard:** Type to enter letters.
* **Enter:** Submit guess.
* **Backspace:** Delete letter.
* **Esc / Ctrl+C:** Quit game.

## License
Project created by Myles McCoy. Uses the RGB LED Matrix library by Henner Zeller.