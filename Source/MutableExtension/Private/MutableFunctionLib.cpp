// Copyright (c) 2024 Studio Titan


#include "MutableFunctionLib.h"

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

bool UMutableFunctionLib::UpdateMutableMesh(const UCustomizableSkeletalComponent* MutableMesh, bool bIgnoreCloseDist,
	bool bForceHighPriority)
{
	if (MutableMesh && MutableMesh->CustomizableObjectInstance)
	{
		ESkeletalMeshStatus Status = MutableMesh->CustomizableObjectInstance->GetPrivate()->SkeletalMeshStatus;
		if (ensure(Status == ESkeletalMeshStatus::Success))
		{
			MutableMesh->CustomizableObjectInstance->UpdateSkeletalMeshAsync(bIgnoreCloseDist, bForceHighPriority);
			return true;
		}
	}
	return false;
}

bool UMutableFunctionLib::UpdateMutableMesh_Callback(const UCustomizableSkeletalComponent* MutableMesh,
	const FInstanceUpdateDelegate& InstanceUpdateDelegate, bool bIgnoreCloseDist, bool bForceHighPriority)
{
	if (MutableMesh && MutableMesh->CustomizableObjectInstance)
	{
		ESkeletalMeshStatus Status = MutableMesh->CustomizableObjectInstance->GetPrivate()->SkeletalMeshStatus;
		if (ensure(Status == ESkeletalMeshStatus::Success))
		{
			MutableMesh->CustomizableObjectInstance->UpdateSkeletalMeshAsyncResult(InstanceUpdateDelegate,
				bIgnoreCloseDist, bForceHighPriority);
			return true;
		}
	}
	return false;
}
