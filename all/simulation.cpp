
#include "simulation.h"

std::atomic<bool> Simulation::runAble = true;

void Simulation::start() {
    // Vytvorenie riadiaceho vl�kna
    std::thread controller(&Simulation::controlThread, this);
}

void Simulation::controlThread() {
    while (runAble) {
    }
}
