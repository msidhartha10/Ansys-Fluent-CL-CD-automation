

```markdown
# AoA Sweep in ANSYS Fluent with UDF

This project automates **angle-of-attack (AoA) sweeps** for a 2D wing/airfoil CFD case in ANSYS Fluent.  
Instead of manually changing inlet velocity components and force axes for every AoA, this setup uses:

- A **User-Defined Function (UDF)** to set inlet velocity from a single `aoa.txt` file.
- An **On-Demand UDF** to compute aerodynamic forces and coefficients (`Cl`, `Cd`).
- A **Fluent Journal** to loop over AoAs (`-5°` → `10°` in steps of `2.5°`) and save results automatically.

This lets you run **batch sweeps of lift/drag vs. AoA** with minimal manual input.

---

## ✈️ Features
- Automates AoA sweeps for CFD cases.
- Computes and logs:
  - Global forces (`Fx`, `Fy`)
  - Drag (`Fd`)
  - Lift (`Fl`)
  - Coefficients (`Cd`, `Cl`)
- Works with **steady** or **unsteady** solvers.
- Saves results to `aoa_results.txt` (tab-separated).
- Easy to modify for other ranges of AoA, fluid properties, or reference values.

---

## 📂 Repository Structure
```

.
├── aoa_udf.c          # Main UDF (compiled in Fluent)
├── aoa.txt            # Current AoA value (deg) - read by UDF
├── aoa_sweep.jou      # Fluent journal to loop AoAs
├── aoa_results.txt    # Output file (generated after runs)
└── README.md          # This documentation

````

---

## ⚙️ Setup Instructions

### 1. Prepare Fluent Case
1. Create your CFD case/mesh in Fluent (2D or 3D).
2. Ensure your **wing/airfoil surface** is a separate *face zone*.

### 2. Edit the UDF
Open `aoa_udf.c` and set:
```c
#define SURFACE_ZONE_ID  7      /* <-- Replace with your wing surface ID */
#define UINF             16.0   /* Freestream speed [m/s] */
#define RHO              1.225  /* Fluid density [kg/m^3] */
#define AREF             0.4    /* Reference area [m^2] */
#define LREF             0.435  /* Reference length [m] */
````

👉 To find your **surface zone ID**:

* Fluent GUI → *Boundary Conditions* → select wing/airfoil surface (zone ID shows in console).
* Or use TUI: `/define/boundary-conditions/list-zones`.

### 3. Place Files

Put the following in your Fluent **working directory**:

* `aoa_udf.c`
* `aoa.txt` (initial AoA value, e.g. `-5.0`)
* `aoa_sweep.jou`

### 4. Compile & Hook UDF

1. Fluent menu: **Define → User-Defined → Functions → Compiled**

   * **Add** `aoa_udf.c`
   * **Build**, then **Load**
2. Hook UDFs:

   * Inlet boundary → *Velocity Specification = Components*
   * **X-velocity** → `inlet_U_profile`
   * **Y-velocity** → `inlet_V_profile`

### 5. Set Reference Values

**Report → Reference Values**:

* Area = `0.4`
* Length = `0.435`
* Velocity = `16.0`
* Density = `1.225` (or match your fluid)

---

## 🚀 Running the AoA Sweep

### Automatic (recommended)

Run the journal to sweep AoA from `-5°` to `10°` in steps of `2.5°`:

```
/file/read-journal aoa_sweep.jou
```

This will:

* Update `aoa.txt`
* Run solver (500 iterations per angle — adjust in journal)
* Call UDF to compute and save `Cl`, `Cd`

Results are written to:

```
aoa_results.txt
```

Example line:

```
AoA    Fx       Fy       Fd       Fl       Cd       Cl
-5.0   2.34e-1  ...     ...      ...     0.034    -0.21
```

### Manual (single AoA)

1. Edit `aoa.txt` with desired AoA.
2. Reinitialize or continue solution.
3. Run solver iterations.
4. Execute UDF manually:

   * **Define → User-Defined → Execute On Demand → compute_forces_and_write**
5. Check `aoa_results.txt`.

---

## 📊 Post-Processing

You can import `aoa_results.txt` into:

* **Excel**
* **MATLAB** (`readmatrix('aoa_results.txt')`)
* **Python/Pandas**

Example (Python/Matplotlib):

```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("aoa_results.txt", sep="\t", names=["AoA","Fx","Fy","Fd","Fl","Cd","Cl"])
plt.plot(df["AoA"], df["Cl"], "-o", label="Cl")
plt.plot(df["AoA"], df["Cd"], "-s", label="Cd")
plt.xlabel("Angle of Attack (deg)")
plt.ylabel("Coefficient")
plt.legend()
plt.grid(True)
plt.show()
```

---

## ⚠️ Notes

* Works in **serial** mode by default. In parallel, only the host writes output files.
* Ensure `aoa.txt` and `aoa_results.txt` are in Fluent’s working dir (`!pwd` / `!cd` to check).
* If you change AoA range, edit `aoa_sweep.jou`.
* If `Compute_Force_And_Moment` errors, check `SURFACE_ZONE_ID`.

---

## 📌 To-Do / Possible Extensions

* Allow UDF to read **wing zone by name** instead of ID.
* Add **moment coefficient (Cm)** calculation.
* Automate sweeps via **Workbench Design Points**.
* Parallel-safe output handling.

---

## 👨‍💻 Author Notes

Originally built for a **student project** to automate lift/drag coefficient extraction for a wing in Fluent.
Shared here so others don’t have to waste time manually setting velocity components and lift/drag directions.

Contributions welcome!

```

---

👉 Do you also want me to add **screenshots/GIFs of Fluent menus** in the README (compile UDF, assign boundary, run journal), or keep it text-only?
```
