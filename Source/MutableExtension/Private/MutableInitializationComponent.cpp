﻿// Copyright (c) Jared Taylor. All Rights Reserved


#include "MutableInitializationComponent.h"

#include "MutableFunctionLib.h"
#include "MuCO/CustomizableSkeletalComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(MutableInitializationComponent)

UMutableInitializationComponent::UMutableInitializationComponent()
	: bHasCompletedInitialization(false)
	, bHasCompletedUpdate(false)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
}

void UMutableInitializationComponent::InitializeMutableComponents(TArray<UCustomizableSkeletalComponent*> Components)
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
				Component->CustomizableObjectInstance->UpdatedDelegate.AddDynamic(this, &ThisClass::OnMutableInstanceInitialized);
			}
		}
	}

	if (InstancesPendingInitialization.Num() == 0)
	{
		CallOnAllInstancesInitialized();
	}
}

void UMutableInitializationComponent::OnMutableInstanceInitialized(UCustomizableObjectInstance* Instance)
{
	if (InstancesPendingInitialization.Contains(Instance))
	{
		InstancesPendingInitialization.Remove(Instance);
	}

	Instance->UpdatedDelegate.RemoveDynamic(this, &ThisClass::OnMutableInstanceInitialized);

	if (InstancesPendingInitialization.Num() == 0)
	{
		CallOnAllInstancesInitialized();
	}
}

void UMutableInitializationComponent::CallOnAllInstancesInitialized()
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

void UMutableInitializationComponent::UpdateMutableComponents(TArray<UCustomizableSkeletalComponent*> Components,
	bool bIgnoreCloseDist, bool bForceHighPriority)
{
	if (!ensureAlways(OnAllInstancesInitialized.IsBound()))
	{
		return;
	}
	
	for (UCustomizableSkeletalComponent* Component : Components)
	{
		FInstanceUpdateDelegate Delegate;
		Delegate.BindDynamic(this, &ThisClass::OnMutableInstanceUpdated);
		if (UMutableFunctionLib::UpdateMutableMesh_Callback(Component, Delegate, bIgnoreCloseDist, bForceHighPriority))
		{
			InstancesPendingUpdate.AddUnique(Component->CustomizableObjectInstance);
		}
	}

	if (InstancesPendingUpdate.Num() == 0)
	{
		CallOnAllComponentsUpdated();
	}
}

void UMutableInitializationComponent::OnMutableInstanceUpdated(const FUpdateContext& Result)
{
	check(Result.Instance);
	
	if (InstancesPendingUpdate.Contains(Result.Instance))
	{
		InstancesPendingUpdate.Remove(Result.Instance);
	}

	if (InstancesPendingUpdate.Num() == 0)
	{
		CallOnAllComponentsUpdated();
	}
}

void UMutableInitializationComponent::CallOnAllComponentsUpdated()
{
	// Delay by a frame, or it can crash, it isn't fully completed
	FTimerDelegate Delegate;
	Delegate.BindLambda([&]()
	{
		bHasCompletedUpdate = true;
		OnAllComponentsUpdated.ExecuteIfBound();
	});
	GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UMutableInitializationComponent::Initialize()
{
	InstancesPendingInitialization.Reset();
	InstancesPendingUpdate.Reset();
}

void UMutableInitializationComponent::Deinitialize()
{
	InstancesPendingInitialization.Empty();		// Clear from memory also
	InstancesPendingUpdate.Empty();				// Clear from memory also
}