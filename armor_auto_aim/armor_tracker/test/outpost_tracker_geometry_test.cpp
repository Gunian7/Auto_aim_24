#include <gtest/gtest.h>

#include <array>
#include <cmath>

#include "armor_tracker/tracker.hpp"

namespace armor {
namespace {

constexpr double kRadius = 0.2765;
constexpr double kPhaseStep = 2.0 * M_PI / 3.0;

CarState MakeOutpostState(double yaw, int current_phase_id) {
    CarState state;
    state.id = "outpost";
    state.armors_num = 3;
    state.position = { 2.0, 0.0, 1.0, yaw };
    state.velocity = Eigen::Vector4d::Zero();
    state.r[0] = kRadius;
    state.r[1] = kRadius;
    state.dz = 0.0;
    state.armor_height_offsets = { -0.10, 0.0, 0.10 };
    state.current_outpost_phase_id = current_phase_id;
    return state;
}

TEST(OutpostTrackerGeometryTest, ChoosesHeightForTheClosestOutpostPhase) {
    Tracker tracker(0.5, 2.5);

    const auto low = tracker.ChooseArmor(MakeOutpostState(0.0, 0), 0.0F, 1.0F, 0.0F);
    const auto middle = tracker.ChooseArmor(MakeOutpostState(0.0, 1), 0.0F, 1.0F, 0.0F);
    const auto high = tracker.ChooseArmor(MakeOutpostState(0.0, 2), 0.0F, 1.0F, 0.0F);

    EXPECT_NEAR(low.z(), 0.90, 1e-6);
    EXPECT_NEAR(middle.z(), 1.00, 1e-6);
    EXPECT_NEAR(high.z(), 1.10, 1e-6);
}

TEST(OutpostTrackerGeometryTest, UsesTheOutpostRadius) {
    Tracker tracker(0.5, 2.5);
    const auto armor = tracker.ChooseArmor(MakeOutpostState(0.0, 0), 0.0F, 1.0F, 0.0F);

    EXPECT_NEAR(armor.x(), 2.0 - kRadius, 1e-6);
    EXPECT_NEAR(armor.y(), 0.0, 1e-6);
}

} // namespace
} // namespace armor
