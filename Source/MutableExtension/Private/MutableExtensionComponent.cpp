// Copyright (c) Jared Taylor. All Rights Reserved


#include "MutableExtensionComponent.h"

#include "MutableFunctionLib.h"
#include "MuCO/CustomizableObjectInstancePrivate.h"
#include "MuCO/CustomizableSkeletalComponent.h"
#include "MuCO/CustomizableObjectInstanceDescriptor.h"
#include "MuCO/CustomizableObjectSystemPrivate.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(MutableExtensionComponent)

UMutableExtensionComponent::UMutableExtensionComponent()
	: bHasCompletedInitialization(false)
	, bHasCompletedInitialUpdate(false)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
}

void UMutableExtensionComponent::InitializeMutableComponents(TArray<UCustomizableSkeletalComponent*> Components, bool bInternalFromPreInit)
{
	if (!ensureAlways(OnAllInstancesInitializeCompleted.IsBound()))
	{
		return;
	}

	InstancesPendingInitialization.Reset();
	ComponentsPendingPreInitialization.Reset();

	// Run pre-initialization first to ensure all components are ready to initialize
	for (UCustomizableSkeletalComponent* Component : Components)
	{
		if (ComponentsPendingPreInitialization.Contains(Component))
		{
			continue;
		}

		if (!Component->IsCustomizableObjectReady())
		{
			if (!Component->TryMakeCustomizableObjectReady())
			{
				// Listen for the instance generating
				ComponentsPendingPreInitialization.Add(Component);
				Component->CustomizableObjectReadyDelegate.BindUObject(this, &ThisClass::OnMutableComponentPreInitializeCompleted);
			}
		}
	}

	if (ComponentsPendingPreInitialization.Num() > 0)
	{
		// Not supported, fix whatever caused it
		check(!bHasCompletedInitialization);
		
		CachedPreInitializeComponents = Components;
		return;
	}

	for (UCustomizableSkeletalComponent* Component : Components)
	{
		if (!Component->CustomizableObjectInstance)
		{
			continue;
		}
		
		check(Component->IsCustomizableObjectReady());

		bool bValid;
		ESkeletalMeshStatus Status = UMutableFunctionLib::GetMutableComponentStatus(Component, bValid);
		if (bValid)
		{
			if (Status == ESkeletalMeshStatus::NotGenerated && Component->CustomizableObjectInstance->CanUpdateInstance())
			{
				if (!InstancesPendingInitialization.Contains(Component->CustomizableObjectInstance))
				{
					// Listen for the instance generating
					InstancesPendingInitialization.Add(Component->CustomizableObjectInstance);
					Component->CustomizableObjectInstance->UpdatedDelegate.AddDynamic(this, &ThisClass::OnMutableInstanceInitializeCompleted);

					// There is a bug with mutable where it simply never generates until you exit PIE and re-PIE after first run
					FMutableInstanceUpdateMap RequestedLODUpdates;
					Component->CustomizableObjectInstance->GetPrivate()->UpdateInstanceIfNotGenerated(*Component->CustomizableObjectInstance, RequestedLODUpdates);
				}
			}
		}
	}

	if (InstancesPendingInitialization.Num() == 0)
	{
		CallOnAllInstancesInitialized();
	}
}

void UMutableExtensionComponent::OnMutableComponentPreInitializeCompleted(UCustomizableSkeletalComponent* Component)
{
	if (ComponentsPendingPreInitialization.Contains(Component))
	{
		check(Component->IsCustomizableObjectReady());
		ComponentsPendingPreInitialization.Remove(Component);

		Component->CustomizableObjectReadyDelegate.Unbind();

		if (ComponentsPendingPreInitialization.Num() == 0)
		{
			InitializeMutableComponents(CachedPreInitializeComponents, true);
			CachedPreInitializeComponents.Empty();
		}
	}
}

void UMutableExtensionComponent::OnMutableInstanceInitializeCompleted(UCustomizableObjectInstance* Instance)
{
	if (InstancesPendingInitialization.Contains(Instance))
	{
		InstancesPendingInitialization.Remove(Instance);

		Instance->UpdatedDelegate.RemoveDynamic(this, &ThisClass::OnMutableInstanceInitializeCompleted);

		if (InstancesPendingInitialization.Num() == 0)
		{
			CallOnAllInstancesInitialized();
		}
	}
}

void UMutableExtensionComponent::CallOnAllInstancesInitialized()
{
	// Delay by a frame, or it can crash, it isn't fully completed
	FTimerDelegate Delegate;
	Delegate.BindLambda([&]()
	{
		bHasCompletedInitialization = true;
		OnAllInstancesInitializeCompleted.ExecuteIfBound();
	});
	GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UMutableExtensionComponent::InitialUpdateMutableComponents(TArray<UCustomizableSkeletalComponent*> Components,
	bool bIgnoreCloseDist, bool bForceHighPriority)
{
	if (!ensureAlways(OnAllComponentsInitialUpdateCompleted.IsBound()))
	{
		return;
	}
	
	for (UCustomizableSkeletalComponent* Component : Components)
	{
		if (!Component || !Component->CustomizableObjectInstance)
		{
			// IsMutableMeshValidToUpdate checks this, but we don't want to fail an ensure
			// unless SkeletalMeshStatus did not succeed (generation failed or has not yet completed)
			continue;
		}
		
		if (!ensure(UMutableFunctionLib::IsMutableMeshValidToUpdate(Component)))
		{
			continue;
		}

		if (!InstancesPendingInitialUpdate.Contains(Component->CustomizableObjectInstance))
		{
			InstancesPendingInitialUpdate.Add(Component->CustomizableObjectInstance);

			FInstanceUpdateDelegate Delegate;
			Delegate.BindDynamic(this, &ThisClass::OnMutableInstanceInitialUpdateCompleted);
			UMutableFunctionLib::UpdateMutableMesh_Callback(Component, Delegate, bIgnoreCloseDist, bForceHighPriority);
		}
	}

	if (InstancesPendingInitialUpdate.Num() == 0)
	{
		CallOnAllComponentsInitialUpdated();
	}
}

void UMutableExtensionComponent::OnMutableInstanceInitialUpdateCompleted(const FUpdateContext& Result)
{
	check(Result.Instance);
	
	if (InstancesPendingInitialUpdate.Contains(Result.Instance))
	{
		InstancesPendingInitialUpdate.Remove(Result.Instance);

		if (InstancesPendingInitialUpdate.Num() == 0)
		{
			CallOnAllComponentsInitialUpdated();
		}
	}
}

void UMutableExtensionComponent::CallOnAllComponentsInitialUpdated()
{
	// Delay by a frame, or it can crash, it isn't fully completed
	FTimerDelegate Delegate;
	Delegate.BindLambda([&]()
	{
		bHasCompletedInitialUpdate = true;
		OnAllComponentsInitialUpdateCompleted.ExecuteIfBound();
	});
	GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
}

bool UMutableExtensionComponent::RuntimeUpdateMutableComponent(USkeletalMeshComponent* OwningComponent,
	UCustomizableSkeletalComponent* Component, EMutableExtensionRuntimeUpdateError& Error, bool bIgnoreCloseDist, bool
	bForceHighPriority)
{
	if (!ensureAlways(OnComponentRuntimeUpdateCompleted.IsBound()))
	{
		Error = EMutableExtensionRuntimeUpdateError::DelegateNotBound;
		return false;
	}

	Error = EMutableExtensionRuntimeUpdateError::None;

	if (IsPendingUpdate(Component))
	{
		// Error instead of doing the check for them, it is likely not intended that they come back here again so soon,
		// so they should handle it themselves
		
		FMessageLog MessageLog {"PIE"};
		const FString ErrorString = FString::Printf(TEXT("[ %s ] { %s } is already pending update. Please call UMutableExtensionComponent::IsPendingUpdate() before making this call."),
			*FString(__FUNCTION__), *Component->CustomizableObjectInstance.GetName());
		MessageLog.Error(FText::FromString(ErrorString));

		Error = EMutableExtensionRuntimeUpdateError::AlreadyPendingUpdate;
		return false;
	}

	if (!UMutableFunctionLib::IsMutableMeshValidToUpdate(Component))
	{
		Error = EMutableExtensionRuntimeUpdateError::MeshNotValidToUpdate;
		return false;
	}

	FMutablePendingRuntimeUpdate PendingUpdate { Component->CustomizableObjectInstance, Component, OwningComponent };
	InstancesPendingRuntimeUpdate.Add(Component->CustomizableObjectInstance, PendingUpdate);
	
	FInstanceUpdateDelegate Delegate;
	Delegate.BindDynamic(this, &ThisClass::OnMutableInstanceRuntimeUpdateCompleted);
	UMutableFunctionLib::UpdateMutableMesh_Callback(Component, Delegate, bIgnoreCloseDist, bForceHighPriority);

	return true;
}

bool UMutableExtensionComponent::IsPendingUpdate(const UCustomizableSkeletalComponent* Component) const
{
	return IsPendingUpdate(Component->CustomizableObjectInstance);
}

bool UMutableExtensionComponent::IsPendingUpdate(const UCustomizableObjectInstance* Instance) const
{
	return InstancesPendingRuntimeUpdate.Contains(Instance);
}

const FMutablePendingRuntimeUpdate* UMutableExtensionComponent::GetIncompletePendingRuntimeUpdate(
	const UCustomizableSkeletalComponent* Component) const
{
	return GetIncompletePendingRuntimeUpdate(Component->CustomizableObjectInstance);
}

const FMutablePendingRuntimeUpdate* UMutableExtensionComponent::GetIncompletePendingRuntimeUpdate(
	const UCustomizableObjectInstance* Instance) const
{
	return InstancesPendingRuntimeUpdate.Find(Instance);
}

void UMutableExtensionComponent::OnMutableInstanceRuntimeUpdateCompleted(const FUpdateContext& Result)
{
	check(Result.Instance);

	if (FMutablePendingRuntimeUpdate* PendingUpdate = InstancesPendingRuntimeUpdate.Find(Result.Instance))
	{
		PendingUpdate->UpdateResult = Result.UpdateResult;

		InstancesPendingRuntimeUpdate.Remove(Result.Instance);
		CallOnComponentRuntimeUpdateCompleted(*PendingUpdate);
	}
}

void UMutableExtensionComponent::CallOnComponentRuntimeUpdateCompleted(const FMutablePendingRuntimeUpdate& PendingUpdate) const
{
	// Delay by a frame to be safe -- can it crash? Not yet tested
	FTimerDelegate Delegate;
	Delegate.BindLambda([&]()
	{
		OnComponentRuntimeUpdateCompleted.ExecuteIfBound(PendingUpdate);
	});
	GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UMutableExtensionComponent::Initialize()
{
	InstancesPendingInitialization.Reset();
	InstancesPendingInitialUpdate.Reset();
	InstancesPendingRuntimeUpdate.Reset();
}

void UMutableExtensionComponent::Deinitialize()
{
	// Clear from memory also ( Empty() )
	InstancesPendingInitialization.Empty();		
	InstancesPendingInitialUpdate.Empty();
	InstancesPendingRuntimeUpdate.Empty();
}
