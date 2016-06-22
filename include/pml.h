#ifndef GUARD_pml_h
#define GUARD_pml_h

#include "matrix.h"
#include <memory>

namespace qbox {
    class grid_properties;

    //PML boundary object
    class pml {
    public:
        //*** This class needs rule of 3 (or Zero)
        pml(grid_properties grid);
        pml() = default;

    public:
        //*** Smart Pointers
        //PML array and matrix properties
        std::unique_ptr<double[]> fi1,fi2,fi3,fj1,fj2,fj3;
        std::unique_ptr<double[]> gi2,gi3,gj2,gj3;
        matrix<double> Ihx, Ihy;

    private:
        //PML dimensions
        int Nx, Ny;      //Nx, Ny of boundary_parameters
        int thickness;   //# of cells for thickness
    };
}

#endif