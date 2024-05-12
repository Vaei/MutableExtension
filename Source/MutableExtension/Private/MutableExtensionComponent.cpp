// Copyright (c) Jared Taylor. All Rights Reserved


#include "MutableExtensionComponent.h"

#include "MutableFunctionLib.h"
#include "MuCO/CustomizableObjectInstancePrivate.h"
#include "MuCO/CustomizableSkeletalComponent.h"
#include "MuCO/CustomizableObjectSystemPrivate.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(MutableExtensionComponent)

UMutableExtensionComponent::UMutableExtensionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
}

void UMutableExtensionComponent::ResetMutableInitialization()
{
	bHasRequestedInitialize = false;
	InstancesPendingInitialization.Reset();
	CachedInitializingInstances.Reset();
	CachedInitializingComponents.Reset();
	if (OnMutableInitialized.IsBound())
	{
		OnMutableInitialized.Unbind();
	}
}

FOnMutableExtensionSimpleDelegate& UMutableExtensionComponent::RequestMutableInitialization(
	const TArray<UCustomizableSkeletalComponent*>& MutableComponents)
{
	ResetMutableInitialization();

	bHasRequestedInitialize = true;
	
	for (UCustomizableSkeletalComponent* Component : MutableComponents)
	{
		// Ignore any that have no instance assigned by the user
		UCustomizableObjectInstance* Instance = Component->CustomizableObjectInstance;
		if (Instance)
		{
			Component->CreateCustomizableObjectInstanceUsage();

			CachedInitializingComponents.Add(Component);
			if (!InstancesPendingInitialization.Contains(Instance))
			{
				InstancesPendingInitialization.Add(Instance);
			}
		}
	}
	
	CachedInitializingInstances = InstancesPendingInitialization;

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::BeginMutableInitialization);
	
	return OnMutableInitialized;
}

void UMutableExtensionComponent::BeginMutableInitialization()
{
	for (UCustomizableObjectInstance* Instance : CachedInitializingInstances)
	{
		if (InstancesPendingInitialization.Num() == 0)
		{
			return;
		}
		
		FDelegateHandle Handle = Instance->UpdatedNativeDelegate.AddLambda([&](UCustomizableObjectInstance* UpdatedInstance)
		{
			InstancesPendingInitialization.Remove(UpdatedInstance);
			if (InstancesPendingInitialization.Num() == 0)
			{
				UpdatedInstance->UpdatedNativeDelegate.Remove(Handle);
				OnInitializationCompleted();
			}
		});
		Instance->UpdateSkeletalMeshAsync(true, true);
	}
}

void UMutableExtensionComponent::OnInitializationCompleted()
{
	// Updates are async, so this can be called a second time by another
	if (OnMutableInitialized.IsBound())
	{
		OnMutableInitialized.Execute();
		OnMutableInitialized.Unbind();
	}
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

const FMutablePendingRuntimeUpdate* UMutableExtensionComponent::GetInstancePendingRuntimeUpdate(
	const UCustomizableSkeletalComponent* Component) const
{
	return GetInstancePendingRuntimeUpdate(Component->CustomizableObjectInstance);
}

const FMutablePendingRuntimeUpdate* UMutableExtensionComponent::GetInstancePendingRuntimeUpdate(
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