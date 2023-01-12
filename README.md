# SPADCam


## Troubleshooting
The folder SPADCam is a Python virtual environment which has all the correct libraries installed. To activate it, run `source SPADCam/bin/activate`

If there is an issue with virtualenv not being on the $PATH (e.g. you get the error `bash: virtualenv: command not found` when running virtualenv), try running `PATH=/home/uwgraphics/.local/bin:$PATH`

If you're getting an error related to the Arducam that's something like "can't find/access /dev/video0", make sure that the ribbon cable is plugged in correctly (side with text on it should face towards USB ports at the Pi end and towards the camera (so you can see the text if you can see the camera lens) at the camera end) and make sure the pi has been restarted since it was connected.

When trying to install numpy (necessary for ArduCamDepthCamera library, but weirdly not installed automatically with it), I ran into the issue described here: https://github.com/numpy/numpy/issues/16012. The fix mentioned there (`sudo apt-get install libatlas-base-dev`) worked to fix the issue for me

To connect an I2C depth sensor directly to the pi, you need to make the following connections: 

GND on sensor (black wire on qwiic) -> GND on Pi

3.3V on sensor (red wire on qwiic) -> 3.3V on Pi

SDA on sensor (blue wire on qwiic) -> BCM2 (SDA) on Pi

SCL on sensor (yellow wire on qwiic) -> BCM3 (SCL) on Pi

![pi_pins](pi_pins.png)

## Problem with hooking up an Arduino to a Raspberry Pi (1/12/23):

While all the pieces of the SPADCam program (e.g. snapshot.py) work in theory, I'm having trouble getting them to actual operate reliably on the Raspberry Pi. Here is why:

- There is a difference in behavior between the Arduino Nano 33 IoT and the Arduino Uno: The Nano 33 IoT runs its setup script once, when you upload to the Arduino, then it runs its loop continuously. The Arduino Nano 33 IoT does not care if you open or close the serial port listening to it. The Arduino Nanos on the other hand run their startup script only when you open a serial port, and run it every time a serial port is opened
- The Arduino sketch in `arduino/read_once` was written for the Arduino Nano 33 IoT hooked up to my laptop. This sketch can be laoded to the Arduino, then snapshot.py can be run perfectly fine (with no depth image capture, as depth camera only works with Pi)
- Code for the Arduino Nano 33 IoT cannot be compiled on the Raspberry Pi. Attempting to do so yields an error `/home/pi/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/7-2017q4/bin/arm-none-eabi-ar: error while loading shared libraries: libfl.so.2: cannot open shared object file: No such file or directory exit status 127`. As explained in [this forum thread](https://forum.arduino.cc/t/solved-install-samd-boards-in-ide-on-rpi4-exit-status-127/626103/2) this is a known bug. The solution is to revert to a previous version of the board library, but that previous version is from before the Nano 33 IoT was released.
- The Nano 33 IoT sketch cannot be uploaded on a laptop then plugged into a pi, because it requires loading firmware into the RAM on the TMF8820 itself, which disappears if it loses power.

Possible solutions:
- Create a "read once" sketch which just reads off a single measurement when a serial port is opened, and use it with uno
- Modify "read continuously" sketch to correctly disable then re-enable the sensor completely every time a serial port is opened
- Modify an arduino uno to not run the startup script every time a new serial port is connected [stack exchange post](https://arduino.stackexchange.com/questions/38468/disable-reset-when-com-port-connected-disconnected)


## Resources
[Arducam Depth Camera Documentation](https://docs.arducam.com/Raspberry-Pi-Camera/Tof-camera/TOF-Camera/)