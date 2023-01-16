"""
Figure out what linear equation best converts SPAD bin to actual distance by doing linear regression
over x, y where x is the actual distance and y is the observed peak in the SPAD
"""

from scipy import stats
import matplotlib.pyplot as plt

# xs and ys are gathered from experiments (ex01_vary_depth)
xs = [3, 5, 10, 15, 20, 25, 30, 40]
ys = [15, 17, 21, 25, 28, 32, 36, 44]

def main():
    res = stats.linregress(ys, xs)
    print(res)

    fig, ax = plt.subplots()
    ax.scatter(xs, ys)
    ax.set_xlim([0, max(xs)+2])
    ax.set_ylim([0, max(ys)+2])
    ax.set_xlabel("Actual distance")
    ax.set_ylabel("Observed peak")
    plt.show()
    

if __name__ == "__main__":
    main()