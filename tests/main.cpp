#include <qbox.h>
#include <iostream>

using namespace std;
using namespace qbox;

int main() {
    int pml_thickness = 10;
    int res = 2;
    double Lx = 120;
    double Ly = 120;


    grid_properties grid(Lx,Ly,res,pml_thickness);

    Field2D test(grid, "out.h5");
    
    double f = 2/30.0;

    test.add<point_source> (fields::Ez, vec(30,30), gaussian_time(f, 1/200.0, 80));

    for (int i = 0; i != 3000; i++) {
        test.update();
        test.writeE();
    }
}
