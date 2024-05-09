# Mutable Extension

Solving how to initialize a character adequately with Mutable was a nightmare and required multiple workarounds.

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

This has been pull requested to the engine here: TODO

## Setup

### Create Functions

In your character header:

```cpp
UFUNCTION()
virtual void OnMutableMeshesGenerated();

UFUNCTION()
virtual void OnMutableMeshesUpdated();


virtual TArray<UCustomizableSkeletalComponent*> GatherMutableMeshesToInitialize() const;
```

This has not been tested from Begin Play.

Where your character is ready (generally they have a player controller, player state, and input is setup)

You can call the following:

```cpp
check(GetWorld());
UMutableExtensionSubsystem* Mutable = GetWorld()->GetSubsystem<UMutableExtensionSubsystem>();
check(Mutable);

Mutable->OnAllInstancesInitialized.BindDynamic(this, &ThisClass::OnMutableMeshesGenerated);
Mutable->InitializeMutableComponents(GatherMutableMeshesToInitialize());
```

Implement the functions:

```cpp
void AMyCharacter::OnMutableMeshesGenerated()
{
	check(GetWorld());
	UMutableExtensionSubsystem* Mutable = GetWorld()->GetSubsystem<UMutableExtensionSubsystem>();
	check(Mutable);
	
	Mutable->OnAllComponentsUpdated.BindDynamic(this, &ThisClass::OnMutableMeshesUpdated);
	Mutable->UpdateMutableComponents(GatherMutableMeshesToInitialize(), true, true);
}

void AMyCharacter::OnMutableMeshesUpdated()
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


## Changelog

### 1.0.0
* Release