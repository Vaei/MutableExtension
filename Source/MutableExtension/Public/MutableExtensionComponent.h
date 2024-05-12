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

	void ResetMutableInitialization();

public:
	// Begin Initialization

	FOnMutableExtensionSimpleDelegate& RequestMutableInitialization(const TArray<UCustomizableSkeletalComponent*>& MutableComponents);

	const TArray<UCustomizableSkeletalComponent*>& GetMutableInitializingComponents() const { return CachedInitializingComponents; };
	const TArray<UCustomizableObjectInstance*>& GetMutableInitializingInstances() const { return CachedInitializingInstances; };

	/** @return True if nothing is pending initialization and RequestMutableInitialization() was ever called */
	bool HasMutableInitialized() const { return bHasRequestedInitialize && InstancesPendingInitialization.Num() == 0; }

private:
	FOnMutableExtensionSimpleDelegate OnMutableInitialized;

	UPROPERTY()
	TArray<UCustomizableObjectInstance*> InstancesPendingInitialization;

	UPROPERTY()
	TArray<UCustomizableObjectInstance*> CachedInitializingInstances;
	
	UPROPERTY()
	TArray<UCustomizableSkeletalComponent*> CachedInitializingComponents;

	UPROPERTY()
	bool bHasRequestedInitialize = false;

	UFUNCTION()
	void BeginMutableInitialization();
	
	void OnInitializationCompleted();

	// ~End Initialization

public:
	// Begin Runtime Update

	FOnMutableExtensionUpdateDelegate OnComponentRuntimeUpdateCompleted;

	bool RuntimeUpdateMutableComponent(USkeletalMeshComponent* OwningComponent, UCustomizableSkeletalComponent* Component, EMutableExtensionRuntimeUpdateError& Error, bool bIgnoreCloseDist = false, bool bForceHighPriority = false);

	bool IsPendingUpdate(const UCustomizableSkeletalComponent* Component) const;
	bool IsPendingUpdate(const UCustomizableObjectInstance* Instance) const;

	/** @return INCOMPLETE Pending Runtime Update if it exists, otherwise nullptr. */
	const FMutablePendingRuntimeUpdate* GetInstancePendingRuntimeUpdate(const UCustomizableSkeletalComponent* Component) const;

	/** @return INCOMPLETE Pending Runtime Update if it exists, otherwise nullptr. */
	const FMutablePendingRuntimeUpdate* GetInstancePendingRuntimeUpdate(const UCustomizableObjectInstance* Instance) const;

	const TMap<UCustomizableObjectInstance*, FMutablePendingRuntimeUpdate>& GetInstancesPendingRuntimeUpdate() const { return InstancesPendingRuntimeUpdate; };

private:

	UPROPERTY()
	TMap<UCustomizableObjectInstance*, FMutablePendingRuntimeUpdate> InstancesPendingRuntimeUpdate;

	UFUNCTION()
	void OnMutableInstanceRuntimeUpdateCompleted(const FUpdateContext& Result);
	
	UFUNCTION()
	void CallOnComponentRuntimeUpdateCompleted(const FMutablePendingRuntimeUpdate& PendingUpdate) const;

	// ~End Runtime Update
};
