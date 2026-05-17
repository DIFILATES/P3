import numpy as np
import matplotlib.pyplot as plt
import os

# 1. Carrega el teu f0 (C++) - Una sola columna amb els Hz
try:
    my_f0 = np.loadtxt('pav_2042.f0')
except Exception as e:
    print(f"Error llegint pav_2042.f0: {e}")
    my_f0 = np.array([])

# 2. Carrega el f0 de Wavesurfer - Necessitem la columna 0 (Pitch) i la columna 1 (Veu/Mut)
try:
    # Carreguem les dues primeres columnes
    ws_data = np.loadtxt('wavesurfer.f0', usecols=(0, 1))
    ws_f0 = ws_data[:, 0]       # Primera columna: pitch original
    ws_voiced = ws_data[:, 1]   # Segona columna: 1.0 si és veu, 0.0 si és mut
    
    # Forçar a 0 el pitch de Wavesurfer quan la columna de veu/mut és 0
    ws_f0[ws_voiced == 0] = 0.0

except Exception as e:
    print(f"Error llegint wavesurfer.f0: {e}")
    ws_f0 = np.array([])

# 3. Validar i retallar al mínim dels dos perquè quadrin perfectament en el temps
if len(my_f0) == 0 or len(ws_f0) == 0:
    print("Error: Un dels dos fitxers està buit o no s'ha trobat.")
else:
    n = min(len(my_f0), len(ws_f0))
    my_f0 = my_f0[:n]
    ws_f0 = ws_f0[:n]

    # Convertim l'eix X de trames a segons reals (cada trama és de 15ms = 0.015s)
    t = np.arange(n) * 0.015

    # Netegem els zeros (silencis/muts) substituint-los per NaN perquè no dibuixi línies lletges cap al terra
    my_f0[my_f0 == 0] = np.nan
    ws_f0[ws_f0 == 0] = np.nan

    # 4. Crear la gràfica
    plt.style.use('seaborn-v0_8-whitegrid')
    fig, ax = plt.subplots(figsize=(12, 4), facecolor='white')
    
    # Wavesurfer (Referència filtrada) en línia vermella
    ax.plot(t, ws_f0, label='Wavesurfer (ESPS - Ref)', color='#c0392b', lw=2, alpha=0.8)
    # El teu estimador en blau
    ax.plot(t, my_f0, label='El meu estimador (C++)', color='#1a6fba', lw=1.5)
    
    ax.set_xlabel('Temps (s)')
    ax.set_ylabel('f0 (Hz)')
    ax.set_title('Comparació de Pitch (f0) real — pav_2042.wav')
    
    # Rang de freqüència adaptat exactament de 50 a 500 Hz
    ax.set_ylim(50, 500) 
    ax.legend()
    plt.tight_layout()
    
    # Creem la carpeta img si no existeix i guardem el gràfic net per a la memòria
    os.makedirs('img', exist_ok=True)
    plt.savefig('img/comparacio_pitch.png', dpi=150, bbox_inches='tight', facecolor='white')
    print("Figura guardada correctament a 'img/comparacio_pitch.png'")
    
    plt.show()
