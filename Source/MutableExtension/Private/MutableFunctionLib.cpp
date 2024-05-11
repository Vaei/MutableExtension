// Copyright (c) Jared Taylor. All Rights Reserved


#include "MutableFunctionLib.h"

#include "MutableExtensionComponent.h"
#include "MutableExtensionLog.h"
#include "MutableExtensionTypes.h"
#include "GameFramework/PlayerState.h"
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

USkeletalMeshComponent* UMutableFunctionLib::GetSkeletalMeshCompFromMutableComp(
	const UCustomizableSkeletalComponent* Component)
{
	// We assume that mutable components are always attached to skeletal mesh components
	USceneComponent* AttachParent = Component->GetAttachParent();
	if (!ensureAlways(!AttachParent || AttachParent->IsA<USkeletalMeshComponent>()))
	{
		return nullptr;
	}
	USkeletalMeshComponent* OwningComponent = AttachParent ? CastChecked<USkeletalMeshComponent>(AttachParent) : nullptr;
	return OwningComponent;
}

AActor* UMutableFunctionLib::GetTargetedActor(const APlayerController* PlayerController, ECollisionChannel TraceChannel, bool bAllowUnderCursor, bool
	bDebugTargetActorTrace)
{
	if (!PlayerController)
	{
		return nullptr;
	}
	
	// Check for actor under cursor first
	FHitResult HitResult;

	if (bAllowUnderCursor)
	{
		PlayerController->GetHitResultUnderCursor(TraceChannel, false, HitResult);
	
#if ENABLE_DRAW_DEBUG
		if (bDebugTargetActorTrace)
		{
			DrawDebugSphere(
				PlayerController->GetWorld(),
				HitResult.ImpactPoint,
				10.f,
				5,
				HitResult.bBlockingHit ? FColor::Red : FColor::Green,
				false,
				5.f
			);
		}
#endif
	
		if (AActor* HitActor = HitResult.GetActor())
		{
			return HitActor;
		}
	}

	// If no actor under cursor, check in the direction the camera is facing
	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);  // Get the camera location and rotation
	const FVector Start = CameraLocation + CameraRotation.Vector() * 30.f;
	FVector End = Start + CameraRotation.Vector() * 10000;  // Adjust the length as needed

	// Perform a line trace
	FCollisionQueryParams QueryParams { TEXT("TraceForTargetActor"), false, PlayerController->GetPawn() };
	PlayerController->GetWorld()->SweepSingleByChannel(HitResult, CameraLocation, End, FQuat::Identity,
		TraceChannel, FCollisionShape::MakeSphere(32.f), QueryParams);

#if ENABLE_DRAW_DEBUG
	if (bDebugTargetActorTrace)
	{
		DrawDebugLine(
			PlayerController->GetWorld(),
			CameraLocation,
			HitResult.bBlockingHit ? HitResult.ImpactPoint : End,
			HitResult.bBlockingHit ? FColor::Yellow : FColor::Blue,
			false,
			5.f,
			0,
			2.f
		);
		DrawDebugPoint(
			PlayerController->GetWorld(),
			HitResult.ImpactPoint,
			10.f,
			HitResult.bBlockingHit ? FColor::Red : FColor::Green,
			false,
			5.f
		);
	}
#endif

	if (HitResult.GetActor())
	{
		return HitResult.GetActor();
	}

	// If nothing is found return nullptr
	return nullptr;
}

void UMutableFunctionLib::DumpMutableData(const AActor* ForActor, bool bDumpToMessageLog)
{
	const FString Dump = GatherMutableDataDump(ForActor);
	if (bDumpToMessageLog)
	{
		FMessageLog MessageLog { "PIE" };
		MessageLog.Info(FText::FromString(Dump));
	}
	else
	{
		UE_LOG(LogMutableExtension, Log, TEXT("%s"), *Dump);
	}
}

void UMutableFunctionLib::DumpMutableDataForTargetedActor(const APlayerController* PlayerController,
	ECollisionChannel TraceChannel, bool bDumpToMessageLog, bool bAllowUnderCursor, bool bDebugTargetActorTrace)
{
	DumpMutableData(GetTargetedActor(PlayerController, TraceChannel, bAllowUnderCursor, bDebugTargetActorTrace), bDumpToMessageLog);
}

FString UMutableFunctionLib::GatherMutableDataDump(const AActor* ForActor)
{
	const FString HeaderFooter = "=================================================";
	const FString Breaker = "-------------------------------------------";
	FString Dump = "\n" + HeaderFooter + "\n";

	if (IsValid(ForActor))
	{
		const APawn* MaybePawn = Cast<APawn>(ForActor);
		const AController* MaybeC = MaybePawn ? MaybePawn->GetController() : nullptr;
		const APlayerController* MaybePC = MaybeC ? Cast<APlayerController>(MaybeC) : nullptr;
		const APlayerState* MaybePS = MaybePawn ? MaybePawn->GetPlayerState() : nullptr;
		const bool bIsLocallyControlled = MaybePC ? MaybePC->IsLocalController() : false;
		const bool bIsBot = MaybePS ? MaybePS->IsABot() : false;

		// Actor data
		Dump += FString::Printf(TEXT("ACTOR: { %s }\n"), *ForActor->GetName());
		Dump += FString::Printf(TEXT("OWNER: { %s }\n"), *GetNameSafe(ForActor->GetOwner()));
		Dump += FString::Printf(TEXT("OUTER: { %s }\n"), *GetNameSafe(ForActor->GetOuter()));

		// Net data
		Dump += FString::Printf(TEXT("LOCAL ROLE: { %s }\n"), *GetNetRoleString(ForActor->GetLocalRole()));
		Dump += FString::Printf(TEXT("REMOTE ROLE: { %s }\n"), *GetNetRoleString(ForActor->GetRemoteRole()));
		Dump += FString::Printf(TEXT("HAS AUTHORITY: { %s }\n"), *LexToString(ForActor->HasAuthority()));

		// Render data
		Dump += FString::Printf(TEXT("HIDDEN IN GAME: { %s }\n"), *LexToString(ForActor->IsHidden()));

		// Controller data
		Dump += FString::Printf(TEXT("PLAYER STATE: { %s }\n"), *GetNameSafe(MaybePS));
		Dump += FString::Printf(TEXT("CONTROLLER: { %s }\n"), *GetNameSafe(MaybeC));
		Dump += FString::Printf(TEXT("PLAYER CONTROLLER: { %s }\n"), *GetNameSafe(MaybePC));
		Dump += FString::Printf(TEXT("IS LOCALLY CONTROLLED: { %s }\n"), *LexToString(bIsLocallyControlled));
		Dump += FString::Printf(TEXT("IS A BOT: { %s }\n"), *LexToString(bIsBot));

		// Extension comp data
		Dump += Breaker + "\n";
		UMutableExtensionComponent* ExtensionComp = ForActor->GetComponentByClass<UMutableExtensionComponent>();
		Dump += FString::Printf(TEXT("EXTENSION COMP: { %s }\n"), *GetNameSafe(ExtensionComp));
		if (ExtensionComp)
		{
			Dump += FString::Printf(TEXT("Has Completed Initialization: { %s }\n"), *LexToString(ExtensionComp->bHasCompletedInitialization));
			Dump += FString::Printf(TEXT("Has Completed Initial Update: { %s }\n"), *LexToString(ExtensionComp->bHasCompletedInitialUpdate));
			Dump += FString::Printf(TEXT("Instances Pending Initialization: { %d }\n"), ExtensionComp->InstancesPendingInitialization.Num());
			Dump += FString::Printf(TEXT("Instances Pending Initial Update: { %d }\n"), ExtensionComp->InstancesPendingInitialUpdate.Num());
			Dump += FString::Printf(TEXT("Instances Pending Runtime Update: { %d }\n"), ExtensionComp->InstancesPendingRuntimeUpdate.Num());
		}

		// Mutable comp data
		TArray<UCustomizableSkeletalComponent*> MutableComponents;
		ForActor->GetComponents<UCustomizableSkeletalComponent>(MutableComponents);
		if (MutableComponents.Num() > 0)
		{
			Dump += FString::Printf(TEXT("MUTABLE SKELETAL COMPONENTS: { %d }\n"), MutableComponents.Num());
			for (UCustomizableSkeletalComponent* MutableComp : MutableComponents)
			{
				Dump += Breaker + "\n";
				Dump += FString::Printf(TEXT("[------------%s------------]\n"), *MutableComp->GetName());

				UCustomizableObjectInstance* Instance = MutableComp->CustomizableObjectInstance;
				Dump += FString::Printf(TEXT("Instance: { %s }\n"), *GetNameSafe(Instance));

				bool bValidResult;
				const ESkeletalMeshStatus Status = GetMutableComponentStatus(MutableComp, bValidResult);
				Dump += FString::Printf(TEXT("Mesh Status: { %s }\n"), *(bValidResult ? GetSkeletalMeshStatusString(Status) : "Unknown"));

				// Skeletal mesh comp data
				USkeletalMeshComponent* MeshComponent = GetSkeletalMeshCompFromMutableComp(MutableComp);
				Dump += FString::Printf(TEXT("Skeletal Mesh Comp: { %s }\n"), *GetNameSafe(MeshComponent));
				if (MeshComponent)
				{
					USkeletalMesh* SkeletalMesh = MeshComponent->GetSkeletalMeshAsset();
					Dump += FString::Printf(TEXT("Skeletal Mesh: { %s }\n"), *GetNameSafe(SkeletalMesh));
					Dump += FString::Printf(TEXT("Anim Instance: { %s }\n"), *GetNameSafe(MeshComponent->GetAnimInstance()));
					Dump += FString::Printf(TEXT("Post-Process Instance: { %s }\n"), *GetNameSafe(MeshComponent->GetPostProcessInstance()));
					Dump += FString::Printf(TEXT("Hidden In Game: { %s }\n"), *LexToString(MeshComponent->bHiddenInGame));
					Dump += FString::Printf(TEXT("Visible: { %s }\n"), *LexToString(MeshComponent->IsVisible()));
					Dump += FString::Printf(TEXT("Num Materials: { %d }\n"), MeshComponent->GetNumMaterials());
				}
			}
		}
		else
		{
			Dump += "-- Actor has no UCustomizableSkeletalComponent --\n";
		}
	}
	else
	{
		Dump += "-- Actor is not valid --\n";
	}
	
	// Iterates all mutable components
	// Parent skeletal mesh
	// Parent skeletal mesh rendering data
	// Success state
	// Instance

	Dump += HeaderFooter;
	return Dump;
}

FString UMutableFunctionLib::ParseRuntimeUpdateError(const EMutableExtensionRuntimeUpdateError& Error, bool bVerbose)
{
	return bVerbose ? ParseRuntimeUpdateError_Verbose(Error) : ParseRuntimeUpdateError_Simple(Error);
}

FString UMutableFunctionLib::GetNetRoleString(const ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None: return "None";
	case ROLE_SimulatedProxy: return "Simulated Proxy";
	case ROLE_AutonomousProxy: return "Autonomous Proxy";
	case ROLE_Authority: return "Authority";
	default: return "Unknown";
	}
}

FString UMutableFunctionLib::GetSkeletalMeshStatusString(ESkeletalMeshStatus Status)
{
	switch (Status)
	{
	case ESkeletalMeshStatus::NotGenerated: return "Not Generated";
	case ESkeletalMeshStatus::Success: return "Success";
	case ESkeletalMeshStatus::Error: return "Error";
	default: return "Unknown";
	}
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
