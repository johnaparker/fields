#ifndef GUARD_monitor_h
#define GUARD_monitor_h

#include <vector>
#include <string>
#include "matrix.h"
#include "field2.h"


//technically, these are DFT monitors. Normal monitor can exist too.
class monitor {
public:
    std::string name;             //to call output from main
    int N;
    double *freq;
    double *prevE;
    Field2D *F                    //to be set when call add_monitor in fields
public:
    monitor() {};
    virtual void update() {};
    virtual void output() {};
};

//add friendship to Field2D
class surface_monitor: public monitor {
public:
    std::vector<int> p1, p2;
    matrix<double> rE, iE, rH, iH;
    int dir;
    int length;
public:
    surface_monitor(std::string name, std::vector<double> p1, std::vector<double> p2, double *freq, int N);
    surface_monitor(std::string name, std::vector<double> p1, std::vector<double> p2, double fmin, double fmax, int N);
    surface_monitor(std::string name, std::vector<double> p1, std::vector<double> p2, double f);
    void update();
    void write(std::string filename);
};


#endif