# Wordle: LED Matrix Edition
A C++ implementation that runs the popular game Wordle on an LED matrix panel. This is designed to be run on a Raspberry Pi Zero W 2, but can be configured to run on any Pi device. This program requires the use of a 128 x 64 HUB75 matrix panel, and pulls data directly from the New York Times' Wordle using Python.

## Features
*   **Real-time Animations:** Smooth tile flips, pop effects on key press, and shake animations for invalid words.
*   **Dynamic Win / Loss Screens:** Displays "Victory" or "Defeat" along with real-time statistics and animations.
*   **Real-time Updates:** Data is updated every night at 12:01 AM to your local time using cron to schedule the program.
*   **Auto Turn On:** The screen automatically turns on at the time you set it to. No need to manually run the code.
*   **Easy Setup:** One setup script that configures the C++ and Python scripts to run at their respective times.
*   **Statistics Tracking:** Keeps track of games played, wins, current streak, and max streak.

## Hardware Requirements
*   **Raspberry Pi Zero W 2** (or any Raspberry Pi model)
*   **128x64 HUB75E LED Matrix Panel**
*   **Adafruit RGB Matrix HAT or Bonnet** (for connecting the LED panel to the Pi)
*   **5V Power Supply** (sufficient amperage for your LED panel - typically 4A or higher)
*   **Keyboard** (USB or Bluetooth keyboard for Pi Zero W 2)

## Installation

### Software Dependencies
*   [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) - LED matrix driver library by Henner Zeller
*   Python 3 - for pulling word list and today's answer from New York Times
*   OTF2BDF - Converts and formats the fonts for use with the LED Matrix
*   Git, Make, GCC/G++ - Build tools

### Installation Guide

1.  **Clone the repository:**
```bash
    git clone https://github.com/mmccoy13/rpi-led-wordle.git
    cd rpi-led-wordle
```

2.  **Make the installation script executable:**
```bash
    chmod +x install.sh
```

3.  **Run the installation script:**
```bash
    ./install.sh
```
    The script will:
    - Update your system
    - Install all dependencies
    - Compile the LED matrix library
    - Convert fonts to BDF format
    - Fetch today's Wordle data
    - Compile the game
    - Schedule automatic updates and game start time
    
4.  **Follow the prompts** to set your preferred game start time.

## Controls

*   **A-Z Keys:** Enter letters into the current row
*   **Backspace/Delete:** Remove the last letter
*   **Enter/Return:** Submit your guess
*   **Ctrl+C or ESC:** Exit the game

## Configuration

### Adjusting LED Matrix Settings
If you're using a different LED matrix configuration, you can modify the flags in `install.sh` and when running manually:
```bash
sudo ./wordle_game \
  --led-rows=64 \
  --led-cols=128 \
  --led-slowdown-gpio=4 \
  --led-pwm-lsb-nanoseconds=65 \
  --led-gpio-mapping=adafruit-hat \
  --led-no-drop-privs
```

### Changing Scheduled Times
To modify when the game runs or when data updates:
```bash
crontab -e
```

Edit the cron jobs:
- `01 00 * * *` - Data fetch time (default: 12:01 AM)
- `MM HH * * *` - Game start time (set during installation)

## File Structure
```
rpi-led-wordle/
├── install.sh              # Setup script
├── wordle_game.cpp         # Main game code
├── Makefile                # Build configuration
├── requirements.txt        # Python dependencies
├── fonts/                  # Font files (.ttf, .otf, .bdf)
├── python/
│   ├── wordle-data.py     # Python script to fetch daily word
│   ├── answer.txt         # Today's answer
│   └── valid_guesses.txt  # Valid word list
└── stats.txt              # Game statistics
```

## Troubleshooting

**Game doesn't display:**
- Check that all fonts are converted: `ls -la fonts/*.bdf`
- Verify data files exist: `ls -la python/answer.txt python/valid_guesses.txt`
- Test the LED matrix with the demo: `cd rpi-rgb-led-matrix/examples-api-use && sudo ./demo -D0`

**Fonts not loading:**
- Ensure all `.bdf` files are present in the `fonts/` directory
- Check that font names in code match actual filenames

**Data not updating:**
- Verify cron jobs are set: `crontab -l`
- Manually run the Python script: `cd python && python3 wordle-data.py`
- Check for internet connectivity

**Permission errors:**
- The game must be run with `sudo` to access GPIO pins
- Ensure install script was run as regular user (not root)

## Credits

*   **LED Matrix Library:** [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) by Henner Zeller
*   **Wordle:** Original game by Josh Wardle, now owned by The New York Times
*   **Fonts:** NYT Karnak Condensed, Fortune Mouner Regular, and custom pixel fonts

## License

[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)

[Wordle: LED Matrix Edition](https://github.com/mmccoy13/rpi-led-wordle) © 2026 by [Myles McCoy](https://github.com/mmccoy13) is licensed under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/). See the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.