# SPADCam


## Troubleshooting
The folder SPADCam is a Python virtual environment which has all the correct libraries installed. To activate it, run `source SPADCam/bin/activate`

If there is an issue with virtualenv not being on the $PATH (e.g. you get the error `bash: virtualenv: command not found` when running virtualenv), try running `PATH=/home/uwgraphics/.local/bin:$PATH`

If you're getting an error related to the Arducam that's something like "can't find/access /dev/video0", make sure that the ribbon cable is plugged in correctly (side with text on it should face away from USB ports) and make sure the pi has been restarted since it was connected.

When trying to install numpy (necessary for ArduCamDepthCamera library, but weirdly not installed automatically with it), I ran into the issue described here: https://github.com/numpy/numpy/issues/16012. The fix mentioned there (`sudo apt-get install libatlas-base-dev`) worked to fix the issue for me

## Resources
[Arducam Depth Camera Documentation](https://docs.arducam.com/Raspberry-Pi-Camera/Tof-camera/TOF-Camera/)