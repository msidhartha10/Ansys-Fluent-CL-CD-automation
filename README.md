# 🛩️ ANSYS Fluent AoA Sweep — UDF + Journal Automation

Automates angle-of-attack (AoA) sweeps in ANSYS Fluent using a compiled User-Defined Function (UDF) and a journal file. Run a series of AoA cases automatically, compute forces, convert to lift/drag, and save Cl/Cd to a results file for quick post-processing.

---

## Key features
- Automates AoA sweep (default: -5° → +10° in 2.5° steps).
- UDF provides velocity components (reads AoA from `aoa.txt`) and computes forces.
- Computes Fx, Fy → Drag (Fd), Lift (Fl) → coefficients Cd, Cl.
- Appends results to `aoa_results.txt` (tab-separated).
- Works in GUI or batch (CLI) Fluent modes.
- Easy to adapt to other AoA ranges, fluids, or geometries.

---

## Quick start

1. Place these files in your Fluent working directory:
   - `aoa_udf.c` — compiled in Fluent
   - `aoa_sweep.jou` — journal file that runs the sweep
   - `aoa.txt` — contains the current AoA value (example: `-5.0`)

2. Prepare your Fluent case/mesh:
   - Ensure the airfoil/wing surface is a separate face zone.
   - Note the SURFACE_ZONE_ID for the airfoil (Fluent GUI or TUI: `/define/boundary-conditions/list-zones`).

3. Edit `aoa_udf.c` constants as needed:
```c
#define SURFACE_ZONE_ID  7      /* Airfoil surface zone ID */
#define UINF             16.0   /* Freestream velocity [m/s] */
#define RHO              1.225  /* Fluid density [kg/m³] */
#define AREF             0.4    /* Reference area [m²] */
#define LREF             0.435  /* Reference length [m] */
```

4. Compile the UDF in Fluent:
   - Define → User-Defined → Functions → Compiled → Add `aoa_udf.c` → Build → Load
   - Exposes UDF functions: `inlet_U_profile`, `inlet_V_profile`, `compute_forces_and_write`

5. Hook velocity UDFs to the inlet boundary:
   - Velocity specification → Components
   - X-component: `inlet_U_profile`
   - Y-component: `inlet_V_profile`

6. Set reference values in Fluent (Report → Reference Values):
   - Area = 0.4, Length = 0.435, Velocity = 16, Density = 1.225

---

## Running the sweep

Option A — GUI
- File → Read → Journal → select `aoa_sweep.jou`

Option B — CLI (batch)
```bash
fluent 2ddp -g -i aoa_sweep.jou
```
- `2ddp` = 2D double precision (use `3ddp` for 3D)
- `-g` = no GUI, `-i` = input journal

The journal overwrites `aoa.txt` for each AoA, runs the solver/iterations, triggers `compute_forces_and_write`, and appends results to `aoa_results.txt`.

---

## Output format

`aoa_results.txt` contains one line per AoA run. Example columns:
AoA, Fx, Fy, Fd, Fl, Cd, Cl

Example:
```
AoA   Fx   Fy   Fd   Fl   Cd   Cl
-5.0  ...  ...  ...  ...  ...  ...
-2.5  ...  ...  ...  ...  ...  ...
 0.0  ...  ...  ...  ...  ...  ...
 ...
10.0  ...  ...  ...  ...  ...  ...
```

---

## Post-processing examples

MATLAB:
```matlab
data = readmatrix('aoa_results.txt');
aoa = data(:,1);  Cl = data(:,7);  Cd = data(:,6);
plot(aoa,Cl,'-o'); hold on; plot(aoa,Cd,'-s');
xlabel('AoA (deg)'); ylabel('Coefficient'); legend('Cl','Cd'); grid on;
```

Python:
```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("aoa_results.txt", sep="\t", names=["AoA","Fx","Fy","Fd","Fl","Cd","Cl"])
plt.plot(df["AoA"], df["Cl"], "-o", label="Cl")
plt.plot(df["AoA"], df["Cd"], "-s", label="Cd")
plt.xlabel("AoA (deg)"); plt.ylabel("Coefficient"); plt.grid(True); plt.legend(); plt.show()
```

---

## Notes & tips
- Best run in serial mode; in parallel only the host writes `aoa_results.txt`.
- Ensure `SURFACE_ZONE_ID` matches your geometry.
- If density ≠ 1.225 kg/m³, update the UDF constant.
- To change AoA range/step, edit `aoa_sweep.jou`.
- If `aoa_results.txt` exists, results are appended — delete it before a fresh sweep if desired.

---

## Suggested extensions
- Add moment coefficient (Cm) calculation.
- Search surface by name instead of a hard-coded zone ID.
- Integrate with Workbench parametric sweeps.
- Auto-plot results after the sweep completes.

---

## Files in this repo
- aoa_udf.c          — UDF source (compile in Fluent)
- aoa_sweep.jou      — Journal file to run the sweep
- aoa.txt            — Current AoA value (overwritten by the journal)
- aoa_results.txt    — Output (generated/appended by runs)
- README.md          — This documentation

---

## Author
Made for student projects to speed up airfoil lift/drag extraction in Fluent. Contributions and improvements are welcome — please open issues or PRs.