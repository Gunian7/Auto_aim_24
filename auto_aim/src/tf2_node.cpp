#include "auto_aim/tf2_node.hpp"
#include <memory>
#include <rclcpp/logging.hpp>
#include <rclcpp/qos.hpp>
#include <sensor_msgs/msg/detail/joint_state__struct.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <std_msgs/msg/header.hpp>
#include <tf2/LinearMath/Quaternion.h>

namespace auto_aim {

TF2Node::TF2Node(const rclcpp::NodeOptions& options):
    Node("tf2_node", options) {
    this->shooter2camera_tvec_ = declare_parameter(
        "shooter2camera_tvec",
        std::vector<double> { 0.0, 0.0, 0.0 }
    );
    this->odom2shooter_r_ = declare_parameter("odom2shooter_r", 0.5);
    this->shooter_coordinate = declare_parameter("shooter_coordinate", "shooter");
    this->camera_coordinate = declare_parameter("camera_coordinate", "camera");
    this->odom_coordinate = declare_parameter("odom_coordinate", "odom");

    mock_hardware_ = declare_parameter("mock_hardware", false);
    mock_yaw_ = declare_parameter("mock_yaw", 0.0);
    mock_pitch_ = declare_parameter("mock_pitch", 0.0);

    broadcaster_shooter2camera_ = std::make_unique<tf2_ros::TransformBroadcaster>(this);
    broadcaster_odom2shooter_ = std::make_unique<tf2_ros::TransformBroadcaster>(this);
    tfs_shooter2camera_ = std::make_unique<geometry_msgs::msg::TransformStamped>();
    tfs_odom2shooter_ = std::make_unique<geometry_msgs::msg::TransformStamped>();

    auto publish_tf = [this](double yaw, double pitch, const rclcpp::Time& stamp) {
        SendTransform(
            broadcaster_odom2shooter_,
            tfs_odom2shooter_,
            stamp,
            odom_coordinate,
            shooter_coordinate,
            [pitch, yaw]() {
                tf2::Quaternion q;
                q.setRPY(0, pitch, yaw);
                return q;
            }(),
            tf2::Vector3(
                odom2shooter_r_ * cos(pitch) * cos(yaw),
                odom2shooter_r_ * cos(pitch) * sin(yaw),
                odom2shooter_r_ * sin(-pitch)
            )
        );
        SendTransform(
            broadcaster_shooter2camera_,
            tfs_shooter2camera_,
            stamp,
            shooter_coordinate,
            camera_coordinate,
            []() {
                tf2::Quaternion q;
                q.setEuler(M_PI_2, 0, -M_PI_2);
                return q;
            }(),
            tf2::Vector3(shooter2camera_tvec_[0], shooter2camera_tvec_[1], shooter2camera_tvec_[2])
        );
    };

    if (mock_hardware_) {
        RCLCPP_INFO(this->get_logger(),
            "tf2_node running in MOCK mode (yaw=%.2f, pitch=%.2f) — no hardware required",
            mock_yaw_, mock_pitch_);
        mock_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20),  // 50 Hz
            [this, publish_tf]() {
                publish_tf(mock_yaw_, mock_pitch_, this->now());
            }
        );
    } else {
        RCLCPP_INFO(this->get_logger(), "tf2_node running in REAL mode — waiting for gyro data");
        euler_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "/communicate/gyro/left",
            rclcpp::SensorDataQoS(),
            [this, publish_tf](const sensor_msgs::msg::JointState::SharedPtr msg) {
                publish_tf(msg->position[0], msg->position[1], msg->header.stamp);
            }
        );
    }
}

void TF2Node::SendTransform(
    const std::unique_ptr<tf2_ros::TransformBroadcaster>& broadcaster,
    const std::unique_ptr<geometry_msgs::msg::TransformStamped>& tfs,
    const rclcpp::Time& timestamp,
    const std::string& frame_id,
    const std::string& child_frame_id,
    const tf2::Quaternion& q,
    const tf2::Vector3& v
) {
    tfs->header.stamp = timestamp;
    tfs->header.frame_id = frame_id;
    tfs->child_frame_id = child_frame_id;
    tfs->transform.rotation.x = q.getX();
    tfs->transform.rotation.y = q.getY();
    tfs->transform.rotation.z = q.getZ();
    tfs->transform.rotation.w = q.getW();
    tfs->transform.translation.x = v.getX();
    tfs->transform.translation.y = v.getY();
    tfs->transform.translation.z = v.getZ();

    broadcaster->sendTransform(*tfs);
}
} // namespace auto_aim

#include "rclcpp_components/register_node_macro.hpp"

// Register the component with class_loader.
RCLCPP_COMPONENTS_REGISTER_NODE(auto_aim::TF2Node)
