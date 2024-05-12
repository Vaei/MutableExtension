// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "MutableExtensionTypes.generated.h"

enum class EUpdateResult : uint8;
class UCustomizableSkeletalComponent;
class UCustomizableObjectInstance;

UENUM(BlueprintType)
enum class EMutableExtensionRuntimeUpdateError : uint8
{
	None,
	DelegateNotBound,
	AlreadyPendingUpdate,
	MeshNotValidToUpdate,
};

USTRUCT(BlueprintType)
struct MUTABLEEXTENSION_API FMutablePendingRuntimeUpdate
{
	GENERATED_BODY()

	FMutablePendingRuntimeUpdate(
	UCustomizableObjectInstance* InMutableInstance = nullptr,
	UCustomizableSkeletalComponent* InMutableComponent = nullptr,
	USkeletalMeshComponent* InOwningComponent = nullptr);

	UPROPERTY(BlueprintReadOnly, Category="Mutable")
	EUpdateResult UpdateResult;
	
	UPROPERTY(BlueprintReadOnly, Category="Mutable")
	UCustomizableObjectInstance* MutableInstance;

	UPROPERTY(BlueprintReadOnly, Category="Mutable")
	UCustomizableSkeletalComponent* MutableComponent;

	UPROPERTY(BlueprintReadOnly, Category="Mutable")
	USkeletalMeshComponent* OwningComponent;
};
