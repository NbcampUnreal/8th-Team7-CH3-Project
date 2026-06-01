// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDSessionService.h"

UWorld* UPDSessionService::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}