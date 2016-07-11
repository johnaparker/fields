#include <math.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <vector>
#include <string>
#include "field2.h"
#include "field.h"
#include "matrix.h"
#include "objects/object.h"
#include "tfsf.h"
#include "termcolor.h"

using namespace std;

namespace qbox {

    Field2D::Field2D(grid_properties grid, string filename): grid(grid), Nx(grid.Nx), Ny(grid.Ny), dx(grid.dx), Lx(grid.Lx), Ly(grid.Ly) {
        t = 1;
        tStep = 0;
        dt = dx/2.0;
        Ez = matrix<double>(Nx, Ny); 
        Hx = matrix<double>(Nx, Ny);
        Hy = matrix<double>(Nx, Ny);
        Dz = matrix<double>(Nx, Ny);
        Iz = matrix<double>(Nx, Ny);
        ca = matrix<double>(Nx, Ny);
        cb = matrix<double>(Nx, Ny);

        obj  = matrix<object*>(Nx, Ny); 
        background = make_unique<medium>();
        obj_list = {background.get()};
        
        for (int i = 0; i != Nx; i++) {
            for (int j = 0; j != Ny; j++) {
                Ez[i][j] = 0;
                Hx[i][j] = 0;
                Hy[i][j] = 0;
                Dz[i][j] = 0;
                Iz[i][j] = 0;
                double eps = obj_list[0]->eps;      
                double conduc = obj_list[0]->conduc; 
                ca[i][j] = 1/(eps + conduc*dt/epsilon);
                cb[i][j] = conduc*dt/epsilon;
                obj[i][j] = obj_list[0];
             }
        }

        BC = make_unique<pml>(grid); 

        if (grid.totalFieldScatteredField) 
            total = make_unique<tfsf>(grid, dt);
        else
            total = nullptr;
        field_components = {{"Ez", &Ez}, {"Hx", &Hx}, {"Hy", &Hy}};

        if (!filename.empty())
            outFile = make_unique<h5cpp::h5file>(filename, h5cpp::io::w);

        display_info();
    }

    void Field2D::write_field(const fields field) {
        clocks.start(clock_name::hdf5);

        unique_ptr<h5cpp::h5group> gFields;
        if (!outFile->object_exists("Fields"))
            gFields = outFile->create_group("Fields");
        else
            gFields = outFile->open_group("Fields");


        unique_ptr<h5cpp::h5dset> dset;
        switch(field) {
            case fields::Ez: if (!outFile->object_exists("Fields/Ez")) {
                    vector<hsize_t> dims = {hsize_t(Nx),hsize_t(Ny),1};
                    vector<hsize_t> max_dims = {hsize_t(Nx),hsize_t(Ny),h5cpp::inf};
                    vector<hsize_t> chunk_dims = dims;
                    h5cpp::dataspace ds(dims, max_dims, chunk_dims, false);
                    dset = gFields->create_dataset("Ez", 
                                 h5cpp::dtype::Double, ds); 
                    dset->write(Ez.data());

                    h5cpp::dataspace ds_a(vector<hsize_t>{1});
                    auto attr = dset->create_attribute("dx", h5cpp::dtype::Double, ds_a);
                    attr->write(&dx);
                    attr = dset->create_attribute("dt", h5cpp::dtype::Double, ds_a);
                    attr->write(&dt);

                 }
                 else {
                     dset = gFields->open_dataset("Ez");
                     dset->append(Ez.data());
                 }
                 break;
        }
        clocks.stop(clock_name::hdf5);
    }

    void Field2D::writeE() {
        write_field(fields::Ez);
    }

    void Field2D::writeH() {
        write_field(fields::Hx);
        write_field(fields::Hy);
    }

    void Field2D::write_monitor(string name, double* data, int N, bool extendable) {
        clocks.start(clock_name::hdf5);

        unique_ptr<h5cpp::h5group> gFields;
        if (!outFile->object_exists("Monitors"))
            gFields = outFile->create_group("Monitors");
        else
            gFields = outFile->open_group("Monitors");

        unique_ptr<h5cpp::h5dset> dset;
        if (!outFile->object_exists("Monitors/"+name)) {
            vector<hsize_t> dims = {hsize_t(N)};
            vector<hsize_t> max_dims = {hsize_t(N)};
            //vector<hsize_t> chunk_dims = dims;
            h5cpp::dataspace ds(dims, max_dims);
            dset = gFields->create_dataset(name, 
                         h5cpp::dtype::Double, ds); 
            dset->write(data);
        }
        else {
            dset = gFields->open_dataset(name);
            dset->append(data);
        }

        clocks.stop(clock_name::hdf5);
    }

    void Field2D::display_info() {
        const int max_spacing = 5;
        cout << termcolor::bold << termcolor::underline << "Grid data" 
             << termcolor::reset << endl; 
        cout << "     " << left << setw(max_spacing) 
             << setfill('.') << "dx" << dx << " m" << endl;
        cout << "     " << left << setw(max_spacing) 
             << setfill('.') << "Lx" << dx*Nx << " m" << endl;
        cout << "     " << left << setw(max_spacing) 
             << setfill('.') << "Ly" << dx*Ny << " m" << endl;
        cout << "     " << left << setw(max_spacing) 
             << setfill('.') << "dt" << dt << " s" << endl;
        cout << endl;
    }

    //inside each for loop: make a call to an external function that makes the necessary update: vacuum, material, pml
    void Field2D::update() {
        tStep += 1;
        t += dt;
        
        clocks.start(clock_name::looping);
        for (int i=1; i<Nx-1; i++) {
            for (int j=1; j<Ny-1; j++) {
                Dz[i][j] = BC->gi3[i]*BC->gj3[j]*Dz[i][j] + 
                    BC->gi2[i]*BC->gj2[j]*0.5*(Hy[i][j]-Hy[i-1][j] - Hx[i][j]+Hx[i][j-1]);
             }
        }

        if (total) 
            total->updateD(this);

        for (int i=1; i<Nx-1; i++) {
            for (int j=1; j<Ny-1; j++) {
                Ez[i][j] = ca[i][j]*(Dz[i][j] -Iz[i][j]);
                Iz[i][j] += cb[i][j]*Ez[i][j];
             }
        }

        for (const auto &s : source_list) {
            s->pulse();
        } 
        clocks.stop(clock_name::looping);

        clocks.start(clock_name::fourier);
        for (const auto &m : monitor_list) {
            m->update();
        }
        clocks.stop(clock_name::fourier);


        clocks.start(clock_name::looping);
        //this can possibly be moved to the previous if statement
        if (total) 
            total->pulse();

        for (int i=1; i<Nx-1; i++) {
            for (int j=1; j<Ny-1; j++) {
                double curl_e = Ez[i+1][j] - Ez[i][j];
                BC->Ihy[i][j] += curl_e;
                Hy[i][j] = BC->fi3[i]*Hy[i][j] + BC->fi2[i]*0.5*curl_e + BC->fj1[j]*BC->Ihy[i][j];
                
                curl_e = Ez[i][j] - Ez[i][j+1];
                BC->Ihx[i][j] += curl_e;
                Hx[i][j] = BC->fj3[j]*Hx[i][j] + BC->fj2[j]*0.5*curl_e + BC->fi1[i]*BC->Ihx[i][j];
            }
        }

        if (total) 
            total->updateH(this);
        clocks.stop(clock_name::looping);
        
    }

    void Field2D::add_object(object &new_object) {
        obj_list.push_back(&new_object);
        double eps = new_object.eps;
        double conduc = new_object.conduc;

        for (int i = 0; i != Nx; i++) {
            for (int j = 0; j != Ny; j++) {
                vector<int> pi = {i,j};
                vector<int> p = grid.convertToReal(pi);
                if (new_object.inside(p)) {
                    obj[i][j] = &new_object;
                    obj[i][j] = &new_object;
                        
                    ca[i][j] = 1/(eps + conduc*dt/epsilon);
                    cb[i][j] = conduc*dt/epsilon;
                }
            }
        }
    }

    void Field2D::add_source(source &new_source) {
        new_source.set_F(this);
        source_list.push_back(&new_source);
    }

    void Field2D::add_monitor(monitor &new_monitor) {
        new_monitor.set_F(this);
        monitor_list.push_back(&new_monitor);
    } 

    grid_properties::grid_properties(int Lx, int Ly, int res, int pml_thickness):
            Lx(Lx), Ly(Ly), res(res), pml_thickness(pml_thickness) {
        Nx = Lx*res;
        Ny = Ly*res;
        dx = 1.0/res;
        totalFieldScatteredField = false;       
    }

    void grid_properties::set_tfsf(vector<int> p1_val, vector<int> p2_val){
        p1 = convertToGrid(p1_val);
        p2 = convertToGrid(p2_val);
        totalFieldScatteredField = true;
    }

    void grid_properties::set_tfsf(int xbuff, int ybuff){
        vector<int> p1_val = {xbuff, ybuff};
        vector<int> p2_val = {Lx-xbuff, Ly-ybuff};
        set_tfsf(p1_val, p2_val);
    }

    void grid_properties::set_tfsf(int buff){
        set_tfsf(buff, buff);
    }

    vector<int> grid_properties::convertToGrid(vector<int> p) {
        vector<int> pi = {p[0]*res, p[1]*res};
        return pi;
    }

    vector<int> grid_properties::convertToReal(vector<int> pi) {
        vector<int> p = {pi[0]/res, pi[1]/res};
        return p;
    }
}





