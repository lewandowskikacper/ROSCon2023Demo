/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root
 * of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WrapperComponent.h"
#include "AzCore/Debug/Trace.h"
#include "AzCore/Math/MathUtils.h"
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <ROS2/ROS2Bus.h>
#include <ROS2/Sensor/ROS2SensorComponent.h>
#include <ROS2/Utilities/ROS2Conversions.h>
#include <ROS2/Utilities/ROS2Names.h>
#include <ROS2/Frame/ROS2FrameComponent.h>
#include <Include/Integration/SimpleMotionComponentBus.h>
#include <imgui/imgui.h>

namespace ROS2::Demo
{
    void WrapperComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
    }

    void WrapperComponent::Deactivate()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }   

    void WrapperComponent::StartAnimation()
    {
        AZ_Info("WrapperComponent", "Starting animation");


        EMotionFX::Integration::SimpleMotionComponentRequestBus::Event(GetEntityId(), &EMotionFX::Integration::SimpleMotionComponentRequests::PlayMotion);

        // AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);
    }

    void WrapperComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<WrapperComponent>()->Version(1)
                ->Field("IdealVision", &WrapperComponent::m_idealVision)
                ->Field("Pallet", &WrapperComponent::m_pallet)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<WrapperComponent>("WrapperComponent", "WrapperComponent.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "WrapperComponent")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::Category, "ROS2::Demo")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WrapperComponent::m_idealVision, "Ideal Vision", "Ideal Vision used for robot detection")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WrapperComponent::m_pallet, "Pallet to replace", "Pallet to replace")
                    ;
            }
        }
    }

    void WrapperComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // required.push_back(AZ_CRC("IdealVisionSystem"));
        required.push_back(AZ_CRC("EMotionFXSimpleMotionService", 0xea7a05d8));
    }

    void WrapperComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        float progress = -1.0f;
        EMotionFX::Integration::SimpleMotionComponentRequestBus::EventResult(progress, GetEntityId(), &EMotionFX::Integration::SimpleMotionComponentRequests::GetPlayTime);

        float duration;
        EMotionFX::Integration::SimpleMotionComponentRequestBus::EventResult(duration, GetEntityId(), &EMotionFX::Integration::SimpleMotionComponentRequests::GetDuration);


        if (progress == 0.0f)
        {
            return;
        }

        if(progress != -1.0f)
        {
            AZ_Info("WrapperComponent", "Animation in progress %f. Duration: %f", progress, duration);
        }

        if(progress == 1.0f)
        {
            AZ_Info("WrapperComponent", "Animation finished (exact)");


            // if (m_replacePallet)
            // {
            //     auto [scene, handle] = AZ::Interface<AzPhysics::SystemInterface>::Get()->FindAttachedBodyHandleFromEntityId(m_pallet);
                
                
            //     AZ::Interface<AzPhysics::SceneInterface>::Get()->RemoveSimulatedBody(scene, handle);

            //     m_replacePallet = false;
            // }

        } 

        if(AZ::IsClose(progress, 1.0f))
        {
            AZ_Info("WrapperComponent", "Animation finished (close)");
        }
    }

    void WrapperComponent::OnImGuiUpdate()
    {
        ImGui::Begin("WrapperComponent");

        // ImGui::SliderFloat("Target Position", &m_ImGuiPosition, 0.0f, 0.1f);

        if (ImGui::Button("Execute Command"))
        {
            StartAnimation();
            // GripperCommand(m_ImGuiPosition, AZStd::numeric_limits<float>::infinity());
        }

        ImGui::End();
    }


} // namespace ROS2::Demo
