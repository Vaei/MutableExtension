// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "MutableExtensionTypes.h"
#include "Components/ActorComponent.h"
#include "MutableExtensionComponent.generated.h"

struct FUpdateContext;
class UCustomizableObjectInstance;
class UCustomizableSkeletalComponent;

DECLARE_DYNAMIC_DELEGATE(FOnMutableExtensionSimpleDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnMutableExtensionUpdateDelegate, const FMutablePendingRuntimeUpdate&, Updated);

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

	/** These do not have valid descriptor hashes or object instance usage yet */
	UPROPERTY()
	TArray<UCustomizableSkeletalComponent*> ComponentsPendingPreInitialization;

	UPROPERTY()
	TArray<UCustomizableSkeletalComponent*> CachedPreInitializeComponents;
	
	UPROPERTY()
	TArray<UCustomizableObjectInstance*> InstancesPendingInitialization;
	
public:
	UFUNCTION()
	void InitializeMutableComponents(TArray<UCustomizableSkeletalComponent*> Components, bool bInternalFromPreInit = false);

	UFUNCTION()
	void OnMutableComponentPreInitializeCompleted(UCustomizableSkeletalComponent* Component);
	
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
	// Begin Runtime Update

	FOnMutableExtensionUpdateDelegate OnComponentRuntimeUpdateCompleted;

	UPROPERTY()
	TMap<UCustomizableObjectInstance*, FMutablePendingRuntimeUpdate> InstancesPendingRuntimeUpdate;

	/** @return True if update was requested (was not aborted) */
	bool RuntimeUpdateMutableComponent(USkeletalMeshComponent* OwningComponent, UCustomizableSkeletalComponent* Component, EMutableExtensionRuntimeUpdateError& Error, bool bIgnoreCloseDist = false, bool bForceHighPriority = false);

	bool IsPendingUpdate(const UCustomizableSkeletalComponent* Component) const;
	bool IsPendingUpdate(const UCustomizableObjectInstance* Instance) const;

	/** @return INCOMPLETE Pending Runtime Update if it exists, otherwise nullptr. */
	const FMutablePendingRuntimeUpdate* GetIncompletePendingRuntimeUpdate(const UCustomizableSkeletalComponent* Component) const;
	const FMutablePendingRuntimeUpdate* GetIncompletePendingRuntimeUpdate(const UCustomizableObjectInstance* Instance) const;
	
	UFUNCTION()
	void OnMutableInstanceRuntimeUpdateCompleted(const FUpdateContext& Result);
	
	UFUNCTION()
	void CallOnComponentRuntimeUpdateCompleted(const FMutablePendingRuntimeUpdate& PendingUpdate) const;

	// ~End Runtime Update
	
public:

	// Begin Initialization - You probably don't need to use these
	
	void Initialize();
	void Deinitialize();
	
	// ~End Initialization
};
