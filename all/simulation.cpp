
#include "simulation.h"

std::atomic<bool> Simulation::runAble = true;

void Simulation::start() {
    // Vytvorenie riadiaceho vlákna
    std::thread controller(&Simulation::controlThread, this);
}

void Simulation::controlThread() {
    while (runAble) {
    }
}
