#include <memory>

#include <chrono>
#include <rclcpp/rclcpp.hpp>
#include <thread>
// #include <rcutils/logging.h>
#include <control_msgs/action/gripper_command.hpp>
#include <tf2_msgs/msg/tf_message.hpp>

#include <moveit/move_group_interface/move_group_interface.h>
#include <moveit/planning_scene_interface/planning_scene_interface.h>
#include <moveit_visual_tools/moveit_visual_tools.h>

#include "tf2_ros/transform_listener.h"

#include <iostream>

bool SendGripperGrip(rclcpp_action::Client<control_msgs::action::GripperCommand>::SharedPtr client_ptr);

bool SendGripperrelease(rclcpp_action::Client<control_msgs::action::GripperCommand>::SharedPtr client_ptr);

geometry_msgs::msg::Pose getBoxLocation(rclcpp::Node::SharedPtr node, tf2_ros::Buffer& tf_buffer_, std::string box_name = "Box3/box")
{
    geometry_msgs::msg::TransformStamped box_transform;
    try
    {
        box_transform = tf_buffer_.lookupTransform("Box1/odom", "Box1/box", tf2::TimePointZero);
    } catch (const tf2::TransformException& ex)
    {
        std::cerr << "Could not transform " << ex.what() << std::endl;
        std::abort();
    }
    geometry_msgs::msg::Pose box_pose;
    box_pose.position.x = box_transform.transform.translation.x;
    box_pose.position.y = box_transform.transform.translation.y;
    box_pose.position.z = box_transform.transform.translation.z;
    box_pose.orientation.x = box_transform.transform.rotation.x;
    box_pose.orientation.y = box_transform.transform.rotation.y;
    box_pose.orientation.z = box_transform.transform.rotation.z;
    box_pose.orientation.w = box_transform.transform.rotation.w;
    return box_pose;
}

Eigen::Quaterniond fromMsg(const geometry_msgs::msg::Quaternion& msg)
{
    Eigen::Quaterniond q;
    q.x() = msg.x;
    q.y() = msg.y;
    q.z() = msg.z;
    q.w() = msg.w;
    return q;
}

Eigen::Vector3d fromMsg(const geometry_msgs::msg::Point msg)
{
    Eigen::Vector3d v{ msg.x, msg.y, msg.z };
    return v;
}
Eigen::Vector3d fromMsg(const geometry_msgs::msg::Vector3 msg)
{
    Eigen::Vector3d v{ msg.x, msg.y, msg.z };
    return v;
}

const geometry_msgs::msg::Point toMsgPoint(Eigen::Vector3d v)
{
    geometry_msgs::msg::Point msg;
    msg.x = v.x();
    msg.y = v.y();
    msg.z = v.z();
    return msg;
}

const geometry_msgs::msg::Quaternion toMsg(const Eigen::Quaterniond& q)
{
    geometry_msgs::msg::Quaternion msg;
    msg.x = q.x();
    msg.y = q.y();
    msg.z = q.z();
    msg.w = q.w();
    return msg;
}

moveit_msgs::msg::CollisionObject CreateBoxCollision(
    const std::string& name,
    const Eigen::Vector3d dimension,
    const Eigen::Vector3d location,
    const Eigen::Quaterniond& rot = Eigen::Quaterniond::Identity())
{
    moveit_msgs::msg::CollisionObject collision_object;
    collision_object.header.frame_id = "world";
    collision_object.id = name;
    shape_msgs::msg::SolidPrimitive primitive;
    primitive.type = primitive.BOX;
    primitive.dimensions.push_back(dimension.x());
    primitive.dimensions.push_back(dimension.y());
    primitive.dimensions.push_back(dimension.z());
    collision_object.primitives.push_back(primitive);
    geometry_msgs::msg::Pose box_pose;
    box_pose.position = toMsgPoint(location);
    box_pose.orientation = toMsg(rot);
    collision_object.primitive_poses.push_back(box_pose);
    collision_object.operation = collision_object.ADD;
    return collision_object;
}

constexpr float BoxHeight = 0.3f;
const Eigen::Vector3d TableDimension{ 0.950, 0.950, 0.411 };
const Eigen::Vector3d ConveyorDimensions{ 2.0, 0.7, 0.15 };
const Eigen::Vector3d PickupLocation{ 0.890, 0, 0.049 };
const Eigen::Vector3d BoxDimension{ 0.2, 0.2, 0.2 };

// Robot configuration to pickup

// const  std::map<std::string, double> PickupConfig
//     {
//         {"wrist_1_joint", -0.8600329160690308},
//         {"wrist_2_joint", 1.563552975654602},
//         {"elbow_joint", -1.6101552248001099},
//         {"shoulder_pan_joint", -2.6487786769866943},
//         {"shoulder_lift_joint", -2.2290878295898438},
//         {"wrist_3_joint", -5.687946796417236}
//     };

const std::map<std::string, double> PickupConfig{
    { "wrist_1_joint", -0.8385483622550964 },      { "wrist_2_joint", 1.5643877983093262 },       { "elbow_joint", -1.550349235534668 },
    { "shoulder_pan_joint", -2.7139534950256348 }, { "shoulder_lift_joint", -2.314471483230591 }, { "wrist_3_joint", -5.752989768981934 }
};

const std::map<std::string, double> LiftConfig{
    { "wrist_1_joint", -1.6993759870529175 },     { "wrist_2_joint", 1.5634684562683105 },        { "elbow_joint", -1.0284128189086914 },
    { "shoulder_pan_joint", -2.644500255584717 }, { "shoulder_lift_joint", -1.9712634086608887 }, { "wrist_3_joint", -5.683835983276367 }
};
const std::map<std::string, double> DropConfig{
    { "wrist_1_joint", -1.5789473056793213 },       { "wrist_2_joint", 1.5672444105148315 },       { "elbow_joint", -0.9531064033508301 },
    { "shoulder_pan_joint", -0.15679530799388885 }, { "shoulder_lift_joint", -2.199610710144043 }, { "wrist_3_joint", -3.1959822177886963 }
};


const Eigen::Vector3d DropLocation {-1.0, 0.0, 0.420};
const Eigen::Quaterniond DropOrientation {0.0, -0.0, 0.713,  -0.701};

constexpr double BoxSize = 0.12;
const std::vector<Eigen::Vector3d> Pattern{
    { -1, -1, -2.0 }, { -1, 1, -2.0 }, { 1, -1, -2.0 }, { 1, 1, -2.0 },

    { 0, 0, 0 },

};

bool PlanAndGo(
    moveit::planning_interface::MoveGroupInterface& move_group_interface,
    const std::map<std::string, double>& config,
    moveit_visual_tools::MoveItVisualTools& moveit_visual_tools)
{
    move_group_interface.setJointValueTarget(config);

    using moveit::planning_interface::MoveGroupInterface;
    moveit::planning_interface::MoveGroupInterface::Plan plan;
    auto isOk = move_group_interface.plan(plan);
    if (!isOk)
    {
        return false;
    }
    moveit_visual_tools.prompt("  ");
    moveit_visual_tools.trigger();
    auto successExecution = move_group_interface.execute(plan);

    if (!successExecution)
    {
        return false;
    }
    return true;
}

bool PlanAndGo(
    moveit::planning_interface::MoveGroupInterface& move_group_interface, moveit_visual_tools::MoveItVisualTools& moveit_visual_tools)
{
    using moveit::planning_interface::MoveGroupInterface;
    moveit::planning_interface::MoveGroupInterface::Plan plan;
    auto isOk = move_group_interface.plan(plan);
    if (!isOk)
    {
        return false;
    }
    moveit_visual_tools.prompt("  ");
    moveit_visual_tools.trigger();
    auto successExecution = move_group_interface.execute(plan);

    if (!successExecution)
    {
        return false;
    }
    return true;
}

bool PlanAndGo(
    moveit::planning_interface::MoveGroupInterface& move_group_interface,
    Eigen::Vector3d position,
    Eigen::Quaterniond orientation,
    moveit_visual_tools::MoveItVisualTools& moveit_visual_tools)
{
    using moveit::planning_interface::MoveGroupInterface;

    geometry_msgs::msg::Pose pose;
    pose.position = toMsgPoint(position);
    // move_group_interface.getCurrentPose().orientation;
    pose.orientation = move_group_interface.getCurrentPose().pose.orientation;//toMsg(orientation);
    // move_group_interface.setApproximateJointValueTarget(pose, "tool0");
    move_group_interface.setPoseTarget(pose);

    moveit::planning_interface::MoveGroupInterface::Plan plan;
    auto isOk = move_group_interface.plan(plan);
    if (!isOk)
    {
        return false;
    }
    moveit_visual_tools.prompt("  ");
    moveit_visual_tools.trigger();
    // std::this_thread::sleep_for(std::chrono::seconds(4));
    auto successExecution = move_group_interface.execute(plan);

    if (!successExecution)
    {
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    // Initialize ROS and create the Node
    rclcpp::init(argc, argv);
    auto const node =
        std::make_shared<rclcpp::Node>("hello_moveit", rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true));

    // Create a ROS logger
    auto const logger = rclcpp::get_logger("ur_moveit_demo");

    // Spin up a SingleThreadedExecutor for MoveItVisualTools to interact with ROS
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    auto spinner = std::thread(
        [&executor]()
        {
            executor.spin();
        });

    robot_model_loader::RobotModelLoader robot_model_loader(node);
    const moveit::core::RobotModelPtr& kinematic_model = robot_model_loader.getModel();
    RCLCPP_INFO(logger, "Model frame: %s", kinematic_model->getModelFrame().c_str());

    moveit::core::RobotStatePtr robot_state(new moveit::core::RobotState(kinematic_model));
    robot_state->setToDefaultValues();
    const moveit::core::JointModelGroup* joint_model_group = kinematic_model->getJointModelGroup("ur_manipulator");

    const std::vector<std::string>& joint_names = joint_model_group->getVariableNames();

    for (auto joint_name : joint_names)
    {
        RCLCPP_INFO(logger, "Joint %s", joint_name.c_str());
    }

    // Create the MoveIt MoveGroup Interface
    using moveit::planning_interface::MoveGroupInterface;
    auto move_group_interface = MoveGroupInterface(node, "ur_manipulator");

    auto moveit_visual_tools = moveit_visual_tools::MoveItVisualTools{
        node, "base_link", rviz_visual_tools::RVIZ_MARKER_TOPIC, move_group_interface.getRobotModel()
    };
    moveit_visual_tools.loadRemoteControl();
    moveit_visual_tools.trigger();

    auto client_ptr = rclcpp_action::create_client<control_msgs::action::GripperCommand>(node, "/gripper_server");

    moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
    planning_scene_interface.applyCollisionObject(
        CreateBoxCollision("table", TableDimension, Eigen::Vector3d{ 0, 0, -TableDimension.z() / 2.0 }));
    planning_scene_interface.applyCollisionObject(
        CreateBoxCollision("conveyor", ConveyorDimensions, Eigen::Vector3d{ ConveyorDimensions.x() / 2 + 0.75, 0, -0.2 }));

    move_group_interface.clearPathConstraints();
    move_group_interface.clearPoseTargets();
    move_group_interface.allowReplanning(true);
    move_group_interface.setPlanningTime(10);
    move_group_interface.setNumPlanningAttempts(10);
    move_group_interface.setMaxVelocityScalingFactor(0.5);
    move_group_interface.setMaxAccelerationScalingFactor(0.5);

    move_group_interface.setGoalTolerance(0.001);
    move_group_interface.setGoalOrientationTolerance(0.001);
    move_group_interface.setGoalJointTolerance(0.001);
    move_group_interface.setJointValueTarget(PickupConfig);
    move_group_interface.startStateMonitor();

    for (auto offset : Pattern)
    {
        if (!PlanAndGo(move_group_interface, LiftConfig, moveit_visual_tools))
        {
            RCLCPP_ERROR(logger, "Execution failed!");
            rclcpp::shutdown();
            spinner.join();
            return 0;
        }
        //

        if (!PlanAndGo(move_group_interface, PickupConfig, moveit_visual_tools))
        {
            RCLCPP_ERROR(logger, "Execution failed!");
            rclcpp::shutdown();
            spinner.join();
            return 0;
        }

        SendGripperGrip(client_ptr);
        if (!PlanAndGo(move_group_interface, LiftConfig, moveit_visual_tools))
        {
            RCLCPP_ERROR(logger, "Execution failed!");
            rclcpp::shutdown();
            spinner.join();
            return 0;
        }

        if (!PlanAndGo(move_group_interface, DropConfig, moveit_visual_tools))
        {
            RCLCPP_ERROR(logger, "Execution failed!");
            rclcpp::shutdown();
            spinner.join();
            return 0;
        }

        moveit_msgs::msg::PositionConstraint box_constraint;
        box_constraint.header.frame_id = move_group_interface.getPoseReferenceFrame();
        box_constraint.link_name = move_group_interface.getEndEffectorLink();
        shape_msgs::msg::SolidPrimitive box;
        box.type = shape_msgs::msg::SolidPrimitive::BOX;
        box.dimensions = { 2.5, 4, 4 };
        // box.dimensions = { 100, 100, 100 };
        box_constraint.constraint_region.primitives.emplace_back(box);

        geometry_msgs::msg::Pose box_pose;
        box_pose.position.x = -1;
        box_pose.position.y = 0;
        box_pose.position.z = 0;
        box_pose.orientation.w = 1.0;
        box_constraint.constraint_region.primitive_poses.emplace_back(box_pose);
        box_constraint.weight = 1.0;

        Eigen::Vector3d box_point_1(
            box_pose.position.x - box.dimensions[0] / 2,
            box_pose.position.y - box.dimensions[1] / 2,
            box_pose.position.z - box.dimensions[2] / 2);
        Eigen::Vector3d box_point_2(
            box_pose.position.x + box.dimensions[0] / 2,
            box_pose.position.y + box.dimensions[1] / 2,
            box_pose.position.z + box.dimensions[2] / 2);
        moveit_visual_tools.publishCuboid(box_point_1, box_point_2, rviz_visual_tools::TRANSLUCENT_DARK);
        moveit_visual_tools.trigger();
        moveit_visual_tools.prompt("  ");
        moveit_visual_tools.trigger();

        moveit_msgs::msg::Constraints box_constraints;
        box_constraints.position_constraints.emplace_back(box_constraint);
        // move_group_interface.setPathConstraints(box_constraints);
        moveit_visual_tools.trigger();
        moveit_visual_tools.prompt("  ");
        moveit_visual_tools.trigger();

        Eigen::Vector3d modUp = (offset * BoxSize);
        modUp.z() = 0;
        if (!PlanAndGo(move_group_interface, DropLocation + modUp, DropOrientation, moveit_visual_tools))
        {
            RCLCPP_ERROR(logger, "Execution failed!");
            rclcpp::shutdown();
            spinner.join();
            return 0;
        }
        Eigen::Vector3d modDn = (offset * BoxSize);

        if (!PlanAndGo(move_group_interface, DropLocation + modDn, DropOrientation, moveit_visual_tools))
        {
            RCLCPP_ERROR(logger, "Execution failed!");
            rclcpp::shutdown();
            spinner.join();
            return 0;
        }

        SendGripperrelease(client_ptr);
    }
    if (!PlanAndGo(move_group_interface, LiftConfig, moveit_visual_tools))
    {
        RCLCPP_ERROR(logger, "Execution failed!");
        rclcpp::shutdown();
        spinner.join();
        return 0;
    }

    // Shutdown ROS
    rclcpp::shutdown(); // <--- This will cause the spin function in the thread to return
    spinner.join(); // <--- Join the thread before exiting
    return 0;
}

bool Grip(rclcpp_action::Client<control_msgs::action::GripperCommand>::SharedPtr client_ptr, bool grip)
{
    auto goal = control_msgs::action::GripperCommand::Goal();
    goal.command.position = grip ? 0.0 : 1.0;
    goal.command.max_effort = 10000.0;
    auto future = client_ptr->async_send_goal(goal);

    future.wait();

    auto goal_handle = future.get();

    auto result_future = client_ptr->async_get_result(goal_handle);

    result_future.wait();

    auto result = result_future.get().result;

    return result->reached_goal;
}

bool SendGripperGrip(rclcpp_action::Client<control_msgs::action::GripperCommand>::SharedPtr client_ptr)
{
    return Grip(client_ptr, true);
}

bool SendGripperrelease(rclcpp_action::Client<control_msgs::action::GripperCommand>::SharedPtr client_ptr)
{
    return Grip(client_ptr, false);
}
