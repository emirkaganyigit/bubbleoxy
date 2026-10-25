#include "bubbleoxy.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace bubbleoxy;
static int checks = 0;
static void check(bool ok, const char* label) {
  ++checks;
  if (!ok) { std::cerr << "FAIL: " << label << '\n'; std::exit(1); }
}
static Input sample(uint32_t t, float oxygen = 3.5f, float battery = 80, bool flow = false) {
  Input in; in.valid = true; in.sampledAtMs = t; in.oxygenMgL = oxygen;
  in.batterySoc = battery; in.flow = flow; return in;
}
static void running(Controller& c) {
  c.tick(0, sample(0)); c.tick(5000, sample(5000));
  c.tick(6000, sample(6000, 3.5f, 80, true));
  check(c.tick(7000, sample(7000, 3.5f, 80, true)).state == State::Aerating, "start after stable flow");
}
int main() {
  {
    Controller c;
    auto o = c.tick(0, sample(0));
    check(!o.pump && !o.compressor, "boot off");
    check(c.tick(4999, sample(4999)).state == State::Idle, "boot cooldown");
    o = c.tick(5000, sample(5000, 4));
    check(o.pump && !o.compressor && o.state == State::Priming, "lower boundary starts pump only");
    check(c.tick(10000, sample(10000)).fault == Fault::NoFlow, "prime timeout");
    check(c.tick(11000, sample(11000)).state == State::Fault, "fault latches");
    o = c.tick(12000, sample(12000), true);
    check(o.state == State::Idle && !o.pump, "ack stays off");
    check(c.tick(16999, sample(16999)).state == State::Idle, "ack cooldown");
    check(c.tick(17000, sample(17000)).state == State::Priming, "restart after ack cooldown");
  }
  {
    Controller c; running(c);
    check(c.tick(8000, sample(8000, 6, 80, true)).state == State::Aerating, "minimum run time");
    check(c.tick(17000, sample(17000, 5, 80, true)).state == State::Aerating, "oxygen hysteresis holds");
    check(c.tick(18000, sample(18000, 6, 80, true)).state == State::Idle, "upper boundary stops");
    check(c.tick(22000, sample(22000)).state == State::Idle, "restart cooldown");
    check(c.tick(23000, sample(23000)).state == State::Priming, "restart allowed");
  }
  {
    Controller c; running(c);
    auto o = c.tick(8000, sample(8000, 3.5f, 20, true));
    check(o.state == State::LowBattery && !o.pump && !o.compressor, "battery overrides minimum runtime");
    check(c.tick(9000, sample(9000, 3.5f, 34)).state == State::LowBattery, "battery hysteresis");
    check(c.tick(10000, sample(10000, 3.5f, 35)).state == State::Idle, "battery recovery");
    check(c.tick(10001, sample(10001)).state == State::Idle, "battery recovery respects last stop");
  }
  {
    Controller c; running(c);
    auto o = c.tick(8000, sample(8000));
    check(o.fault == Fault::NoFlow && !o.pump && !o.compressor, "running flow loss stops immediately");
  }
  {
    Controller c; running(c);
    auto in = sample(8000, 3.5f, 80, true); in.emergency = true;
    auto o = c.tick(8000, in, true);
    check(o.fault == Fault::Emergency && !o.pump, "emergency overrides ack");
    check(c.tick(8001, in, true).state == State::Fault, "active emergency cannot reset");
  }
  {
    Controller c; running(c);
    auto o = c.tick(10001, sample(7000, 3.5f, 80, true));
    check(o.fault == Fault::Sensor && !o.pump && !o.compressor, "stale sensor stops running equipment");
  }
  {
    Controller c;
    check(c.tick(3000, sample(0)).state == State::Idle, "sample age exact boundary valid");
    check(c.tick(3001, sample(0), true).state == State::Fault, "stale data cannot acknowledge");
  }
  {
    Controller c; auto in = sample(0); in.valid = false;
    check(c.tick(0, in).fault == Fault::Sensor, "invalid sensor flag");
    Controller d; in = sample(0); in.oxygenMgL = std::numeric_limits<float>::quiet_NaN();
    check(d.tick(0, in).fault == Fault::Sensor, "NaN oxygen");
    Controller e; in = sample(0, 3.5f, 101);
    check(e.tick(0, in).fault == Fault::Sensor, "out of range SOC");
    Controller f; in = sample(0, -1);
    check(f.tick(0, in).fault == Fault::Sensor, "negative oxygen");
  }
  {
    Config config; config.oxygenStop = config.oxygenStart;
    Controller c(config);
    check(c.tick(0, sample(0)).fault == Fault::Configuration, "invalid thresholds");
    check(c.tick(1, sample(1), true).state == State::Fault, "invalid config cannot reset");
  }
  {
    Controller c; c.tick(0, sample(0)); c.tick(5000, sample(5000));
    c.tick(6000, sample(6000, 3.5f, 80, true));
    c.tick(6500, sample(6500));
    check(c.tick(7000, sample(7000, 3.5f, 80, true)).state == State::Priming, "interrupted flow resets timer");
    check(c.tick(7999, sample(7999, 3.5f, 80, true)).state == State::Priming, "flow not stable yet");
    check(c.tick(8000, sample(8000, 3.5f, 80, true)).state == State::Aerating, "flow stable boundary");
  }
  {
    Controller c; c.tick(0, sample(0)); c.tick(5000, sample(5000));
    check(c.tick(6000, sample(6000, 6)).state == State::Idle, "oxygen recovery cancels priming");
  }
  {
    Controller c; const uint32_t start = 0xfffffff0u;
    c.tick(start, sample(start));
    const uint32_t later = start + 5000u;
    check(c.tick(later, sample(later)).state == State::Priming, "millis wrap cooldown");
    c.tick(later + 1000u, sample(later + 1000u, 3.5f, 80, true));
    check(c.tick(later + 2000u, sample(later + 2000u, 3.5f, 80, true)).state == State::Aerating, "millis wrap start");
  }
  std::cout << checks << " checks passed\n";
}
