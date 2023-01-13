"""
Read histograms from TMF8820, which has been loaded with the tmf882x_a/tmf882xa.ino sketch
"""

import serial
import time
import os
import matplotlib.pyplot as plt

from read_histograms import process_raw_hists

PORT = "/dev/ttyACM0"
OUTPUT_FOLDER = os.path.join(os.path.dirname(__file__), 'output/histograms/')

CUR_TIME = time.strftime("%Y%m%d-%H%M%S")

TMF882X_CHANNELS = 10
TMF882X_BINS = 128
TMF882X_SKIP_FIELDS = 3 # skip first 3 items in each row
TMF882X_IDX_FIELD = 2 # second item in each row contains the idx field

def main():
    arduino = serial.Serial(port=PORT, baudrate=115200, timeout=0.1)

    buffer = []

    fig, ax = plt.subplots()
    plt.ion()
    plt.show(block=False)

    first_buffer = True

    while(True):
        line = arduino.readline().rstrip()
        buffer.append(line)

        try:
            decoded_line = line.decode('utf-8').rstrip().split(',')
            if len(decoded_line) > TMF882X_IDX_FIELD and decoded_line[TMF882X_IDX_FIELD] == "29":
                if not first_buffer: # throw out the first buffer, as it is probably incomplete
                    processed_hists = process_raw_hists(buffer)
                    buffer = []
                    
                    if processed_hists is not None:
                        ax.clear()
                        hist = processed_hists[5] # plot just the center histogram for now
                        ax.bar(range(len(hist)), hist, width=1.0)
                        ax.set_ylabel("detection count")
                        ax.set_xlabel("bin number / time / distance")
                
                first_buffer = False
        except UnicodeDecodeError:
            pass # if you start in a weird place you get random data that can't be decoded, so just ignore

        # plt.pause() causes problems with reading from the serial port. It is what causes the
        # WARNING: Buffer wrong size messages. But, I can't figure out a way to plot without it
        plt.pause(0.0000001) # https://stackoverflow.com/questions/28269157/plotting-in-a-non-blocking-way-with-matplotlib

if __name__ == "__main__":
    main()