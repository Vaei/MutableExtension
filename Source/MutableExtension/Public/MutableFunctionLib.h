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

public:
	static USkeletalMeshComponent* GetSkeletalMeshCompFromMutableComp(const UCustomizableSkeletalComponent* Component);
	
public:
	static AActor* GetTargetedActor(const APlayerController* PlayerController, ECollisionChannel TraceChannel = ECC_Visibility, bool bAllowUnderCursor = false, bool bDebugTargetActorTrace = false);

	UFUNCTION(BlueprintCallable, Category="Mutable")
	static void DumpMutableData(const AActor* ForActor, bool bDumpToMessageLog = false);

	/** Determine which actor we are looking at and dump their data */
	UFUNCTION(BlueprintCallable, Category="Mutable")
	static void DumpMutableDataForTargetedActor(const APlayerController* PlayerController, ECollisionChannel TraceChannel = ECC_Visibility, bool bDumpToMessageLog = false, bool bAllowUnderCursor = false, bool bDebugTargetActorTrace = true);

	static FString GatherMutableDataDump(const AActor* ForActor);

	static FString ParseRuntimeUpdateError(const EMutableExtensionRuntimeUpdateError& Error, bool bVerbose);

	static FString GetNetRoleString(ENetRole Role);

	static FString GetSkeletalMeshStatusString(ESkeletalMeshStatus Status);

protected:
	static FString ParseRuntimeUpdateError_Simple(const EMutableExtensionRuntimeUpdateError& Error);
	static FString ParseRuntimeUpdateError_Verbose(const EMutableExtensionRuntimeUpdateError& Error);
};
