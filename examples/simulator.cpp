#include "bubbleoxy.h"
#include <iostream>
int main() {
  bubbleoxy::Controller controller;
  std::cout << "time_ms,oxygen_mg_l,battery_soc,flow,state,fault,pump,compressor\n";
  for (uint32_t t = 0; t <= 40000; t += 1000) {
    bubbleoxy::Input in;
    in.valid = true;
    in.sampledAtMs = t;
    in.oxygenMgL = t < 18000 ? 3.5f : 6.5f;
    in.batterySoc = t >= 25000 && t < 35000 ? 15.0f : 80.0f;
    in.flow = t >= 6000 && t <= 18000;
    auto out = controller.tick(t, in);
    std::cout << t << ',' << in.oxygenMgL << ',' << in.batterySoc << ',' << in.flow
      << ',' << bubbleoxy::name(out.state) << ',' << bubbleoxy::name(out.fault)
      << ',' << out.pump << ',' << out.compressor << '\n';
  }
}
