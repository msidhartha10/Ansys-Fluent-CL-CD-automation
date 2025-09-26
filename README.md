# 🛩️ ANSYS Fluent AoA Sweep (UDF + Journal Automation)

This repository demonstrates how to automate **angle-of-attack (AoA) sweeps** in ANSYS Fluent
using a **User Defined Function (UDF)** and a **journal file**.

Instead of manually editing inlet velocity components and force directions for each angle,
this workflow lets you sweep AoA values, compute **Cl** and **Cd**, and save results automatically.

---

## 🔹 Features
- Automates **AoA sweep** (–5° → +10° in 2.5° steps).
- Inlet velocity set by UDF (reads `aoa.txt`).
- Computes global forces (`Fx`, `Fy`), then converts to:
  - Drag (`Fd`)
  - Lift (`Fl`)
  - Coefficients (`Cd`, `Cl`)
- Results appended to `aoa_results.txt` (tab-separated).
- Runs in **GUI** or **batch CLI mode**.
- Easy to extend for different ranges, fluids, or geometries.

---

## 📂 Repository Structure
```
.
├── aoa_udf.c          # UDF source code (compiled in Fluent)
├── aoa_sweep.jou      # Journal file to automate AoA sweep
├── aoa.txt            # Holds current AoA value (overwritten by journal)
├── aoa_results.txt    # Output results (generated after sweep)
└── README.md          # This documentation
```

---

## 🟦 Flowchart / Workflow

```text
Start
  |
  v
Prepare working folder
  • Put case + mesh files
  • Place aoa_udf.c, aoa.txt, aoa_sweep.jou
  |
  v
Launch Fluent → Load case/mesh
  |
  v
Compile UDF
  • Define → User-Defined → Functions → Compiled
  • Add aoa_udf.c → Build → Load
  |
  v
Hook velocity profiles
  • Inlet BC → Velocity Components
  • X = inlet_U_profile
  • Y = inlet_V_profile
  |
  v
Set reference values
  • Area = 0.4 m²
  • Length = 0.435 m
  • Velocity = 16 m/s
  |
  v
Run journal (aoa_sweep.jou)
  • File → Read → Journal (GUI)
     OR
  • fluent 2ddp -g -i aoa_sweep.jou (CLI)
  |
  v
Journal loop:
  • Update aoa.txt with new angle
  • Iterate solver
  • Call compute_forces_and_write UDF
  • Append results to aoa_results.txt
  |
  v
Check aoa_results.txt
  • Contains AoA, Fx, Fy, Fd, Fl, Cd, Cl
  |
  v
Plot CL vs AoA, CD vs AoA in MATLAB/Excel
  |
  v
End
```

---

## 🔹 Setup Instructions

### 1. Prepare Case
1. Create a Fluent case/mesh with your airfoil/wing.
2. Make sure the wing surface is a separate **face zone**.

### 2. Edit the UDF (`aoa_udf.c`)
Update constants as needed:
```c
#define SURFACE_ZONE_ID  7      /* Airfoil surface zone ID */
#define UINF             16.0   /* Freestream velocity [m/s] */
#define RHO              1.225  /* Fluid density [kg/m³] */
#define AREF             0.4    /* Reference area [m²] */
#define LREF             0.435  /* Reference length [m] */
```

👉 Find **zone ID** in Fluent:
- GUI → *Boundary Conditions* → select surface → ID shown in console.
- Or TUI: `/define/boundary-conditions/list-zones`.

### 3. Place Files
Put into Fluent working dir:
- `aoa_udf.c`
- `aoa_sweep.jou`
- `aoa.txt` (initial AoA, e.g. `-5.0`)

Check dir with:
- Windows: `!cd`
- Linux: `!pwd`

### 4. Compile UDF
In Fluent:
- **Define → User-Defined → Functions → Compiled**
- **Add** → `aoa_udf.c`
- **Build**, then **Load**
- Functions available: `inlet_U_profile`, `inlet_V_profile`, `compute_forces_and_write`

### 5. Hook Velocity Profiles
- Inlet boundary condition:
  - Velocity Specification = **Components**
  - X-component → `inlet_U_profile`
  - Y-component → `inlet_V_profile`

### 6. Set Reference Values
- **Report → Reference Values**:
  - Area = `0.4`
  - Length = `0.435`
  - Velocity = `16`
  - Density = `1.225`

---

## 🔹 Running the Sweep

### Option A: GUI
- **File → Read → Journal**
- Select `aoa_sweep.jou`

### Option B: CLI (batch)
```bash
fluent 2ddp -g -i aoa_sweep.jou
```
- `2ddp` → 2D, double precision (use `3ddp` for 3D)
- `-g` → no GUI
- `-i` → input journal

---

## 🔹 Output

Results saved to `aoa_results.txt`.  
Each line = one AoA run:

```
AoA   Fx   Fy   Fd   Fl   Cd   Cl
-5.0  ...  ...  ...  ...  ...  ...
-2.5  ...  ...  ...  ...  ...  ...
 0.0  ...  ...  ...  ...  ...  ...
 ...
10.0  ...  ...  ...  ...  ...  ...
```

---

## 📊 Post-Processing

### MATLAB
```matlab
data = readmatrix('aoa_results.txt');
aoa = data(:,1);  Cl = data(:,7);  Cd = data(:,6);
plot(aoa,Cl,'-o'); hold on; plot(aoa,Cd,'-s');
xlabel('AoA (deg)'); ylabel('Coefficient'); legend('Cl','Cd'); grid on;
```

### Python
```python
import pandas as pd, matplotlib.pyplot as plt
df = pd.read_csv("aoa_results.txt", sep="\t", names=["AoA","Fx","Fy","Fd","Fl","Cd","Cl"])
plt.plot(df["AoA"], df["Cl"], "-o", label="Cl")
plt.plot(df["AoA"], df["Cd"], "-s", label="Cd")
plt.xlabel("AoA (deg)"); plt.ylabel("Coefficient"); plt.grid(True); plt.legend(); plt.show()
```

---

## 🔹 Notes & Tips
- Works best in **serial mode**; in parallel, only host writes `aoa_results.txt`.
- Ensure `SURFACE_ZONE_ID` matches your geometry.
- If density differs from `1.225`, update UDF.
- To change AoA range/step → edit `aoa_sweep.jou`.
- If `aoa_results.txt` already exists, results are **appended**.

---

## 📌 Extensions
- Add **Cm (moment coefficient)** calculation.
- Search surface by **name** instead of ID.
- Integrate into **Workbench parametric sweep**.
- Make results auto-plot after each AoA.

---

## 👨‍💻 Author Notes
This setup was built for student projects to automate **airfoil lift/drag extraction** in Fluent.
Sharing here so others can save time. Contributions welcome!
