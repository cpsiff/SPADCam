"""
Simultaneously take a snapshot from Arducam depth camera and from TMF8820 SPAD
"""

import ArduCamDepthCamera as ac

def get_depth_frame(cam):
    frame = cam.requestFrame(200)
    if frame != None:
        depth_buf = frame.getDepthData()
        amplitude_buf = frame.getAmplitudeData()
    
    return depth_buf, amplitude_buf

def main():
    cam = ac.ArducamCamera()
    if cam.init(ac.TOFConnect.CSI,ac.TOFOutput.DEPTH,0) != 0 :
        print("initialization failed")
    if cam.start() != 0 :
        print("Failed to start camera")

    depth_frame, amplitude_frame = get_depth_frame(cam)

    print(depth_frame.shape)
    print(amplitude_frame.shape)

if __name__ == "__main__":
    main()