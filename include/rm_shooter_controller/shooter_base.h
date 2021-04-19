//
// Created by huakang on 2021/3/28.
//

#ifndef RM_SHOOTER_CONTROLLERS_INCLUDE_RM_SHOOTER_CONTROLLER_SHOOTER_BASE_H_
#define RM_SHOOTER_CONTROLLERS_INCLUDE_RM_SHOOTER_CONTROLLER_SHOOTER_BASE_H_

#include <control_toolbox/pid.h>
#include <controller_interface/multi_interface_controller.h>
#include <hardware_interface/joint_command_interface.h>
#include <rm_common/hardware_interface/robot_state_interface.h>
#include <rm_common/ros_utilities.h>
#include <realtime_tools/realtime_publisher.h>
#include <dynamic_reconfigure/server.h>
#include <rm_shooter_controllers/ShooterBaseConfig.h>
#include <rm_msgs/ShootCmd.h>

namespace rm_shooter_base {

enum State {
  PASSIVE = 0,
  READY = 1,
  PUSH = 2,
  STOP = 3,
  BLOCK = 4
};

enum MagazineState {
  MAGAZINE_PASSIVE = 0,
  OPEN = 1,
  CLOSE = 2
};

struct Config {
  double push_angle, magazine_q_des;
  double qd_10, qd_15, qd_16, qd_18, qd_30;
};

struct BlockConfig {
  double block_effort, block_duration, block_speed, anti_block_angle, anti_block_error;
};

class Block {
 public:
  Block(ros::NodeHandle &controller_nh) {
    block_config_ = {.block_effort = getParam(controller_nh, "block_effort", 0.),
        .block_duration = getParam(controller_nh, "block_duration", 0.),
        .block_speed = getParam(controller_nh, "block_speed", 0.),
        .anti_block_angle = getParam(controller_nh, "anti_block_angle", 0.),
        .anti_block_error = getParam(controller_nh, "anti_block_error", 0.),};
    block_config_rt_buffer_.initRT(block_config_);
  };
  bool isBlock(const ros::Time &time, const hardware_interface::JointHandle joint_handle);

  BlockConfig block_config_{};
  realtime_tools::RealtimeBuffer<BlockConfig> block_config_rt_buffer_{};
  bool is_start_block_time_ = false;
  ros::Time block_time_;

};

class ShooterBase : public controller_interface::MultiInterfaceController<hardware_interface::EffortJointInterface,
                                                                          hardware_interface::RobotStateInterface> {
 public:
  ShooterBase() = default;
  virtual bool init(hardware_interface::RobotHW *robot_hw,
                    ros::NodeHandle &root_nh, ros::NodeHandle &controller_nh) override;
  virtual void update(const ros::Time &time, const ros::Duration &period) override;
 protected:
  virtual void passive();
  virtual void ready(const ros::Duration &period);
  virtual void push(const ros::Time &time, const ros::Duration &period);
  virtual void block(const ros::Time &time, const ros::Duration &period);
  virtual void stop(const ros::Time &time, const ros::Duration &period) {};
  virtual void magazine(const ros::Time &time, const ros::Duration &period);
  virtual void moveMagazineJoint(const ros::Duration &period);
  virtual void moveJoint(const ros::Duration &period) = 0;
  virtual void commandCB(const rm_msgs::ShootCmdConstPtr &msg);
  virtual void reconfigCB(rm_shooter_controllers::ShooterBaseConfig &config, uint32_t /*level*/);

  std::vector<hardware_interface::JointHandle> joint_friction_vector_{}, joint_trigger_vector_{};
  std::vector<control_toolbox::Pid> pid_friction_vector_{}, pid_trigger_vector_{};
  hardware_interface::JointHandle joint_magazine_{};
  control_toolbox::Pid pid_magazine_{};

  double friction_qd_des_{}, trigger_q_des_{}, last_trigger_q_des_{}, magazine_q_des_{};
  double enter_push_qd_coef_{}, push_angle_error_{};
  bool dynamic_reconfig_initialized_ = false;
  bool state_changed_ = false;
  bool is_out_from_block_ = false;

  Block *block_{};
  ros::Time last_shoot_time_;

  State state_ = PASSIVE;
  MagazineState magazine_state_ = MAGAZINE_PASSIVE;
  Config config_{};
  realtime_tools::RealtimeBuffer<Config> config_rt_buffer;
  realtime_tools::RealtimeBuffer<rm_msgs::ShootCmd> cmd_rt_buffer_;
  rm_msgs::ShootCmd cmd_;
  ros::Subscriber cmd_subscriber_;
  dynamic_reconfigure::Server<rm_shooter_controllers::ShooterBaseConfig> *d_srv_{};
};

} // namespace rm_shooter_base
#endif //RM_SHOOTER_CONTROLLERS_INCLUDE_RM_SHOOTER_CONTROLLER_SHOOTER_BASE_H_
