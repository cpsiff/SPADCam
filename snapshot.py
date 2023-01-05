"""
Simultaneously take a snapshot from Arducam depth camera and from TMF8820 SPAD
"""

import ArduCamDepthCamera as ac
from PIL import Image as im
import numpy as np
import os
import time

CAPTURES_DIR = "captures"

def get_depth_frame(cam):
    frame = cam.requestFrame(200) #TODO what is 200?
    if frame != None:
        depth_buf = frame.getDepthData()
        amplitude_buf = frame.getAmplitudeData()
    else:
        raise RuntimeError("Depth camera frame failed to capture")
        
    cam.releaseFrame(frame)
    return depth_buf, amplitude_buf

def main():

    curtime = time.strftime("%Y%m%d-%H%M%S")
    save_dir = os.path.join(CAPTURES_DIR, curtime)
    os.makedirs(save_dir)

    cam = ac.ArducamCamera()
    if cam.init(ac.TOFConnect.CSI,ac.TOFOutput.DEPTH,0) != 0 :
        print("initialization failed")
    if cam.start() != 0 :
        print("Failed to start camera")

    depth_frame, amplitude_frame = get_depth_frame(cam)

    print(depth_frame.shape)
    print(amplitude_frame.shape)

    print(amplitude_frame.max())

    # scale depth frame so that 4m is total white in depth image
    # this mimics https://docs.arducam.com/Raspberry-Pi-Camera/Tof-camera/Arducam-ToF-Camera-SDK/
    depth_frame = np.nan_to_num(depth_frame)
    depth_frame = (depth_frame/4)*255
    depth_frame = np.clip(depth_frame, 0, 255)

    im.fromarray(depth_frame).convert('L').save(f"{save_dir}/depth.png")

    # scale amplitude frame so that 1024 (this is a guess) is total white in depth image
    amplitude_frame = (amplitude_frame/1024)*255

    im.fromarray(amplitude_frame).convert('L').save(f"{save_dir}/amplitude.png")


if __name__ == "__main__":
    main()