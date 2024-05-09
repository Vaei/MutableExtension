// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MutableExtensionSubsystem.generated.h"

struct FUpdateContext;
class UCustomizableObjectInstance;
class UCustomizableSkeletalComponent;

DECLARE_DYNAMIC_DELEGATE(FOnMutableExtensionSimpleDelegate);

/**
 * Handler for initialization of Mutable components
 * General use-case is a character with various mutable components wanting to initialize after they are
 * initially spawned and wanting to ensure they all follow correct pathing (mutable init is a nightmare)
 */
UCLASS()
class MUTABLEEXTENSION_API UMutableExtensionSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// Begin Initialization

	FOnMutableExtensionSimpleDelegate OnAllInstancesInitialized;
	
	UPROPERTY()
	TArray<UCustomizableObjectInstance*> InstancesPendingInitialization;
	
public:
	void InitializeMutableComponents(TArray<UCustomizableSkeletalComponent*> Components);

	UFUNCTION()
	void OnMutableInstanceInitialized(UCustomizableObjectInstance* Instance);

	UFUNCTION()
	void CallOnAllInstancesInitialized();

	// ~End Initialization

public:
	// Begin Update

	FOnMutableExtensionSimpleDelegate OnAllComponentsUpdated;
	
	UPROPERTY()
	TArray<UCustomizableObjectInstance*> InstancesPendingUpdate;

	void UpdateMutableComponents(TArray<UCustomizableSkeletalComponent*> Components, bool bIgnoreCloseDist = false, bool bForceHighPriority = false);

	UFUNCTION()
	void OnMutableInstanceUpdated(const FUpdateContext& Result);

	UFUNCTION()
	void CallOnAllComponentsUpdated();

	// ~End Update
	
public:
	// Begin UTickableWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// ~End UTickableWorldSubsystem
};
