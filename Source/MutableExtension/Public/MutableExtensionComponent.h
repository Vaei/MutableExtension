// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MutableExtensionComponent.generated.h"

struct FUpdateContext;
class UCustomizableObjectInstance;
class UCustomizableSkeletalComponent;

DECLARE_DYNAMIC_DELEGATE(FOnMutableExtensionSimpleDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnMutableExtensionUpdateDelegate, UCustomizableObjectInstance*, Instance);

/**
 * Handler for initialization of Mutable components
 * General use-case is a character with various mutable components wanting to initialize after they are
 * initially spawned and wanting to ensure they all follow correct pathing (mutable init is a nightmare)
 *
 * INITIALIZATION FLOW:
 * [ On My Character Ready ] ➜ InitializeMutableComponents() ➜ InitialUpdateMutableComponents()
 *
 * RUNTIME FLOW: 
 * [ When Mesh Changed]  ➜ RuntimeUpdateMutableComponents()
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

	FOnMutableExtensionSimpleDelegate OnAllInstancesInitializeCompleted;
	
	UPROPERTY()
	TArray<UCustomizableObjectInstance*> InstancesPendingInitialization;
	
public:
	void InitializeMutableComponents(TArray<UCustomizableSkeletalComponent*> Components);

	UFUNCTION()
	void OnMutableInstanceInitializeCompleted(UCustomizableObjectInstance* Instance);

	UFUNCTION()
	void CallOnAllInstancesInitialized();

	// ~End Initialization

public:
	// Begin Initial Update

	UPROPERTY(Transient, DuplicateTransient)
	bool bHasCompletedInitialUpdate;

	FOnMutableExtensionSimpleDelegate OnAllComponentsInitialUpdateCompleted;
	
	UPROPERTY()
	TArray<UCustomizableObjectInstance*> InstancesPendingInitialUpdate;

	void InitialUpdateMutableComponents(TArray<UCustomizableSkeletalComponent*> Components, bool bIgnoreCloseDist = false, bool bForceHighPriority = false);

	UFUNCTION()
	void OnMutableInstanceInitialUpdateCompleted(const FUpdateContext& Result);

	UFUNCTION()
	void CallOnAllComponentsInitialUpdated();

	// ~End Initial Update

public:
	
public:

	// Begin Initialization - You probably don't need to use these
	
	void Initialize();
	void Deinitialize();
	
	// ~End Initialization
};
