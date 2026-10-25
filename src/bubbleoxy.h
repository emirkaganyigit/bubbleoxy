#pragma once
#include <cmath>
#include <cstdint>

namespace bubbleoxy {
enum class State { Idle, Priming, Aerating, LowBattery, Fault };
enum class Fault { None, Configuration, Sensor, Emergency, NoFlow };

// Illustrative simulation values only. Calibrate before connecting equipment.
struct Config {
  float oxygenStart = 4.0f;  // mg/L
  float oxygenStop = 6.0f;
  float batteryStop = 20.0f; // measured/estimated SOC %, NOT raw voltage
  float batteryResume = 35.0f;
  uint32_t maxSampleAgeMs = 3000;
  uint32_t primeTimeoutMs = 5000;
  uint32_t flowStableMs = 1000;
  uint32_t minRunMs = 10000;
  uint32_t minOffMs = 5000;
};

// Supply a coherent snapshot. Update sampledAtMs only after ALL required
// measurements are refreshed. A cached/failed measurement must not look fresh.
struct Input {
  float oxygenMgL = 0;
  float batterySoc = 0;
  bool valid = false;
  bool flow = false;
  bool emergency = false; // aggregate leak / overcurrent / emergency stop
  uint32_t sampledAtMs = 0;
};
struct Output {
  State state;
  Fault fault;
  bool pump;
  bool compressor;
};

class Controller {
 public:
  explicit Controller(Config config = {}) : config_(config) {}
  Output tick(uint32_t now, const Input& in, bool acknowledgeFault = false) {
    if (!started_) { started_ = true; enteredAt_ = now; lastStopAt_ = now; }
    if (!configurationValid()) return trip(now, Fault::Configuration);
    if (in.emergency) return trip(now, Fault::Emergency);
    if (!sampleValid(now, in)) return trip(now, Fault::Sensor);

    // Fault acknowledgement never restarts actuators in the same tick.
    if (state_ == State::Fault) {
      if (acknowledgeFault) {
        fault_ = Fault::None;
        transition(now, State::Idle);
        lastStopAt_ = now;
      }
      return output();
    }
    // Safety conditions override minimum run time.
    if (in.batterySoc <= config_.batteryStop) {
      if (state_ != State::LowBattery) transition(now, State::LowBattery);
      return output();
    }
    if (state_ == State::LowBattery) {
      if (in.batterySoc >= config_.batteryResume) transition(now, State::Idle);
      return output();
    }
    if (state_ == State::Idle) {
      if (in.oxygenMgL <= config_.oxygenStart &&
          elapsed(now, lastStopAt_) >= config_.minOffMs)
        transition(now, State::Priming);
    } else if (state_ == State::Priming) {
      if (in.oxygenMgL >= config_.oxygenStop) {
        transition(now, State::Idle);
      } else {
        if (in.flow) {
          if (!flowObserved_) { flowObserved_ = true; flowSince_ = now; }
        } else { flowObserved_ = false; }
        if (flowObserved_ && elapsed(now, flowSince_) >= config_.flowStableMs)
          transition(now, State::Aerating);
        else if (elapsed(now, enteredAt_) >= config_.primeTimeoutMs)
          return trip(now, Fault::NoFlow);
      }
    } else if (state_ == State::Aerating) {
      if (!in.flow) return trip(now, Fault::NoFlow);
      if (in.oxygenMgL >= config_.oxygenStop &&
          elapsed(now, enteredAt_) >= config_.minRunMs)
        transition(now, State::Idle);
    }
    return output();
  }

 private:
  Config config_;
  State state_ = State::Idle;
  Fault fault_ = Fault::None;
  bool started_ = false, flowObserved_ = false;
  uint32_t enteredAt_ = 0, lastStopAt_ = 0, flowSince_ = 0;
  static uint32_t elapsed(uint32_t now, uint32_t then) { return now - then; }
  bool configurationValid() const {
    return std::isfinite(config_.oxygenStart) && std::isfinite(config_.oxygenStop) &&
      config_.oxygenStart >= 0 && config_.oxygenStart < config_.oxygenStop &&
      std::isfinite(config_.batteryStop) && std::isfinite(config_.batteryResume) &&
      config_.batteryStop >= 0 && config_.batteryStop < config_.batteryResume &&
      config_.batteryResume <= 100 && config_.maxSampleAgeMs > 0 &&
      config_.flowStableMs > 0 && config_.primeTimeoutMs > config_.flowStableMs &&
      config_.maxSampleAgeMs < 0x80000000u && config_.primeTimeoutMs < 0x80000000u &&
      config_.minRunMs < 0x80000000u && config_.minOffMs < 0x80000000u;
  }
  bool sampleValid(uint32_t now, const Input& in) const {
    return in.valid && std::isfinite(in.oxygenMgL) && in.oxygenMgL >= 0 &&
      std::isfinite(in.batterySoc) && in.batterySoc >= 0 && in.batterySoc <= 100 &&
      elapsed(now, in.sampledAtMs) <= config_.maxSampleAgeMs;
  }
  void transition(uint32_t now, State next) {
    if (state_ == State::Priming || state_ == State::Aerating) lastStopAt_ = now;
    state_ = next;
    enteredAt_ = now;
    flowObserved_ = false;
  }
  Output trip(uint32_t now, Fault reason) {
    // Preserve the first fault to support diagnosis.
    if (state_ != State::Fault) { fault_ = reason; transition(now, State::Fault); }
    return output();
  }
  Output output() const {
    return {state_, fault_, state_ == State::Priming || state_ == State::Aerating,
      state_ == State::Aerating};
  }
};
inline const char* name(State s) {
  switch (s) {
    case State::Idle: return "IDLE";
    case State::Priming: return "PRIMING";
    case State::Aerating: return "AERATING";
    case State::LowBattery: return "LOW_BATTERY";
    case State::Fault: return "FAULT";
  }
  return "UNKNOWN";
}
inline const char* name(Fault f) {
  switch (f) {
    case Fault::None: return "NONE";
    case Fault::Configuration: return "CONFIGURATION";
    case Fault::Sensor: return "SENSOR";
    case Fault::Emergency: return "EMERGENCY";
    case Fault::NoFlow: return "NO_FLOW";
  }
  return "UNKNOWN";
}
} // namespace bubbleoxy
