#include <qbox.h>
#include <iostream>

using namespace std;
using namespace qbox;

int main() {
    int pml_thickness = 20;
    int res = 2;
    double Lx = 150;
    double Ly = 70;


    grid_properties grid(Lx,Ly,res,pml_thickness);
    grid.set_tfsf(12, 12);
    //grid.set_tfsf({12,12},{68,119});

    Field2D test(grid, "out.h5");
    
    double f = 2/30.0;

    vec center{Lx/2,Ly/2};
    vec gap{25,0};

    auto mat = simple_material(5);
    auto cyl = cylinder(10);
    object o3(cyl, mat, center - 2*gap); 
    test.add_object(o3);
    object o4(cyl, mat, center - gap);
    test.add_object(o4);
    object o1(cyl, mat, center + gap);
    test.add_object(o1);
    object o2(cyl, mat, center + 2*gap);
    test.add_object(o2);

    box_monitor m1(volume(center, 5*gap[0], 30), freq_data(1/30.0,3/30.0, 500), false); 
    test.add_monitor(m1);

    gaussian_point_source s1(fields::Ez, center, f, 1/200.0, 80);
    //continuous_point_source s1({60,60}, 1/20.0);
    //gaussian_line_source s1(surface({30,0}, {30,120}), f, 1/200.0, 80);
    //continuous_line_source s1(surface({110,30}, {10,30}), f);
    test.add_source(s1);

    for (int i = 0; i != 4000; i++) {
        test.update();
        test.writeE();
    }
    //m1.write_flux_sides();
}
