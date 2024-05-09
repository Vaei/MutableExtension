// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MuCO/CustomizableObjectInstance.h"
#include "MutableFunctionLib.generated.h"

class UCustomizableSkeletalComponent;

#define UPDATE_MUTABLE_MESH(MutableMesh, bIgnoreCloseDist, bForceHighPriority, ...) \
UMutableFunctionLib::UpdateMutableMesh(MutableMesh, bIgnoreCloseDist, bForceHighPriority);

#define UPDATE_MUTABLE_MESH_CALLBACK(MutableMesh, Func, bIgnoreCloseDist, bForceHighPriority, ...) \
FInstanceUpdateDelegate Delegate; \
Delegate.BindDynamic(this, Func); \
UMutableFunctionLib::UpdateMutableMesh_Callback(MutableMesh, Delegate, bIgnoreCloseDist, bForceHighPriority);

/**
 * 
 */
UCLASS()
class MUTABLEEXTENSION_API UMutableFunctionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static ESkeletalMeshStatus GetMutableComponentStatus(const UCustomizableSkeletalComponent* Component, bool& bValidResult);
	
public:
	static bool UpdateMutableMesh(const UCustomizableSkeletalComponent* MutableMesh,
		bool bIgnoreCloseDist = false, bool bForceHighPriority = false);
	
	static bool UpdateMutableMesh_Callback(const UCustomizableSkeletalComponent* MutableMesh,
		const FInstanceUpdateDelegate& InstanceUpdateDelegate,
		bool bIgnoreCloseDist = false, bool bForceHighPriority = false);
};
