/* wing_3d_forces_moments.c
   3-D tapered wing, axes:
     x: chordwise (freestream nominally along +x at 0 AoA)
     y: vertical (up)
     z: span (+z = left wing, -z = right wing)

   Conventions:
     - Drag  + along freestream direction in x–y plane.
     - Lift  + perpendicular to freestream in x–y plane, pointing "up".
     - Side  + toward +z (left wing).
     - Roll  = Mx (about +x)
     - Yaw   = My (about +y)
     - Pitch = Mz (about +z)

   What it does:
     - Reads AoA (deg) from "aoa.txt" (rotation about +z).
     - Provides inlet U (x) & V (y) components.
     - Integrates pressure+viscous forces over one or more wall zones.
     - Decomposes to D/L/Y and reports Mx/My/Mz.
     - Computes Cd, Cl, Cy; Cmx (roll), Cmy (yaw), Cmz (pitch) with correct refs.

   Edit the USER block: zone IDs, freestream, rho, S_REF, B_REF (span), C_REF (MAC), and CG.
*/

#include "udf.h"
#include <stdio.h>
#include <math.h>

/* ===================== USER: EDIT HERE ===================== */
static const int WALL_ZONE_IDS[] = { 5 /*, add more IDs if the wing surface is split */ };
static const int N_WALL_ZONES    = sizeof(WALL_ZONE_IDS)/sizeof(WALL_ZONE_IDS[0]);

#define UINF     16.0      /* m/s */
#define RHO      1.225     /* kg/m^3 */

/* Aerodynamic references (3-D aircraft) */
#define S_REF    0.740     /* m^2  (wing reference area) */
#define B_REF    0.530     /* m    (wing span, tip-to-tip) */
#define C_REF    0.265     /* m    (mean aerodynamic chord) */

/* Moment center (about which Mx/My/Mz are computed): set your actual CG! */
#define X_CG     0.06625   /* m  e.g., quarter-chord @ root: adjust to your CG */
#define Y_CG     0.00000   /* m  typically 0 if origin is at LE plane */
#define Z_CG     0.00000   /* m  midspan = 0 for symmetric wing */
/* =========================================================== */

#define AOA_FILE "aoa.txt"
#define OUT_FILE "aoa_results.txt"
#define PI 3.14159265358979323846

static real aoa_deg = 0.0;

static void read_aoa_from_file(void)
{
    FILE *fp = fopen(AOA_FILE, "r");
    if (fp) {
        double tmp;
        if (fscanf(fp, "%lf", &tmp) == 1) aoa_deg = (real)tmp;
        fclose(fp);
    }
}

/* -------- Inlet U (x) component -------- */
DEFINE_PROFILE(inlet_U_profile, thread, index)
{
    face_t f;
    read_aoa_from_file();
    const real a = aoa_deg * PI/180.0;
    const real Ux = UINF * cos(a);   /* AoA is rotation about +z: V = UINF*sin(a) */

    begin_f_loop(f, thread)
        F_PROFILE(f, thread, index) = Ux;
    end_f_loop(f, thread);
}

/* -------- Inlet V (y) component -------- */
DEFINE_PROFILE(inlet_V_profile, thread, index)
{
    face_t f;
    read_aoa_from_file();
    const real a = aoa_deg * PI/180.0;
    const real Vy = UINF * sin(a);

    begin_f_loop(f, thread)
        F_PROFILE(f, thread, index) = Vy;
    end_f_loop(f, thread);
}

/* -------- On-demand: integrate & report -------- */
DEFINE_ON_DEMAND(wing_forces_moments_write)
{
    Domain *d = Get_Domain(1);

    read_aoa_from_file();
    const real a = aoa_deg * PI/180.0;

    /* Wind-axis unit vectors in your coordinates:
       exw: along freestream in x–y plane
       eyw: lift direction (perp. to exw in x–y, +up)
       ezw: span (same as +z, toward left wing)
    */
    const real exw[3] = {  cos(a),  sin(a), 0.0 };   /* drag direction */
    const real eyw[3] = { -sin(a),  cos(a), 0.0 };   /* lift direction (+up) */
    const real ezw[3] = {  0.0,     0.0,    1.0 };   /* side (+left wing) */

    real xref[3] = { X_CG, Y_CG, Z_CG };

    real Ftot[3] = {0,0,0};
    real Mtot[3] = {0,0,0};

    for (int i = 0; i < N_WALL_ZONES; ++i)
    {
        Thread *t = Lookup_Thread(d, WALL_ZONE_IDS[i]);
        if (!t) {
            Message("WARN: zone id %d not found; skipping.\n", WALL_ZONE_IDS[i]);
            continue;
        }

        real F[3] = {0,0,0}, M[3] = {0,0,0};
        /* include viscous = TRUE */
        Compute_Force_And_Moment(d, t, xref, F, M, TRUE);

        Ftot[0] += F[0]; Ftot[1] += F[1]; Ftot[2] += F[2];
        Mtot[0] += M[0]; Mtot[1] += M[1]; Mtot[2] += M[2];
    }

    /* Decompose forces in wind axes */
    const real Fx = Ftot[0], Fy = Ftot[1], Fz = Ftot[2];

    const real Fd =  Fx*exw[0] + Fy*exw[1] + Fz*exw[2];  /* + along freestream */
    const real Fl =  Fx*eyw[0] + Fy*eyw[1] + Fz*eyw[2];  /* + up in x–y plane */
    const real Fyaw = Fx*ezw[0] + Fy*ezw[1] + Fz*ezw[2]; /* side force +left wing (= +z) */

    /* Moments about CG in your global axes */
    const real Mx = Mtot[0];   /* Roll about +x */
    const real My = Mtot[1];   /* Yaw  about +y (vertical) */
    const real Mz = Mtot[2];   /* Pitch about +z (span) */

    /* Nondimensional coefficients (3-D):
       - Cd, Cl, Cy with S_REF
       - Cmx (roll) with S_REF * B_REF
       - Cmy (yaw)  with S_REF * B_REF
       - Cmz (pitch) with S_REF * C_REF
    */
    const real qinf = 0.5 * RHO * UINF * UINF;

    real Cd=0, Cl=0, Cy=0, Cmx=0, Cmy=0, Cmz=0;
    if (qinf > 0.0 && S_REF > 0.0) {
        Cd  = Fd / (qinf * S_REF);
        Cl  = Fl / (qinf * S_REF);
        Cy  = Fyaw / (qinf * S_REF);

        if (B_REF > 0.0) {
            Cmx = Mx / (qinf * S_REF * B_REF);  /* roll coeff */
            Cmy = My / (qinf * S_REF * B_REF);  /* yaw coeff  */
        }
        if (C_REF > 0.0) {
            Cmz = Mz / (qinf * S_REF * C_REF);  /* pitch coeff */
        }
    }

    /* Append CSV (header on first write) */
    int file_exists = 0;
    {
        FILE *ft = fopen(OUT_FILE, "r");
        if (ft) { file_exists = 1; fclose(ft); }
    }
    FILE *fp = fopen(OUT_FILE, "a");
    if (fp) {
        if (!file_exists) {
            fprintf(fp, "AoA_deg,Fx[N],Fy[N],Fz[N],Fd[N],Fl[N],Fyaw[N],Cd,Cl,Cy,"
                        "Mx[Nm],My[Nm],Mz[Nm],Cmx,Cmy,Cmz\n");
        }
        fprintf(fp, "%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",
            (double)aoa_deg,
            (double)Fx, (double)Fy, (double)Fz,
            (double)Fd, (double)Fl, (double)Fyaw,
            (double)Cd, (double)Cl, (double)Cy,
            (double)Mx, (double)My, (double)Mz,
            (double)Cmx, (double)Cmy, (double)Cmz);
        fclose(fp);
    }

    Message("AoA %g deg | D=%g L=%g Y=%g | Cd=%g Cl=%g Cy=%g | "
            "Roll Mx=%g (Cmx=%g) | Yaw My=%g (Cmy=%g) | Pitch Mz=%g (Cmz=%g)\n",
            aoa_deg, Fd, Fl, Fyaw, Cd, Cl, Cy, Mx, Cmx, My, Cmy, Mz, Cmz);
}
