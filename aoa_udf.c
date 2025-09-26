/* aoa_udf.c
   - Inlet velocity components (U,V) are provided by two DEFINE_PROFILE functions.
     They read the current AoA (deg) from "aoa.txt" in Fluent's working directory.
   - A DEFINE_ON_DEMAND function computes forces on the airfoil surface (zone id
     must be edited below) using Compute_Force_And_Moment, computes Fd & Fl,
     then computes Cd and Cl and appends a line to aoa_results.txt.

   Edit these macros before compiling:
     - SURFACE_ZONE_ID   : set to the face-zone ID of your airfoil/wing surface
     - UINF              : freestream speed (m/s) = 16 as you requested
     - RHO               : fluid density (kg/m3) — change if your case uses different
     - AREF              : reference area (m^2) = 0.4 (you specified)
     - LREF              : reference length (m) = 0.435 (you specified) [not used in CD/CL calc here but kept for record]
*/

#include "udf.h"
#include <stdio.h>
#include <math.h>

/* --- USER EDITS --- */
#define SURFACE_ZONE_ID  5      /* <<< CHANGE this to your airfoil face-zone ID */
#define UINF             16.0   /* m/s (given) */
#define RHO              1.225  /* kg/m3 - change if your material density differs */
#define AREF             0.4    /* m^2 (given) */
#define LREF             0.435  /* m (given) */
/* ------------------- */

#define PI 3.14159265358979323846

/* store latest AoA (deg) in a static variable so both functions share it */
static real aoa_deg = 0.0;

/* read a single numeric AoA (deg) from aoa.txt (first token). Silent if file missing. */
static void read_aoa_from_file(void)
{
    FILE *fp = fopen("aoa.txt", "r");
    if (fp)
    {
        double tmp;
        if (fscanf(fp, "%lf", &tmp) == 1)
            aoa_deg = (real) tmp;
        fclose(fp);
    }
    else
    {
        /* file not found: keep previous aoa_deg (or 0.0 on first run) */
        /* If you want a warning uncomment next line */
        /* Message("aoa_udf: aoa.txt not found in working dir\n"); */
    }
}

/* ----- inlet U component profile (X-direction) ----- */
DEFINE_PROFILE(inlet_U_profile, thread, index)
{
    face_t f;
    real ux;
    read_aoa_from_file();
    real a_rad = aoa_deg * PI / 180.0;
    ux = UINF * cos(a_rad);

    begin_f_loop(f, thread)
    {
        F_PROFILE(f, thread, index) = ux;
    }
    end_f_loop(f, thread);
}

/* ----- inlet V component profile (Y-direction) ----- */
DEFINE_PROFILE(inlet_V_profile, thread, index)
{
    face_t f;
    real uy;
    read_aoa_from_file();
    real a_rad = aoa_deg * PI / 180.0;
    uy = UINF * sin(a_rad);

    begin_f_loop(f, thread)
    {
        F_PROFILE(f, thread, index) = uy;
    }
    end_f_loop(f, thread);
}

/* --- On-demand postprocess: forces + moments + coeffs --- */
DEFINE_ON_DEMAND(compute_forces_and_write)
{
    Domain *d = Get_Domain(1);
    Thread *t_airfoil;
    real cg[ND_ND];         /* reference point (aero center) */
    real force[ND_ND], moment[ND_ND];
    real Fx, Fy, Fz;
    real Mx, My, Mz;
    real a_rad, qinf;
    real Fd, Fl, Cd, Cl;
    real Cmx, Cmy, Cmz;     /* roll, pitch, yaw moment coefficients */
    FILE *fp;

    /* === Step 1: ensure AoA is up to date === */
    read_aoa_from_file();
    a_rad = aoa_deg * PI / 180.0;

    /* === Step 2: locate surface thread === */
    t_airfoil = Lookup_Thread(d, SURFACE_ZONE_ID);
    if (t_airfoil == NULL)
    {
        Message("aoa_udf: ERROR - zone id %d not found. Edit SURFACE_ZONE_ID.\n", SURFACE_ZONE_ID);
        return;
    }

    /* === Step 3: set aerodynamic center (reference point for moments) === */
    cg[0] = 0.25 * LREF;   /* x = quarter-chord */
    cg[1] = 0.0;           /* y = mid-span (centerline) */
    cg[2] = 0.0;           /* z = 0 plane */

    /* === Step 4: compute force & moment === */
    Compute_Force_And_Moment(d, t_airfoil, cg, force, moment, TRUE);

    Fx = force[0];  /* x force */
    Fy = force[1];  /* y force */
    Fz = force[2];  /* z force (for 3D case) */

    Mx = moment[0]; /* roll moment */
    My = moment[1]; /* pitch moment */
    Mz = moment[2]; /* yaw moment */

    /* === Step 5: Lift/Drag decomposition === */
    Fd = Fx * cos(a_rad) + Fy * sin(a_rad);
    Fl = -Fx * sin(a_rad) + Fy * cos(a_rad);

    /* === Step 6: dynamic pressure === */
    qinf = 0.5 * RHO * UINF * UINF;

    /* === Step 7: Coefficients === */
    if (qinf * AREF != 0.0)
    {
        Cd = Fd / (qinf * AREF);
        Cl = Fl / (qinf * AREF);

        Cmx = Mx / (qinf * AREF * LREF);  /* roll coeff */
        Cmy = My / (qinf * AREF * LREF);  /* pitch coeff */
        Cmz = Mz / (qinf * AREF * LREF);  /* yaw coeff */
    }
    else
    {
        Cd = Cl = 0.0;
        Cmx = Cmy = Cmz = 0.0;
    }

    /* === Step 8: write to file === */
    fp = fopen("aoa_results.txt", "a");
    if (fp != NULL)
    {
        static int header_written = 0;
        if (!header_written)
        {
            fprintf(fp,
                "AoA_deg\tFx[N]\tFy[N]\tFz[N]\tFd[N]\tFl[N]\tCd\tCl\t"
                "Mx[Nm]\tMy[Nm]\tMz[Nm]\tCmx\tCmy\tCmz\n");
            header_written = 1;
        }

        fprintf(fp,
            "%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t"
            "%g\t%g\t%g\t%g\t%g\t%g\n",
            (double)aoa_deg,
            (double)Fx, (double)Fy, (double)Fz,
            (double)Fd, (double)Fl, (double)Cd, (double)Cl,
            (double)Mx, (double)My, (double)Mz,
            (double)Cmx, (double)Cmy, (double)Cmz);

        fclose(fp);
    }

    Message("AoA %g deg: Fx=%g Fy=%g Fz=%g Fd=%g Fl=%g Cd=%g Cl=%g | Mx=%g My=%g Mz=%g Cmx=%g Cmy=%g Cmz=%g\n",
            aoa_deg, Fx, Fy, Fz, Fd, Fl, Cd, Cl, Mx, My, Mz, Cmx, Cmy, Cmz);
}
