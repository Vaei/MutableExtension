// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MutableExtensionComponent.generated.h"

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
class MUTABLEEXTENSION_API UMutableExtensionComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UMutableExtensionComponent();
	
public:
	// Begin Initialization

	UPROPERTY(Transient, DuplicateTransient)
	bool bHasCompletedInitialization;

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

	UPROPERTY(Transient, DuplicateTransient)
	bool bHasCompletedUpdate;

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

	// Begin Initialization - You probably don't need to use these
	
	void Initialize();
	void Deinitialize();
	
	// ~End Initialization
};
