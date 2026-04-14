import numpy as np
from scipy.optimize import curve_fit

temps_C = np.array([10, 25, 35])
temps_K = temps_C + 273.15
resist = np.array([28642, 12997, 8333])  # iz simulatora
lnR = np.log(resist)
invT = 1 / temps_K

def sh(x, A, B, C):
    return A + B * x + C * x**3

popt, pcov = curve_fit(sh, lnR, invT)
A, B, C = popt
print(f"#define A {A}")
print(f"#define B {B}")
print(f"#define C {C}")
