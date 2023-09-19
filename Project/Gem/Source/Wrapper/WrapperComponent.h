/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root
 * of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzCore/Component/EntityId.h"
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <ROS2/Camera/CameraCalibrationRequestBus.h>
#include <ROS2/Sensor/ROS2SensorComponent.h>
#include <geometry_msgs/msg/pose_array.hpp>
#include <rclcpp/publisher.hpp>
#include <vision_msgs/msg/detection2_d_array.hpp>
#include <vision_msgs/msg/detection3_d_array.hpp>
#include <ImGuiBus.h>

namespace ROS2::Demo
{
    class WrapperComponent
        : public AZ::Component, protected AZ::TickBus::Handler
        , public ImGui::ImGuiUpdateListenerBus::Handler
    {
    public:
        AZ_COMPONENT(WrapperComponent, "{bf53e0e8-2d4a-466b-9546-e09eeff28f3f}");
        WrapperComponent() = default;
        ~WrapperComponent() = default;
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        void OnImGuiUpdate() override;

        AZ::EntityId m_idealVision;


        bool m_replacePallet = true;
        AZ::EntityId m_pallet;
    
    private:
        void StartAnimation();
    };
} // namespace ROS2::Demo
