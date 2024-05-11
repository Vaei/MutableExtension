# Mutable Extension

Solving how to initialize a character adequately with Mutable was a nightmare and required multiple workarounds.

This plugin is the result of finessing the initialization routing.

C++ Required.

## Pre-Setup

It requires either making an engine source change or copy/pasting the plugin out of the engine into your project.

In `CustomizableObjectInstance.h` find `struct FUpdateContext` and at the bottom add this:

```cpp
/** When iterating multiple instances with callback we need to identify which instance was updated */
UPROPERTY()
UCustomizableObjectInstance* Instance;
```

Then in `CustomizableObjectSystem.cpp` find `FinishUpdateGlobal()` and change it so it does this:
```cpp
	FUpdateContext ContextPublic;
	ContextPublic.UpdateResult = Context->UpdateResult;
	ContextPublic.Instance = Instance;  // We introduce this line
		
	Context->UpdateCallback.ExecuteIfBound(ContextPublic);
	Context->UpdateNativeCallback.Broadcast(ContextPublic);
```

This has been pull requested to the engine here: https://github.com/EpicGames/UnrealEngine/pull/11859

## Setup

### Add Component

Add a `UMutableExtensionComponent` to your Character.

### Initialization Functions

In your character header:

```cpp
UFUNCTION()
virtual void OnMutableMeshesGenerated();

UFUNCTION()
virtual void OnMutableMeshesUpdated();


virtual TArray<UCustomizableSkeletalComponent*> GatherMutableMeshesToInitialize() const;
```

This has not been tested from `BeginPlay()`.

Where your character is ready - generally they have a player controller, player state, and input is setup.

You can call the following:

```cpp
MutableExtension->OnAllInstancesInitializeCompleted.BindDynamic(this, &ThisClass::OnMutableMeshesGenerated);
MutableExtension->InitializeMutableComponents(GatherMutableMeshesToInitialize());
```

Implement the functions:

```cpp
void AMyCharacter::OnMutableMeshesGenerated()
{
	MutableExtension->OnAllComponentsInitialUpdateCompleted.BindDynamic(this, &ThisClass::OnMutableMeshesInitialUpdated);
	MutableExtension->InitialUpdateMutableComponents(GatherMutableMeshesToInitialize(), true, true);
}

void AMyCharacter::OnMutableMeshesInitialUpdated()
{
	// We're ready to go - do whatever you need when the mutable meshes are ready

	// Perhaps show the actor here
	SetActorHiddenInGame(false);

	// Optional error checking
	TArray<UCustomizableSkeletalComponent*> Components = GatherMutableMeshesToInitialize();
	for (UCustomizableSkeletalComponent* Component : Components)
	{
		// We don't care if something we didn't assign an instance for failed
		if (Component->CustomizableObjectInstance)
		{
			bool bValid;
			ESkeletalMeshStatus Status = UMutableFunctionLib::GetMutableComponentStatus(Component, bValid);
			ensure(bValid);
			ensure(Status == ESkeletalMeshStatus::Success);
		}
	}
}

TArray<UCustomizableSkeletalComponent*> AMyCharacter::GatherMutableMeshesToInitialize() const
{
	return { MutableMesh };
}
```

### Runtime Update Functions

This usage is a suggestion rather than a requirement.

Add these functions to your header:

```cpp
	UFUNCTION(BlueprintCallable, Category="Titan|Mutable")
	void RequestMutableRuntimeUpdate(UCustomizableSkeletalComponent* MutableComponent, bool bIgnoreCloseDist = false, bool bForceHighPriority = false);

	UFUNCTION()
	virtual void OnMutableRuntimeUpdateFinished(const FMutablePendingRuntimeUpdate& MutableUpdate);

	UFUNCTION(BlueprintImplementableEvent, Category="Mutable", meta=(DisplayName="On Mutable Runtime Update Finished"))
	void K2_OnMutableRuntimeUpdateFinished(const FMutablePendingRuntimeUpdate& MutableUpdate);
```

Implement:

```cpp
void AMyCharacter::RequestMutableRuntimeUpdate(USkeletalMeshComponent* OwningComponent,
	UCustomizableSkeletalComponent* MutableComponent, bool bIgnoreCloseDist, bool bForceHighPriority)
{
	if (!ensure(MutableComponent))
	{
		// ...?
		return;
	}
	
	if (ensure(MutableExtension))
	{
		if (!MutableExtension->OnComponentRuntimeUpdateCompleted.IsBoundToObject(this))
		{
			MutableExtension->OnComponentRuntimeUpdateCompleted.BindDynamic(this, &ThisClass::OnMutableRuntimeUpdateFinished);
		}

		// Find the OwningComponent automatically -- Mutable components are added directly under skeletal mesh components
		USceneComponent* AttachParent = MutableComponent->GetAttachParent();
		USkeletalMeshComponent* OwningComponent = AttachParent ? Cast<USkeletalMeshComponent>(AttachParent) : nullptr;
		if (!ensure(OwningComponent))
		{
			return;
		}

		EMutableExtensionRuntimeUpdateError Error;
		if (!MutableExtension->RuntimeUpdateMutableComponent(OwningComponent, MutableComponent, Error, bIgnoreCloseDist, bForceHighPriority))
		{
			if (TitanCharacterCVars::MutableRuntimeErrorDebugLevel > 0)
			{
				const bool bVerbose = TitanCharacterCVars::MutableRuntimeErrorDebugLevel > 1;
				FString ErrorString = UMutableFunctionLib::ParseRuntimeUpdateError(Error, bVerbose);
				UE_LOG(LogTitanCharacter, Error, TEXT("[ %s ] : %s"), *FString(__FUNCTION__), *ErrorString);
			}
		}
	}
}

void AMyCharacter::OnMutableRuntimeUpdateFinished(const FMutablePendingRuntimeUpdate& MutableUpdate)
{
	// Do some stuff here, maybe you want to set some UMaterialInstanceDynamic on the MutableUpdate->OwningComponent?
	
#if ENABLE_DRAW_DEBUG
	if (bMutableRuntimeUpdateDrawDebugValues)
	{
		const FString ResultString =
			FString::Printf(TEXT("[ %s ] : OwningComponent : { %s }, MutableComponent : { %s }, MutableInstance : { %s }"),
				*FString(__FUNCTION__), *GetNameSafe(MutableUpdate.OwningComponent), *GetNameSafe(MutableUpdate.MutableComponent),
				*GetNameSafe(MutableUpdate.MutableInstance));
		
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, ResultString);
	}
#endif
}
```

## Changelog

### 2.0.0
* Refactor `MutableInitializationComponent` ➜ `MutableExtensionComponent`
* Significant code refactor
* Added support for runtime updates ( `UMutableExtensionComponent::RuntimeUpdateMutableComponents()` )

### 1.1.0
* Change to `UActorComponent` instead of Subsystem

### 1.0.0
* Release