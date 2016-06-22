#include "matrix.h"
#include "field2.h"
#include <math.h>
#include <vector>
#include <algorithm>

using namespace std;

//make this more memory efficient
//consider changing it to "grid.add_pml(thickness)"
//Perhaphs allow pml of varying thickness on each side.

namespace qbox {

    double compute_pml_val(double d, int thickness, double alpha) {
        return alpha*pow((d+1)/thickness,3);
    }

    pml::pml(grid_properties grid): Nx(grid.Nx), Ny(grid.Ny), thickness(grid.pml_thickness) {
        thickness -= 2;

        Ihx = matrix<double>(Nx, Ny); 
        Ihy = matrix<double>(Nx, Ny);

        fi1 = make_unique<double[]>(Nx);
        fi2 = make_unique<double[]>(Nx);
        fi3 = make_unique<double[]>(Nx);
        gi2 = make_unique<double[]>(Nx);
        gi3 = make_unique<double[]>(Nx);

        fj1 = make_unique<double[]>(Ny);
        fj2 = make_unique<double[]>(Ny);
        fj3 = make_unique<double[]>(Ny);
        gj2 = make_unique<double[]>(Ny);
        gj3 = make_unique<double[]>(Ny);

        for (int i = 0; i != Nx; i++) {
            for (int j = 0; j != Ny; j++) {
                Ihx[i][j] = 0;
                Ihy[i][j] = 0;
            }
        }

        double xn1;
        double xn2;
        for (int i = 0; i != Nx; i++) {
            int D1 = thickness - i;
            int D2 = thickness - (Nx-1-i);
            int D = max(D1,D2);
            if (D < -1) {
                xn1 = 0;
                xn2 = 0;
            }
            else {
                xn1 = compute_pml_val(D,thickness,0.333);
                xn2 = compute_pml_val(D+0.5,thickness,0.25);
            }

            fi1[i] = xn2;
            fi2[i] = 1/(1+xn2);
            fi3[i] = (1-xn2)/(1+xn2);
            gi2[i] = 1/(1+xn1);
            gi3[i] = (1-xn1)/(1+xn1);
        }

        for (int j = 0; j != Ny; j++) {
            int D1 = thickness - j;
            int D2 = thickness - (Ny-1-j);
            int D = max(D1,D2);
            if (D < 0) {
                xn1 = 0;
                xn2 = 0;
            }
            else {
                xn1 = compute_pml_val(D,thickness,0.333);
                xn2 = compute_pml_val(D+0.5,thickness,0.25);
            }

            fj1[j] = xn2;
            fj2[j] = 1/(1+xn2);
            fj3[j] = (1-xn2)/(1+xn2);
            gj2[j] = 1/(1+xn1);
            gj3[j] = (1-xn1)/(1+xn1);
        }
        thickness += 2;
    }
}