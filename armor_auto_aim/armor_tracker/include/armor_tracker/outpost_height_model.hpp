#ifndef ARMOR_TRACKER__OUTPOST_HEIGHT_MODEL_HPP_
#define ARMOR_TRACKER__OUTPOST_HEIGHT_MODEL_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace armor {

class OutpostHeightModel {
public:
    static constexpr int kArmorCount = 3;
    static constexpr std::size_t kMinSamplesPerArmor = 3;
    static constexpr double kHeightStep = 0.10;
    static constexpr double kMinHeightGap = 0.05;

    void Reset(double reference_yaw) {
        reference_yaw_ = reference_yaw;
        ready_ = false;
        offsets_.fill(0.0);
        for (auto& samples: height_samples_) {
            samples.clear();
        }
    }

    int Observe(double yaw, double height) {
        const int id = PhaseId(yaw);
        height_samples_[id].push_back(height);
        TryFinalize();
        return id;
    }

    int PhaseId(double yaw) const {
        constexpr double phase_step = 2.0 * std::numbers::pi / kArmorCount;
        const auto phase_index = static_cast<long long>(std::llround((yaw - reference_yaw_) / phase_step));
        int id = static_cast<int>(phase_index % kArmorCount);
        if (id < 0) {
            id += kArmorCount;
        }
        return id;
    }

    bool Ready() const { return ready_; }

    double HeightOffset(int id) const {
        return IsValidId(id) ? offsets_[id] : 0.0;
    }

    double NormalizeHeight(double measured_height, int id) const {
        return measured_height - HeightOffset(id);
    }

    double ArmorHeight(double center_height, int id) const {
        return center_height + HeightOffset(id);
    }

    int PhaseIdAtOffset(int current_phase_id, int armor_offset) const {
        int id = (current_phase_id + armor_offset) % kArmorCount;
        if (id < 0) {
            id += kArmorCount;
        }
        return id;
    }

    double ArmorHeightAtOffset(
        double center_height, int current_phase_id, int armor_offset) const
    {
        return ArmorHeight(center_height, PhaseIdAtOffset(current_phase_id, armor_offset));
    }

    const std::array<double, kArmorCount>& HeightOffsets() const { return offsets_; }

private:
    static bool IsValidId(int id) {
        return id >= 0 && id < kArmorCount;
    }

    static double Median(std::vector<double> samples) {
        std::sort(samples.begin(), samples.end());
        const std::size_t middle = samples.size() / 2;
        if (samples.size() % 2 == 1) {
            return samples[middle];
        }
        return 0.5 * (samples[middle - 1] + samples[middle]);
    }

    void TryFinalize() {
        if (ready_) {
            return;
        }
        for (const auto& samples: height_samples_) {
            if (samples.size() < kMinSamplesPerArmor) {
                return;
            }
        }

        std::array<double, kArmorCount> medians {};
        std::array<int, kArmorCount> order { 0, 1, 2 };
        for (int id = 0; id < kArmorCount; ++id) {
            medians[id] = Median(height_samples_[id]);
        }
        std::sort(order.begin(), order.end(), [&](int lhs, int rhs) {
            return medians[lhs] < medians[rhs];
        });

        if (medians[order[1]] - medians[order[0]] < kMinHeightGap
            || medians[order[2]] - medians[order[1]] < kMinHeightGap)
        {
            return;
        }

        offsets_[order[0]] = -kHeightStep;
        offsets_[order[1]] = 0.0;
        offsets_[order[2]] = kHeightStep;
        ready_ = true;
    }

    double reference_yaw_ { 0.0 };
    bool ready_ { false };
    std::array<std::vector<double>, kArmorCount> height_samples_;
    std::array<double, kArmorCount> offsets_ { 0.0, 0.0, 0.0 };
};

} // namespace armor

#endif // ARMOR_TRACKER__OUTPOST_HEIGHT_MODEL_HPP_
