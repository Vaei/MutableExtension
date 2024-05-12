# Mutable Extension

THIS IS NOT SUITABLE FOR PUBLIC USE. ENGINE CHANGES ARE REQUIRED THAT ARE TOO CUMBERSOME TO CONTINUE DOCUMENTING.

DO NOT USE THIS. THIS README IS COMPLETELY OUTDATED NOW.

IF YOU NEED TO USE THIS THEN RAISE AN ISSUE, THERE IS NO POINT UPDATING IT FOR NOBODY ELSE.

Solving how to initialize a character adequately with Mutable was a nightmare and required multiple workarounds.

This plugin is the result of finessing the initialization routing.

C++ Required.

## Pre-Setup

Mutable in it's current state lacks the information we need to initialize altogether.

You must either make the changes in the engine source code or by copy/pasting the plugin out of the engine and into your project and changing it there.

Add the following commits:
* https://github.com/EpicGames/UnrealEngine/pull/11859/commits/f74b6794e7152c7cf71335d38d5e07c96f9e4240
* https://github.com/EpicGames/UnrealEngine/pull/11870/commits/3eafcd0bc80ced3f0fce565b8e3b95efa6aeb39e

## Setup

### Add Component

Add a `UMutableExtensionComponent` to your Character.

### Initialization Functions

In your character header:

```cpp
UFUNCTION()
virtual void OnMutableMeshesGenerated();

UFUNCTION()
virtual void OnMutableMeshesInitialUpdated();


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
			// But we do care if the instance errored or hasn't yet generated
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
	UFUNCTION(BlueprintCallable, Category="Mutable")
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
			if (MutableRuntimeErrorDebugLevel > 0)
			{
				const bool bVerbose = MutableRuntimeErrorDebugLevel > 1;
				FString ErrorString = UMutableFunctionLib::ParseRuntimeUpdateError(Error, bVerbose);
				UE_LOG(LogMyCharacter, Error, TEXT("[ %s ] : %s"), *FString(__FUNCTION__), *ErrorString);
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

### 3.0.1
* Fixed delegate binding issues esp. during editor time
* Reset on EndPlay
* Fixed edge case crash

### 3.0.0
* Reworked initialization completely from scratch
* Engine changes are required, however these have become too cumbersome to update any further


### 2.2.2
* `OnMutableInstanceRuntimeUpdateCompleted` now passes out the update result
* Added `UMutableFunctionLib::GetUpdateResultAsString` for debugging purposes

### 2.2.1
* Fixed crash with dumped data

### 2.2.0
* Improve instance TArray handling to ensure multiple instances aren't being cached or called on
* Additional dumped data
* Fixed bug where `UMutableExtensionComponent::InitialUpdateMutableComponents` was checking for wrong delegate being bound

### 2.1.0
* Add fix for engine bug not generating instances on first run

### 2.0.1
* Add `UMutableFunctionLib::DumpMutableData()` & `UMutableFunctionLib::DumpMutableDataForTargetedActor()`

### 2.0.0
* Refactor `MutableInitializationComponent` ➜ `MutableExtensionComponent`
* Significant code refactor
* Added support for runtime updates ( `UMutableExtensionComponent::RuntimeUpdateMutableComponents()` )

### 1.1.0
* Change to `UActorComponent` instead of Subsystem

### 1.0.0
* Release