// Copyright (c) Jared Taylor. All Rights Reserved


#include "MutableExtensionTypes.h"

#include "MuCO/CustomizableObjectInstanceUsage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MutableExtensionTypes)

FMutablePendingRuntimeUpdate::FMutablePendingRuntimeUpdate(UCustomizableObjectInstance* InMutableInstance,
	UCustomizableSkeletalComponent* InMutableComponent, USkeletalMeshComponent* InOwningComponent)
	: UpdateResult(EUpdateResult::Error)
	, MutableInstance(InMutableInstance)
	, MutableComponent(InMutableComponent)
	, OwningComponent(InOwningComponent)
{}
