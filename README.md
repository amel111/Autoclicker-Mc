# Autoclicker-Mc

A custom hold-to-autoclick script specifically built for Minecraft Bedrock (mcpelauncher) on Hyprland. 

I made this because normal autoclickers are either clunky on Wayland or don't work reliably when you try to use them while physical mouse buttons are actually being held down. It hooks directly into the Linux input layer (`interception-tools`) and uses `ydotool` to send clicks. It also only activates when the Minecraft window is actively focused, so you don't accidentally wreck your desktop if you tab out.

## How it works
Normally, if you physically hold down the left-click and try to send synthetic clicks over it, the game just ignores the synthetic clicks because it thinks the button is already held down. This tool intercepts the mouse inputs, and injects a "fake release" to trick the system, allowing `ydotool` to safely spam clicks under the hood. 

The trigger is **Order-Independent**: You can hold the side Forward button, then Left/Right click, OR hold the click first and then press the Forward button. Releasing either one instantly stops the autoclicker. No lock-ups, no race conditions.

## Dependencies

You're going to need a few packages installed for this to work:
- `interception-tools` (for `udevmon` and `intercept`)
- `ydotool` (the background daemon needs to be running and its socket at `/tmp/.ydotool_socket`)
- `gcc` (to compile the C script)
- `hyprland` (uses `hyprctl` to check window focus)
- `libnotify` (for the `notify-send` onscreen indicators)

## Setup & Installation

I hardcoded a few paths for my setup, so you might need to tweak them depending on your system and hardware, especially the mouse device ID.

**1. Find your mouse ID**
Run `ls -la /dev/input/by-id/` and look for your mouse event file. 
Open `udevmon.yaml` and replace the `LINK:` path under `DEVICE:` with your actual mouse. 

**2. Compile the interceptor**
```bash
gcc -o combo combo.c
sudo cp combo /usr/bin/interception-combo
```

**3. Move the bash script**
Make sure the bash script is executable and in the right place (or edit `combo.c` to point to wherever you actually save it instead):
```bash
chmod +x autoclicker.sh
mkdir -p ~/.config/scripts/
cp autoclicker.sh ~/.config/scripts/
```

**4. Set up udevmon**
Copy the YAML file to the interception directory and restart the service so it starts listening to your mouse:
```bash
sudo mkdir -p /etc/interception/
sudo cp udevmon.yaml /etc/interception/udevmon.yaml
sudo systemctl restart udevmon
```

## Troubleshooting
If it doesn't work, check these two log files. They will usually point out exactly what's wrong:
- `cat /tmp/combo_debug.log` - This shows the raw button inputs and whether the C program is successfully catching your combos.
- `cat /tmp/autoclicker_script.log` - This shows what the bash script is doing (e.g., if it's failing to find the Hyprland signature or if it thinks Minecraft isn't focused).
