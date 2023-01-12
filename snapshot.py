"""
Simultaneously take a snapshot from Arducam depth camera and from TMF8820 SPAD
"""

import ArduCamDepthCamera as ac
import matplotlib.pyplot as plt
from PIL import Image as im
import numpy as np
import os
import time
from read_histograms import process_raw_hists, TMF882X_IDX_FIELD
import serial
import csv

CAPTURES_DIR = "captures"
ARDUINO_SERIAL_PORT = "/dev/ttyACM0"

def get_depth_frame(cam):
    frame = cam.requestFrame(200) #TODO what is 200?
    if frame != None:
        depth_buf = frame.getDepthData()
        amplitude_buf = frame.getAmplitudeData()
    else:
        raise RuntimeError("Depth camera frame failed to capture")

    cam.releaseFrame(frame)
    return depth_buf, amplitude_buf

def get_histogram():

    arduino = serial.Serial(port=ARDUINO_SERIAL_PORT, baudrate=115200, timeout=0.1)
    buffer = []

    frames_finished = 0

    while frames_finished < 2:
        line = arduino.readline().rstrip()
        if frames_finished >= 1:
            buffer.append(line)

        try:
            decoded_line = line.decode('utf-8').rstrip().split(',')
            if len(decoded_line) > TMF882X_IDX_FIELD and decoded_line[TMF882X_IDX_FIELD] == "29":
                frames_finished += 1
        except UnicodeDecodeError:
            pass # if you start in a weird place you get random data that can't be decoded, so just ignore

    return process_raw_hists(buffer)

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

    # scale depth frame so that 4m is total white in depth image
    # this mimics https://docs.arducam.com/Raspberry-Pi-Camera/Tof-camera/Arducam-ToF-Camera-SDK/
    depth_frame = np.nan_to_num(depth_frame)
    depth_frame = (depth_frame/4)*255
    depth_frame = np.clip(depth_frame, 0, 255)

    im.fromarray(depth_frame).convert('L').save(f"{save_dir}/depth.png")

    # scale amplitude frame so that 1024 (this is a guess) is total white in depth image
    amplitude_frame = (amplitude_frame/1024)*255

    im.fromarray(amplitude_frame).convert('L').save(f"{save_dir}/amplitude.png")

    hist = get_histogram()
    with open(f"{save_dir}/hists.csv", 'w+') as my_csv:
        csvWriter = csv.writer(my_csv, delimiter=',')
        csvWriter.writerows(hist)
        
    fig, ax = plt.subplots(3, 3)
    hist_idx = 1 # start at 1 b/c 0 is reference hist
    for row in range(3):
        for col in range(3):
            ax[row][col].bar(range(len(hist[hist_idx])), hist[hist_idx], width=1.0)
            hist_idx += 1
    
    plt.tight_layout()
    plt.savefig(f"{save_dir}/hists.png")

if __name__ == "__main__":
    main()
