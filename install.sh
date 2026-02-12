#!/bin/bash

# Get absolute path for Cron reliability
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

GREEN='\033[1;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

# --- Display ASCII Logo ---
clear
echo -e "${GREEN}"
cat << "EOF"
 _       __               __  __   
| |     / /___  _________/ / / /__ 
| | /| / / __ \/ ___/ __  / / / _ \
| |/ |/ / /_/ / /  / /_/ / / /  __/
|__/|__/\____/_/   \__,_/_/_/\___/ 
                                   
 RASPBERRY PI LED MATRIX EDITION   

EOF
echo -e "${NC}"

echo "Updating system..."
sudo apt-get update > /dev/null 2>&1
sudo apt-get upgrade -y > /dev/null 2>&1

echo "Installing dependencies..."
sudo apt-get install -y git python3 python3-pip python3-venv build-essential make g++ otf2bdf > /dev/null 2>&1

echo "Setting up Python..."
if [ ! -d "$SCRIPT_DIR/python/venv" ]; then
    python3 -m venv "$SCRIPT_DIR/python/venv"
fi

source "$SCRIPT_DIR/python/venv/bin/activate"
pip install --upgrade pip > /dev/null 2>&1
pip install -r "$SCRIPT_DIR/requirements.txt"

echo "Compiling LED Matrix library..."
if [ ! -d "$SCRIPT_DIR/rpi-rgb-led-matrix" ]; then
    git clone https://github.com/hzeller/rpi-rgb-led-matrix "$SCRIPT_DIR/rpi-rgb-led-matrix"
fi

cd "$SCRIPT_DIR/rpi-rgb-led-matrix"
make -j$(nproc)
cd "$SCRIPT_DIR"

echo "Converting fonts..."
otf2bdf -p 5 -r 72 -o fonts/3x3.bdf fonts/3x3pixel.ttf
otf2bdf -p 6 -r 100 -o fonts/5x6.bdf fonts/5x6Font.otf
otf2bdf -p 10 -r 100 -o fonts/defeat.bdf fonts/FortuneMounerRegular.otf
otf2bdf -p 9 -r 100 -o fonts/victory.bdf fonts/FortuneMounerRegular.otf
otf2bdf -p 9 -r 100 -o fonts/Karnak.bdf fonts/NYTKarnakCondensed.ttf

echo "Fetching data..."
(cd "$SCRIPT_DIR/python" && "$SCRIPT_DIR/python/venv/bin/python" "wordle-data.py")

echo "Compiling Wordle game..."
make

echo "Starting Wordle..."
sudo ./wordle_game --led-rows=64 --led-cols=128 --led-slowdown-gpio=4 --led-pwm-lsb-nanoseconds=65 --led-gpio-mapping=adafruit-hat --led-pixel-mapper=Rotate:90 --led-no-drop-privs 

echo "Scheduling tasks..."
crontab -l >/dev/null 2>&1 || echo "" | crontab -

JOB_PY="01 00 * * * cd $SCRIPT_DIR/python && $SCRIPT_DIR/python/venv/bin/python wordle-data.py"

if (crontab -l 2>/dev/null | grep -v "wordle-data.py"; echo "$JOB_PY") | crontab -; then
    echo " > Python fetch scheduled."
else
    echo -e "${RED} > Failed to schedule Python.${NC}"
fi

while true; do
    read -p "Run game at Hour (0-23): " HH
    read -p "Run game at Minute (0-59): " MM
    if [[ "$HH" =~ ^[0-9]+$ ]] && [ "$HH" -ge 0 ] && [ "$HH" -le 23 ] && \
       [[ "$MM" =~ ^[0-9]+$ ]] && [ "$MM" -ge 0 ] && [ "$MM" -le 59 ]; then
        break
    else
        echo -e "${RED}Invalid time.${NC}"
    fi
done

JOB_GAME="$MM $HH * * * cd $SCRIPT_DIR && sudo ./wordle_game --led-rows=64 --led-cols=128 --led-slowdown-gpio=4 --led-nanoseconds-pulse=65 --led-gpio-mapping=adafruit-hat --led-pixel-mapper=Rotate:90 --led-no-drop-privs"

if (crontab -l 2>/dev/null | grep -v "wordle_game"; echo "$JOB_GAME") | crontab -; then
    echo " > Game scheduled."
else
    echo -e "${RED} > Failed to schedule Game.${NC}"
fi