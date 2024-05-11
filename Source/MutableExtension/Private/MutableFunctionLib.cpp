// Copyright (c) Jared Taylor. All Rights Reserved


#include "MutableFunctionLib.h"

#include "MutableExtensionTypes.h"
#include "MuCO/CustomizableObjectInstancePrivate.h"
#include "MuCO/CustomizableSkeletalComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MutableFunctionLib)

ESkeletalMeshStatus UMutableFunctionLib::GetMutableComponentStatus(
	const UCustomizableSkeletalComponent* Component, bool& bValidResult)
{
	bValidResult = false;
	if (!Component)
	{
		return ESkeletalMeshStatus::Error;
	}
	
	UCustomizableObjectInstance* Instance = Component->CustomizableObjectInstance;
	UCustomizableInstancePrivate* PrivateInstance = Instance ? Instance->GetPrivate() : nullptr;
	if (PrivateInstance)
	{
		bValidResult = true;
		return PrivateInstance->SkeletalMeshStatus;
	}

	return ESkeletalMeshStatus::Error;
}

void UMutableFunctionLib::ErrorOnFailedValidation(const UCustomizableSkeletalComponent* MutableMesh)
{
	FMessageLog MessageLog {"PIE"};
	const FString ErrorString = FString::Printf(TEXT("[ %s ] : { %s } failed IsMutableMeshValidToUpdate() - you must always run this check first. Mutable is now unstable, this must be resolved."), *FString(__FUNCTION__), *GetNameSafe(MutableMesh));
	MessageLog.Error(FText::FromString(ErrorString));
}

bool UMutableFunctionLib::IsMutableMeshValidToUpdate(const UCustomizableSkeletalComponent* MutableMesh)
{
	UCustomizableInstancePrivate* PrivateInstance = MutableMesh && MutableMesh->CustomizableObjectInstance ?
		MutableMesh->CustomizableObjectInstance->GetPrivate() : nullptr;

	return PrivateInstance ? PrivateInstance->SkeletalMeshStatus == ESkeletalMeshStatus::Success : false;
}

void UMutableFunctionLib::UpdateMutableMesh(const UCustomizableSkeletalComponent* MutableMesh, bool bIgnoreCloseDist,
	bool bForceHighPriority)
{
	if (!IsMutableMeshValidToUpdate(MutableMesh))
	{
		ErrorOnFailedValidation(MutableMesh);
		return;
	}
	
	MutableMesh->CustomizableObjectInstance->UpdateSkeletalMeshAsync(bIgnoreCloseDist, bForceHighPriority);
}

void UMutableFunctionLib::UpdateMutableMesh_Callback(const UCustomizableSkeletalComponent* MutableMesh,
	const FInstanceUpdateDelegate& InstanceUpdateDelegate, bool bIgnoreCloseDist, bool bForceHighPriority)
{
	if (!IsMutableMeshValidToUpdate(MutableMesh))
	{
		ErrorOnFailedValidation(MutableMesh);
	}
	
	MutableMesh->CustomizableObjectInstance->UpdateSkeletalMeshAsyncResult(InstanceUpdateDelegate,
		bIgnoreCloseDist, bForceHighPriority);
}

FString UMutableFunctionLib::ParseRuntimeUpdateError(const EMutableExtensionRuntimeUpdateError& Error, bool bVerbose)
{
	return bVerbose ? ParseRuntimeUpdateError_Verbose(Error) : ParseRuntimeUpdateError_Simple(Error);
}

FString UMutableFunctionLib::ParseRuntimeUpdateError_Simple(const EMutableExtensionRuntimeUpdateError& Error)
{
	switch(Error)
	{
	case EMutableExtensionRuntimeUpdateError::DelegateNotBound:
		return "Delegate Not Bound";
	case EMutableExtensionRuntimeUpdateError::AlreadyPendingUpdate:
		return "Update Already Pending";
	case EMutableExtensionRuntimeUpdateError::MeshNotValidToUpdate:
		return "Mesh Not Valid To Update";
	default:
		return "None";
	}
}

FString UMutableFunctionLib::ParseRuntimeUpdateError_Verbose(const EMutableExtensionRuntimeUpdateError& Error)
{
	switch(Error)
	{
	case EMutableExtensionRuntimeUpdateError::DelegateNotBound:
		return "Delegate Not Bound: UMutableExtensionComponent::OnComponentRuntimeUpdateCompleted";
	case EMutableExtensionRuntimeUpdateError::AlreadyPendingUpdate:
		return "Update Already Pending: Call UMutableExtensionComponent::IsPendingUpdate() before UMutableExtensionComponent::RuntimeUpdateMutableComponent";
	case EMutableExtensionRuntimeUpdateError::MeshNotValidToUpdate:
		return "Mesh Not Valid To Update: SkeletalMeshStatus != Success, therefore it either errored or has not yet been generated. Call UMutableFunctionLib::IsMutableMeshValidToUpdate() before UMutableExtensionComponent::RuntimeUpdateMutableComponent";
	default:
		return "None";
	}
}
