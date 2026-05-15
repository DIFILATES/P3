"""
Estimació del pitch — pav_2042.wav
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile

# ── Càrrega del fitxer ─────────────────────────────────────────────────────────
fs, data = wavfile.read('pav_2042.wav')
if data.ndim > 1:
    data = data[:, 0]                        
data = data.astype(float) / np.max(np.abs(data)) 

# ── Paràmetres ─────────────────────────────────────────────────────────
npitch_min = int(fs / 1000)   # f0 < 1000 Hz
npitch_max = int(fs / 20)    # f0 > 20 Hz
frame_len = int(fs * 0.03)   
offset    = int(fs * 0.8)   
npitch_max = min(npitch_max, frame_len // 2) 

x = data[offset : offset + frame_len].copy()

# Hamming
hamming = 0.54 - 0.46 * np.cos(2 * np.pi * np.arange(frame_len) / (frame_len - 1))
x *= hamming

# ── Autocorrelació ───────────────────────────────
N = len(x)
r = np.zeros(npitch_max)
for l in range(npitch_max):
    if N-l > 0: r[l] = np.sum(x[:N-l] * x[l:N]) / N

if r[0] == 0.0:
    r[0] = 1e-10

# ── Primer màxim secundari ──────────────────────────────────────────────────
imax = npitch_min
for l in range(npitch_min, npitch_max):
    if r[l] > r[imax]:
        imax = l

f0_est = fs / imax
T0_est = imax / fs

print(f"lag      = {imax} mostres")
print(f"T0       = {T0_est*1000:.2f} ms")
print(f"f0 est.  = {f0_est:.1f} Hz")
print(f"r1norm   = {r[1]/r[0]:.3f}")
print(f"rmaxnorm = {r[imax]/r[0]:.3f}")

# ── Figura ────────────────────────────────────────────────────────────────────
plt.style.use('dark_background')
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 7), facecolor='#0d1117')
fig.suptitle(f'Estimació del Pitch — pav_2042.wav   |   f0 ≈ {f0_est:.0f} Hz',
             color='#c9d1d9', fontsize=13, fontweight='bold')

COLOR_SIG  = '#58a6ff'
COLOR_T0   = '#f78166'
COLOR_AC   = '#3fb950'
COLOR_MAX  = '#ffa657'
GRID       = '#21262d'

# — Subplot 1: senyal temporal —
t_ms = np.arange(frame_len) / fs * 1000
ax1.set_facecolor('#0d1117')
ax1.plot(t_ms, x, color=COLOR_SIG, lw=1.2)

# Marques periode
k = 0
while k * T0_est * 1000 <= t_ms[-1]:
    ax1.axvline(k * T0_est * 1000, color=COLOR_T0, lw=1.0, ls='--', alpha=0.7)
    k += 1

# Fletxa t0
ax1.annotate('', xy=(T0_est*1000, 0.7), xytext=(0, 0.7),
             arrowprops=dict(arrowstyle='<->', color=COLOR_T0, lw=1.8))
ax1.text(T0_est*1000/2, 0.77,
         f'$T_0$ = {T0_est*1000:.1f} ms  ($f_0$ ≈ {f0_est:.0f} Hz)',
         color=COLOR_T0, ha='center', fontsize=9, fontfamily='monospace')

ax1.set_xlabel('Temps (ms)', color='#8b949e')
ax1.set_ylabel('Amplitud', color='#8b949e')
ax1.set_title('Senyal temporal — fonema sonor (~30 ms)', color='#c9d1d9')
ax1.set_xlim(0, t_ms[-1])
ax1.tick_params(colors='#8b949e')
ax1.grid(True, color=GRID, lw=0.5)
for sp in ax1.spines.values(): sp.set_edgecolor(GRID)

# — Subplot 2: autocorrelació —
lags_ms = np.arange(npitch_max) / fs * 1000
ax2.set_facecolor('#0d1117')
ax2.plot(lags_ms, r / r[0], color=COLOR_AC, lw=1.2)

ax2.axvspan(npitch_min/fs*1000, npitch_max/fs*1000,
            alpha=0.08, color='yellow', label='Zona de búsqueda')
ax2.axvline(imax/fs*1000, color=COLOR_MAX, lw=1.5, ls='--')
ax2.plot(imax/fs*1000, r[imax]/r[0], 'o', color=COLOR_MAX, ms=8, zorder=5)
ax2.annotate(f'1.er màxim sec.\nlag={imax} mostres\nT₀={T0_est*1000:.1f} ms',
             xy=(imax/fs*1000, r[imax]/r[0]),
             xytext=(imax/fs*1000 + 1.5, r[imax]/r[0] - 0.2),
             color=COLOR_MAX, fontsize=8, fontfamily='monospace',
             arrowprops=dict(arrowstyle='->', color=COLOR_MAX))

ax2.set_xlabel('Retard τ (ms)', color='#8b949e')
ax2.set_ylabel('r[l] / r[0]', color='#8b949e')
ax2.set_title('Autocorrelació normalitzada', color='#c9d1d9')
ax2.set_xlim(0, npitch_max/fs*1000 * 1.2)
ax2.tick_params(colors='#8b949e')
ax2.grid(True, color=GRID, lw=0.5)
for sp in ax2.spines.values(): sp.set_edgecolor(GRID)

plt.tight_layout()
plt.savefig('pitch_plot.png', dpi=150, bbox_inches='tight', facecolor='#0d1117')
print("Figura guardada: pitch_plot.png")
