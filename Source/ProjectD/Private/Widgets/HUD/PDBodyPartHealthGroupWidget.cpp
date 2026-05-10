// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDBodyPartHealthGroupWidget.h"

#include "Widgets/HUD/PDAttributeBarWidget.h"

UPDAttributeBarWidget* UPDBodyPartHealthGroupWidget::GetBar(EBodyPart Part) const
{
	switch (Part)
	{
	case EBodyPart::Head:  return Bar_Head;
	case EBodyPart::Torso: return Bar_Torso;
	case EBodyPart::Arm_L: return Bar_ArmL;
	case EBodyPart::Arm_R: return Bar_ArmR;
	case EBodyPart::Leg_L: return Bar_LegL;
	case EBodyPart::Leg_R: return Bar_LegR;
	case EBodyPart::None:
	default:
		return nullptr;
	}
}
