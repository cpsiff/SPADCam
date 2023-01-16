"""
The goal of this script is to align the depth bins in the histogram with the readings from the
Arducam depth camera, and to spatially align the FoVs of the SPAD and the Arducam
"""

import numpy as np
import matplotlib.pyplot as plt
import os

DATA_DIR = "captures/Pi-1-13/ex01_vary_depth/40cm"

def main():
    depth_img = np.load(os.path.join(DATA_DIR, "depth.npy"))
    amp_img = np.load(os.path.join(DATA_DIR, "amplitude.npy"))
    hists = np.load(os.path.join(DATA_DIR, "hists.npy"))

    depth_hist = get_depth_hist(depth_img, amp_img, mask=None)
    
    print("Max bin in SPAD:", hists[5].argmax())
    print("Max bin converted to dist:", convert_bin_to_dist(hists[5].argmax()))
    print("Calculated max range:", convert_bin_to_dist(128))

    fig, ax = plt.subplots(1, 2)

    plot_histogram(ax[0], depth_hist[0])
    ax[0].set_title("From Depth Camera")

    plot_histogram(ax[1], hists[5])
    ax[1].set_title("From SPAD")

    fig.set_size_inches(10, 6)
    plt.tight_layout()
    plt.show()


def get_depth_hist(depth_img, amp_img, mask=None):
    """
    Get a histogram of depths from some depth image, optionally masked by a binary mask
    """
    return np.histogram(
        depth_img,
        bins=128,
        range=(0, 1.0)
    )

def plot_histogram(ax, hist):
    ax.bar(range(len(hist)), hist, width=1.0)
    ax.set_ylabel("detection count")
    ax.set_xlabel("bin number / time / distance")

"""
Convert some histogram bin number to its corresponding distance. Found via SPAD_bins_to_dist.py
"""
def convert_bin_to_dist(bin):
    return (bin*1.298)-16.8

if __name__ == "__main__":
    main()