#include <gtest/gtest.h>

#include <array>
#include <cmath>

#include "armor_tracker/outpost_height_model.hpp"

namespace armor {
namespace {

constexpr double kPhaseStep = 2.0 * M_PI / 3.0;

OutpostHeightModel CreateReadyModel() {
    OutpostHeightModel model;
    model.Reset(0.0);
    for (int turn = 0; turn < 3; ++turn) {
        model.Observe(turn * 2.0 * M_PI, 0.90);
        model.Observe(turn * 2.0 * M_PI + kPhaseStep, 1.00);
        model.Observe(turn * 2.0 * M_PI + 2.0 * kPhaseStep, 1.10);
    }
    return model;
}

TEST(OutpostHeightModelTest, LearnsThreeHeightLevelsAcrossRotatingObservations) {
    auto model = CreateReadyModel();

    ASSERT_TRUE(model.Ready());
    EXPECT_NEAR(model.HeightOffset(0), -0.10, 1e-9);
    EXPECT_NEAR(model.HeightOffset(1), 0.00, 1e-9);
    EXPECT_NEAR(model.HeightOffset(2), 0.10, 1e-9);
}

TEST(OutpostHeightModelTest, NormalizesEveryLevelToTheSameCenterHeight) {
    auto model = CreateReadyModel();

    ASSERT_TRUE(model.Ready());
    EXPECT_NEAR(model.NormalizeHeight(0.90, 0), 1.00, 1e-9);
    EXPECT_NEAR(model.NormalizeHeight(1.00, 1), 1.00, 1e-9);
    EXPECT_NEAR(model.NormalizeHeight(1.10, 2), 1.00, 1e-9);
    EXPECT_NEAR(model.ArmorHeight(1.00, 0), 0.90, 1e-9);
    EXPECT_NEAR(model.ArmorHeight(1.00, 1), 1.00, 1e-9);
    EXPECT_NEAR(model.ArmorHeight(1.00, 2), 1.10, 1e-9);
}

TEST(OutpostHeightModelTest, MapsGeneratedArmorIndexFromCurrentPhase) {
    auto model = CreateReadyModel();

    ASSERT_TRUE(model.Ready());
    EXPECT_EQ(model.PhaseIdAtOffset(2, 0), 2);
    EXPECT_EQ(model.PhaseIdAtOffset(2, 1), 0);
    EXPECT_EQ(model.PhaseIdAtOffset(2, 2), 1);
    EXPECT_NEAR(model.ArmorHeightAtOffset(1.00, 2, 0), 1.10, 1e-9);
    EXPECT_NEAR(model.ArmorHeightAtOffset(1.00, 2, 1), 0.90, 1e-9);
    EXPECT_NEAR(model.ArmorHeightAtOffset(1.00, 2, 2), 1.00, 1e-9);
}

} // namespace
} // namespace armor
