#include "drake/examples/Cars/simple_car.h"

#include <cmath>
#include <memory>

#include "gtest/gtest.h"

#include "drake/common/eigen_matrix_compare.h"
#include "drake/examples/Cars/system1_cars_vectors.h"
#include "drake/systems/Simulation.h"
#include "drake/systems/cascade_system.h"
#include "drake/systems/simulation_options.h"
#include "drake/systems/vector.h"

using drake::NullVector;
using drake::SimulationOptions;

namespace drake {
namespace examples {
namespace simple_car {
namespace test {

template <template <typename> class Vector>
class ConstantInputSystem {
 public:
  template <typename ScalarType>
  using StateVector = NullVector<ScalarType>;
  template <typename ScalarType>
  using InputVector = NullVector<ScalarType>;
  template <typename ScalarType>
  using OutputVector = Vector<ScalarType>;

  explicit ConstantInputSystem(const OutputVector<double>& constant)
      : output_(constant) {}

  StateVector<double> dynamics(const double& t, const StateVector<double>& x,
                               const InputVector<double>& u) const {
    return StateVector<double>();
  }

  OutputVector<double> output(const double& t, const StateVector<double>& x,
                              const InputVector<double>& u) const {
    return output_;
  }

 private:
  const OutputVector<double> output_;
};

template <template <typename> class Vector>
class HistorySystem {
 public:
  template <typename ScalarType>
  using StateVector = NullVector<ScalarType>;
  template <typename ScalarType>
  using InputVector = Vector<ScalarType>;
  template <typename ScalarType>
  using OutputVector = NullVector<ScalarType>;

  explicit HistorySystem(const InputVector<double> initial)
      : initial_(initial) {}

  StateVector<double> dynamics(const double& t, const StateVector<double>& x,
                               const InputVector<double>& u) const {
    return StateVector<double>();
  }

  OutputVector<double> output(const double& t, const StateVector<double>& x,
                              const InputVector<double>& u) const {
    // Pack-rat the inputs.
    if (!std::isfinite(t)) {
      throw std::logic_error("HistorySystem: timestamp is not finite!");
    }
    states_[t] = u;
    return OutputVector<double>();
  }

  const InputVector<double> initial_;
  mutable std::map<double, InputVector<double>> states_;
};

GTEST_TEST(SimpleCarTest, ZerosIn) {
  SimpleCar dut;
  SimpleCarState1<double> state_zeros;
  DrivingCommand1<double> input_zeros;

  SimpleCarState1<double> rates =
      dut.dynamics(0., state_zeros, input_zeros);

  const double tolerance = 1e-8;
  EXPECT_TRUE(CompareMatrices(toEigen(state_zeros), toEigen(rates),
                              tolerance, MatrixCompareType::absolute));
}

GTEST_TEST(SimpleCarTest, Accelerating) {
  DrivingCommand1<double> max_throttle;
  max_throttle.set_throttle(1.);

  auto car = std::make_shared<SimpleCar>();
  SimpleCarState1<double> initial_state;
  auto history_system =
      std::make_shared<HistorySystem<SimpleCarState1>>(initial_state);
  auto lead_foot = drake::cascade(
      drake::cascade(
          std::make_shared<ConstantInputSystem<DrivingCommand1>>(
              max_throttle),
          car),
      history_system);

  double start_time = 0.;
  double end_time = 100.;
  drake::simulate(*lead_foot, start_time, end_time, initial_state);

  double step_size = SimulationOptions().initial_step_size;
  int steps = (end_time - start_time) / step_size;

  EXPECT_EQ(static_cast<int>(history_system->states_.size()), steps);

  double max_time = history_system->states_.rbegin()->first;
  EXPECT_NEAR(max_time, end_time - step_size, 1e-5);

  EXPECT_EQ(history_system->states_[start_time].x(), 0.);
  EXPECT_EQ(history_system->states_[start_time].y(), 0.);
  EXPECT_EQ(history_system->states_[start_time].heading(), 0.);
  EXPECT_EQ(history_system->states_[start_time].velocity(), 0.);

  // These clauses are deliberately broad; we don't care so much about the
  // exact values, but rather that we got somewhere.
  EXPECT_NEAR(history_system->states_[max_time].x(),
              SimpleCar::kDefaultConfig.max_velocity *
              (end_time - start_time), 3e2);
  EXPECT_GT(history_system->states_[max_time].x(),
            SimpleCar::kDefaultConfig.max_velocity *
            (end_time - start_time) / 2);
  EXPECT_EQ(history_system->states_[max_time].y(), 0.);
  EXPECT_EQ(history_system->states_[max_time].heading(), 0.);
  EXPECT_NEAR(history_system->states_[max_time].velocity(),
              SimpleCar::kDefaultConfig.max_velocity, 1e-5);
}

GTEST_TEST(SimpleCarTest, Braking) {
  DrivingCommand1<double> max_brake;
  max_brake.set_brake(1.);

  auto car = std::make_shared<SimpleCar>();
  SimpleCarState1<double> initial_state;
  double speed = 10.;
  initial_state.set_velocity(speed);

  auto history_system =
      std::make_shared<HistorySystem<SimpleCarState1>>(initial_state);
  auto panic_stop = drake::cascade(
      drake::cascade(
          std::make_shared<ConstantInputSystem<DrivingCommand1>>(
              max_brake),
          car),
      history_system);

  double start_time = 0.;
  double end_time = 100.;
  drake::simulate(*panic_stop, start_time, end_time, initial_state);

  double step_size = SimulationOptions().initial_step_size;
  int steps = (end_time - start_time) / step_size;

  EXPECT_EQ(static_cast<int>(history_system->states_.size()), steps);

  double max_time = history_system->states_.rbegin()->first;
  EXPECT_NEAR(max_time, end_time - step_size, 1e-5);

  EXPECT_EQ(history_system->states_[start_time].x(), 0.);
  EXPECT_EQ(history_system->states_[start_time].y(), 0.);
  EXPECT_EQ(history_system->states_[start_time].heading(), 0.);
  EXPECT_EQ(history_system->states_[start_time].velocity(), speed);

  // This clause is deliberately broad; we don't care so much about the exact
  // value, but rather that we stopped reasonably.
  EXPECT_LT(history_system->states_[max_time].x(), 20.);
  EXPECT_EQ(history_system->states_[max_time].y(), 0.);
  EXPECT_EQ(history_system->states_[max_time].heading(), 0.);
  EXPECT_NEAR(history_system->states_[max_time].velocity(), 0., 1e-5);
}

GTEST_TEST(SimpleCarTest, Steering) {
  DrivingCommand1<double> left(Eigen::Vector3d(M_PI / 2, 0., 0.));

  auto car = std::make_shared<SimpleCar>();
  SimpleCarState1<double> initial_state;
  double speed = 40.;
  initial_state.set_velocity(speed);

  auto history_system =
      std::make_shared<HistorySystem<SimpleCarState1>>(initial_state);
  auto brickyard = drake::cascade(
      drake::cascade(
          std::make_shared<ConstantInputSystem<DrivingCommand1>>(left),
          car),
      history_system);

  double start_time = 0.;
  double end_time = 100.;
  drake::simulate(*brickyard, start_time, end_time, initial_state);

  double step_size = SimulationOptions().initial_step_size;
  int steps = (end_time - start_time) / step_size;

  EXPECT_EQ(static_cast<int>(history_system->states_.size()), steps);

  double max_time = history_system->states_.rbegin()->first;
  EXPECT_NEAR(max_time, end_time - step_size, 1e-5);

  EXPECT_EQ(history_system->states_[start_time].x(), 0.);
  EXPECT_EQ(history_system->states_[start_time].y(), 0.);
  EXPECT_EQ(history_system->states_[start_time].heading(), 0.);
  EXPECT_EQ(history_system->states_[start_time].velocity(), speed);

  double min_turn_radius =
      SimpleCar::kDefaultConfig.wheelbase /
      std::tan(SimpleCar::kDefaultConfig.max_abs_steering_angle);
  double turn_epsilon = min_turn_radius * 1e-3;

  for (const auto& pair : history_system->states_) {
    const auto& state = pair.second;
    // Our drive circle should fit in a small box.
    EXPECT_GT(state.x(), -(min_turn_radius + turn_epsilon));
    EXPECT_LT(state.x(), min_turn_radius + turn_epsilon);
    EXPECT_GT(state.y(), -turn_epsilon);
    EXPECT_LT(state.y(), (2 * min_turn_radius + turn_epsilon));
    // Our drive state should be near the predicted circle.
    EXPECT_NEAR(std::hypot(state.x(), state.y() - min_turn_radius),
                min_turn_radius, 1e-2);
  }

  // Predict our final heading.
  double predicted_heading =
      std::remainder((speed / min_turn_radius) * (max_time - start_time),
                      2 * M_PI);
  double end_heading =
      std::remainder(history_system->states_[max_time].heading(), 2 * M_PI);
  EXPECT_NEAR(end_heading, predicted_heading, 1e-5);

  EXPECT_EQ(history_system->states_[max_time].velocity(), speed);
}
}
}
}
}
