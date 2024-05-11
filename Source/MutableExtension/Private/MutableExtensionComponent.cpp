﻿// Copyright (c) Jared Taylor. All Rights Reserved


#include "MutableExtensionComponent.h"

#include "MutableFunctionLib.h"
#include "MuCO/CustomizableSkeletalComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(MutableExtensionComponent)

UMutableExtensionComponent::UMutableExtensionComponent()
	: bHasCompletedInitialization(false)
	, bHasCompletedInitialUpdate(false)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
}

void UMutableExtensionComponent::InitializeMutableComponents(TArray<UCustomizableSkeletalComponent*> Components)
{
	if (!ensureAlways(OnAllInstancesInitialized.IsBound()))
	{
		return;
	}

	InstancesPendingInitialization.Reset();
	
	for (UCustomizableSkeletalComponent* Component : Components)
	{
		if (!Component->CustomizableObjectInstance)
		{
			continue;
		}
		
		bool bValid;
		ESkeletalMeshStatus Status = UMutableFunctionLib::GetMutableComponentStatus(Component, bValid);
		if (bValid)
		{
			if (Status == ESkeletalMeshStatus::NotGenerated)
			{
				InstancesPendingInitialization.AddUnique(Component->CustomizableObjectInstance);
				Component->CustomizableObjectInstance->InitialUpdatedDelegate.AddDynamic(this, &ThisClass::OnMutableInstanceInitialized);
			}
		}
	}

	if (InstancesPendingInitialization.Num() == 0)
	{
		CallOnAllInstancesInitialized();
	}
}

void UMutableExtensionComponent::OnMutableInstanceInitialized(UCustomizableObjectInstance* Instance)
{
	if (InstancesPendingInitialization.Contains(Instance))
	{
		InstancesPendingInitialization.Remove(Instance);
	}

	Instance->InitialUpdatedDelegate.RemoveDynamic(this, &ThisClass::OnMutableInstanceInitialized);

	if (InstancesPendingInitialization.Num() == 0)
	{
		CallOnAllInstancesInitialized();
	}
}

void UMutableExtensionComponent::CallOnAllInstancesInitialized()
{
	// Delay by a frame, or it can crash, it isn't fully completed
	FTimerDelegate Delegate;
	Delegate.BindLambda([&]()
	{
		bHasCompletedInitialization = true;
		OnAllInstancesInitialized.ExecuteIfBound();
	});
	GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UMutableExtensionComponent::InitialUpdateMutableComponents(TArray<UCustomizableSkeletalComponent*> Components,
	bool bIgnoreCloseDist, bool bForceHighPriority)
{
	if (!ensureAlways(OnAllInstancesInitialized.IsBound()))
	{
		return;
	}
	
	for (UCustomizableSkeletalComponent* Component : Components)
	{
		FInstanceUpdateDelegate Delegate;
		Delegate.BindDynamic(this, &ThisClass::OnMutableInstanceInitialUpdated);
		if (UMutableFunctionLib::UpdateMutableMesh_Callback(Component, Delegate, bIgnoreCloseDist, bForceHighPriority))
		{
			InstancesPendingInitialUpdate.AddUnique(Component->CustomizableObjectInstance);
		}
	}

	if (InstancesPendingInitialUpdate.Num() == 0)
	{
		CallOnAllComponentsInitialUpdated();
	}
}

void UMutableExtensionComponent::OnMutableInstanceInitialUpdated(const FUpdateContext& Result)
{
	check(Result.Instance);
	
	if (InstancesPendingInitialUpdate.Contains(Result.Instance))
	{
		InstancesPendingInitialUpdate.Remove(Result.Instance);
	}

	if (InstancesPendingInitialUpdate.Num() == 0)
	{
		CallOnAllComponentsInitialUpdated();
	}
}

void UMutableExtensionComponent::CallOnAllComponentsInitialUpdated()
{
	// Delay by a frame, or it can crash, it isn't fully completed
	FTimerDelegate Delegate;
	Delegate.BindLambda([&]()
	{
		bHasCompletedInitialUpdate = true;
		OnAllComponentsInitialUpdated.ExecuteIfBound();
	});
	GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UMutableExtensionComponent::Initialize()
{
	InstancesPendingInitialization.Reset();
	InstancesPendingInitialUpdate.Reset();
}

void UMutableExtensionComponent::Deinitialize()
{
	InstancesPendingInitialization.Empty();		// Clear from memory also
	InstancesPendingInitialUpdate.Empty();				// Clear from memory also
}
