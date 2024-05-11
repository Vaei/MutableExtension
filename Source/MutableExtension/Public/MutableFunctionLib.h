// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MuCO/CustomizableObjectInstance.h"
#include "MutableFunctionLib.generated.h"

enum class EMutableExtensionRuntimeUpdateError : uint8;
class UCustomizableSkeletalComponent;

/**
 * USAGE:
 *	FInstanceUpdateDelegate Delegate;
 *	Delegate.BindDynamic(this, &ThisClass::OnMutableInstanceUpdated);
 *	if (UMutableFunctionLib::UpdateMutableMesh_Callback(Component, Delegate, bIgnoreCloseDist, bForceHighPriority)) {}
 */
UCLASS()
class MUTABLEEXTENSION_API UMutableFunctionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static ESkeletalMeshStatus GetMutableComponentStatus(const UCustomizableSkeletalComponent* Component, bool& bValidResult);
	
public:
	static void ErrorOnFailedValidation(const UCustomizableSkeletalComponent* MutableMesh);

	/** @return True if we can update this mutable mesh */
	static bool IsMutableMeshValidToUpdate(const UCustomizableSkeletalComponent* MutableMesh);

	/** Must always call IsMutableMeshValidToUpdate() beforehand */
	static void UpdateMutableMesh(const UCustomizableSkeletalComponent* MutableMesh,
		bool bIgnoreCloseDist = false, bool bForceHighPriority = false);

	/** Must always call IsMutableMeshValidToUpdate() beforehand */
	static void UpdateMutableMesh_Callback(const UCustomizableSkeletalComponent* MutableMesh,
		const FInstanceUpdateDelegate& InstanceUpdateDelegate,
		bool bIgnoreCloseDist = false, bool bForceHighPriority = false);

	static FString ParseRuntimeUpdateError(const EMutableExtensionRuntimeUpdateError& Error, bool bVerbose);

protected:
	static FString ParseRuntimeUpdateError_Simple(const EMutableExtensionRuntimeUpdateError& Error);
	static FString ParseRuntimeUpdateError_Verbose(const EMutableExtensionRuntimeUpdateError& Error);
};
