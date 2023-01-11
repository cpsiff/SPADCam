"""
Read histograms from TMF8820, which has been loaded with the tmf882x_a/tmf882xa.ino sketch
"""

import serial
import csv
import time
import os

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

    while(True):
        line = arduino.readline().rstrip()
        buffer.append(line)
        if line.decode('utf-8').rstrip().split(',')[TMF882X_IDX_FIELD] == "29":
            processed_hists = process_raw_hists(buffer)
            # append_hists_to_file(f'{CUR_TIME}.csv', processed_hists)

            buffer = []

def process_raw_hists(buffer):

    print(len(buffer))
    if len(buffer) != 31:
        print("WARNING: Buffer wrong size - skipping and returning None")
        return None

    rawSum = [[0 for _ in range(TMF882X_BINS)] for _ in range(TMF882X_CHANNELS)]

    for line in buffer:
        data = line.decode('utf-8')
        data = data.replace('\r','')
        data = data.replace('\n','')
        row = data.split(',')

        if len(row) > 0 and row[0][0] == "#":
            if row[0] == '#Raw' and len(row) == TMF882X_BINS+TMF882X_SKIP_FIELDS: # ignore lines that start with #obj
                idx = int(row[TMF882X_IDX_FIELD]) # idx is the id of the histogram (e.g. 0-9 for 9 hists + calibration hist)
                if ( idx >= 0 and idx <= 9 ):
                    for col in range(TMF882X_BINS):
                        rawSum[idx][col] = int( row[TMF882X_SKIP_FIELDS+col] )                                      # LSB is only assignement
                elif ( idx >= 10 and idx <= 19 ):
                    idx = idx - 10
                    for col in range(TMF882X_BINS):
                        rawSum[idx][col] = rawSum[idx][col] + int(row[TMF882X_SKIP_FIELDS+col]) * 256               # mid
                elif ( idx >= 20 and idx <= 29 ):
                    idx = idx - 20
                    for col in range(TMF882X_BINS):
                        rawSum[idx][col] = rawSum[idx][col] + int(row[TMF882X_SKIP_FIELDS+col]) * 256 * 256         # MSB

        else:
            print("Incomplete line read - ignoring")

    return rawSum

def append_hists_to_file(file_name, hists):
    os.makedirs(OUTPUT_FOLDER, exist_ok=True)

    with open(os.path.join(OUTPUT_FOLDER, file_name), 'a+', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerows(hists)

if __name__ == "__main__":
    main()