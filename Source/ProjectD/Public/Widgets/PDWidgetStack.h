// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDWidgetStack.generated.h"

class UPDWidgetStack;
class UOverlay;
class UPDActivatableBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnStackChanged, UPDWidgetStack*);
UCLASS(Abstract)
class PROJECTD_API UPDWidgetStack : public UUserWidget
{
	GENERATED_BODY()

public:
	UPDActivatableBase* Push(TSubclassOf<UPDActivatableBase> ScreenClass);

	void Pop();

	void Clear();

	UPDActivatableBase* GetTop() const;

	int32 Num() const { return Stack.Num(); }
	bool IsEmpty() const { return Stack.Num() == 0; }

	// 스택이 변경될 때마다 broadcast
	FOnStackChanged OnStackChanged;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> ContentRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDActivatableBase>> Stack;
};