// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon.h"
#include "SurvivalGame/SurvivalGame.h"

#include "Player/SurvivalPlayerController.h"
#include "Player/SurvivalCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/AudioComponent.h"

#include "Curves/CurveVector.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"

#include "Items/EquippableItem.h"
#include "Items/AmmoItem.h"

#include "DrawDebugHelpers.h"


AWeapon::AWeapon()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->bReceivesDecals = false;
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = WeaponMesh;

	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::Idle;
	AttachSocket1P = FName("GripPoint");
	AttachSocket3P = FName("GripPoint");

	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.f;

	ADSTime = 0.5f;
	RecoilSpeedReset = 5.f;
	RecoilSpeed = 10.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, PawnOwner);

	DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmoInClip, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeapon, bPendingReload, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeapon, Item, COND_InitialOnly); //Replicates only 1ce
}

void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		PawnOwner = Cast<ASurvivalCharacter>(GetOwner());
	}
}

void AWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

void AWeapon::UseClipAmmo()
{
	if (HasAuthority())
	{
		--CurrentAmmoInClip;
	}
}

void AWeapon::ConsumeAmmo(const int32 Amount)
{
	if (HasAuthority() && PawnOwner)
	{
		if (UInventoryComponent* Inventory = PawnOwner->PlayerInventoryComponent)
		{
			if (UItem* AmmoItem = Inventory->FindItemByClass(WeaponConfig.AmmoClass))
			{
				Inventory->ConsumeItem(AmmoItem, Amount);
			}
		}
	}
}

void AWeapon::ReturnAmmoToInventory()
{
	if (HasAuthority())
	{
		if (PawnOwner && CurrentAmmoInClip > 0)
		{
			if (UInventoryComponent* Inventory = PawnOwner->PlayerInventoryComponent)
			{
				Inventory->TryAddItemFromClass(WeaponConfig.AmmoClass, CurrentAmmoInClip);
			}
		}
	}
}

void AWeapon::OnEquip()
{
	AttachMeshToPawn();

	bPendingEquip = true;

	DetermineWeaponState();

	OnEquipFinished();

	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

void AWeapon::OnEquipFinished()
{
	AttachMeshToPawn();
	
	bIsEquipped = true;
	bPendingEquip = false;

	DetermineWeaponState();

	if (PawnOwner)
	{
		if (PawnOwner->IsLocallyControlled() && CanReload())
		{
			StartReloadWep();
		}
	}
}

void AWeapon::OnUnEquip()
{
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}
	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}
	ReturnAmmoToInventory();
	DetermineWeaponState();
}

bool AWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool AWeapon::IsAttachedToPawn() const
{
	return bPendingEquip || bIsEquipped;
}

void AWeapon::StartFire()
{
	if (GetLocalRole() > ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AWeapon::StopFire()
{
	if (GetLocalRole() > ROLE_Authority && PawnOwner && PawnOwner->IsLocallyControlled())
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

void AWeapon::StartReloadWep(bool bFromReplication /* = False */)
{
	if (!bFromReplication && GetLocalRole() < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.f)
		{
			AnimDuration = 0.5f;
		}
		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AWeapon::StopReload, AnimDuration, false);

		if (HasAuthority())
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration -0.1f), false);
		}

		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void AWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

void AWeapon::ReloadWeapon()
{
	const int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, GetCurrentAmmo());

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
		ConsumeAmmo(ClipDelta);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Didnt have enough ammo to reload"));
	}
}

bool AWeapon::CanFire() const
{
	const bool bCanStartFire = PawnOwner != nullptr;
	const bool bStateOkToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanStartFire == true) && (bStateOkToFire == true) && (bPendingReload == false));

}

bool AWeapon::CanReload() const
{
	const bool bCanReload = PawnOwner != nullptr;
	const bool bGotAmmo = ((CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (GetCurrentAmmo() > 0));
	const bool bStateOkToReload = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOkToReload == true));
}

EWeaponState AWeapon::GetCurrentState() const
{
	return CurrentState;
}

int32 AWeapon::GetCurrentAmmo() const
{
	if (PawnOwner)
	{
		if (UInventoryComponent* Inventory = PawnOwner->PlayerInventoryComponent)
		{
			if (UItem* Ammo = Inventory->FindItemByClass(WeaponConfig.AmmoClass))
			{
				return Ammo->GetQuantity();
			}
		}
	}
	return 0;
}

int32 AWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 AWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

USkeletalMeshComponent* AWeapon::GetWeaponMesh() const
{
	return WeaponMesh;
}

ASurvivalCharacter* AWeapon::GetPawnOwner() const
{
	return PawnOwner;
}

void AWeapon::SetPawnOwner(ASurvivalCharacter* SChar)
{
	if (PawnOwner != SChar)
	{
		SetInstigator(SChar);
		PawnOwner = SChar;

		SetOwner(SChar);
	}
}

float AWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float AWeapon::GetEquipDuration() const
{
	return EquipDuration;
}

void AWeapon::ClientStartReload_Implementation()
{
	StartReloadWep();
}

void AWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AWeapon::ServerStartFire_Validate()
{
	return true;
}

void AWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool AWeapon::ServerStopFire_Validate()
{
	return true;
}

void AWeapon::ServerStartReload_Implementation()
{
	StartReloadWep();
}

bool AWeapon::ServerStartReload_Validate()
{
	return true;

}

void AWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

bool AWeapon::ServerStopReload_Validate()
{
	return true;

}

void AWeapon::OnRep_PawnOwner()
{
}

void AWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReloadWep();
	}
	else
	{
		StopReload();
	}
}

void AWeapon::SimulateWeaponFire()
{
	if (HasAuthority() && CurrentState != EWeaponState::Firing)
	{
		return;
	}
	if (MuzzleFX)
	{
		if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			if ((PawnOwner != nullptr) && (PawnOwner->IsLocallyControlled() == true))
			{
				if (AController* PC = PawnOwner->GetController())
				{
					WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = true;
				}
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
			}
		}
	}
	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		FWeaponAnim AnimToPlay = FireAnim;//PawnOwner->IsAiming() || PawnOwner->IsLocallyControlled() ? FireAimingAnim : FireAnim;
		PlayWeaponAnimation(AnimToPlay);
		bPlayingFireAnim = true;
	}
	if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	if (ASurvivalPlayerController* PC = (PawnOwner != NULL) ? Cast<ASurvivalPlayerController>(PawnOwner->GetController()) : NULL)
	{
		if (PC && PC->IsLocalController())
		{
			if (FireCameraShake != NULL)
			{
				PC->ClientPlayCameraShake(FireCameraShake, 1);
			}
			if (FireForceFeedback != NULL)
			{
				FForceFeedbackParameters FFFParams;
				FFFParams.Tag = "Weapon";
				PC->ClientPlayForceFeedback(FireForceFeedback, FFFParams);
			}
		}
	}
}

void AWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX)
	{
		if (MuzzlePSC != NULL)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if (MuzzlePSCSecondary != NULL)
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAimingAnim);
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}
	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.f);
		FireAC = NULL;
		PlayWeaponSound(FireFinishSound);
	}
}

void AWeapon::HandleHit(const FHitResult& Hit, ASurvivalCharacter* SHitPlayer)
{
	if (Hit.GetActor())
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit actor %s"), *Hit.GetActor()->GetName());
	}
	ServerHandleHit(Hit, SHitPlayer);

	if (SHitPlayer && PawnOwner)
	{
		if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(PawnOwner->GetController()))
		{
			PC->OnHitPlayer();
		}
	}
}

void AWeapon::ServerHandleHit_Implementation(const FHitResult& Hit, ASurvivalCharacter* SHitPlayer)
{
	if (PawnOwner)
	{
		float DamageMultiplier = 1.0f;

		for (auto& BoneDamageModifier : HitScanConfig.BoneDamageModifier)
		{
			if (Hit.BoneName == BoneDamageModifier.Key)
			{
				DamageMultiplier = BoneDamageModifier.Value;
				break;
			}
		}

		if (SHitPlayer)
		{
			UGameplayStatics::ApplyPointDamage(SHitPlayer, HitScanConfig.Damage * DamageMultiplier, (Hit.TraceStart - Hit.TraceEnd).GetSafeNormal(), Hit, PawnOwner->GetController(), this, HitScanConfig.DamageType);
		}
	}
}

bool AWeapon::ServerHandleHit_Validate(const FHitResult& Hit, ASurvivalCharacter* SHitPlayer)
{
	return true;
}

void AWeapon::FireShot()
{
	if (PawnOwner)
	{
		if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(PawnOwner->GetController()))
		{
			if (RecoilCurve)
			{
				const FVector2D RecoilAmount(RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).X, RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).Y);
				PC->ApplyRecoil(RecoilAmount, RecoilSpeed, RecoilSpeedReset, FireCameraShake);
			}

			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);

			FHitResult Hit;
			FCollisionQueryParams QParams;
			QParams.AddIgnoredActor(this);
			QParams.AddIgnoredActor(PawnOwner);

			FVector FireDir = CamRot.Vector();//PawnOwner->IsAiming() ? CamRot.Vector() : FMath::VRandCone(CamRot.Vector(), FMath::DegreesToRadians(PawnOwner->IsAiming() ? 0.f : 5.f));

			const FVector TraceStart = CamLoc;
			const FVector TraceEnd = (FireDir * HitScanConfig.Distance) + CamLoc;

			if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, COLLISION_WEAPON, QParams))
			{
				ASurvivalCharacter* HitChar = Cast<ASurvivalCharacter>(Hit.GetActor());

				HandleHit(Hit, HitChar);

				//const FColor PointColor = FColor::White;

				//DrawDebugLine(GetWorld(), TraceStart, TraceEnd, PointColor, false, 0.1f);
			}
		}
	}
}

void AWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		UseClipAmmo();

		BurstCounter++;
		OnRep_BurstCounter();
	}
}

bool AWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void AWeapon::HandleReFiring()
{
	const UWorld* MyWorld = GetWorld();

	float StackTimeThisFrame = FMath::Max(0.0f, (MyWorld->TimeSeconds - LastFireTime) - WeaponConfig.TimeBetweenShots);

	if (bAllowAutomaticWeaponCatchup)
	{
		TimerIntervalAdjustment -= StackTimeThisFrame;
	}

	HandleFiring();
}

void AWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			FireShot();
			UseClipAmmo();

			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReloadWep();
	}

	else if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(PawnOwner->Controller);
		}
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerHandleFiring();
		}
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReloadWep();
		}

		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleReFiring, FMath::Max<float>(WeaponConfig.TimeBetweenShots + TimerIntervalAdjustment, SMALL_NUMBER), false);
			TimerIntervalAdjustment = 0.f;
		}
	}
	LastFireTime = GetWorld()->GetTimeSeconds();
}

void AWeapon::OnBurstStarted()
{
	const float GameTime = GetWorld()->GetTimeSeconds();

	if (LastFireTime > 0.f && WeaponConfig.TimeBetweenShots > 0.f && LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void AWeapon::OnBurstFinished()
{
	BurstCounter = 0;

	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;

	TimerIntervalAdjustment = 0.0f;
}

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	const EWeaponState PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void AWeapon::DetermineWeaponState()
{
	EWeaponState NewState = EWeaponState::Idle;
	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((bPendingReload == false) && bWantsToFire && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}
	SetWeaponState(NewState);
}

void AWeapon::AttachMeshToPawn()
{
	if (PawnOwner)
	{
		if (const USkeletalMeshComponent* PawnMesh = PawnOwner->GetMesh())
		{
			const FName AttachSocket = PawnOwner->IsLocallyControlled() ? AttachSocket1P : AttachSocket3P;
			AttachToComponent(PawnOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
		}
	}
}

UAudioComponent* AWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && PawnOwner)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, GetOwner()->GetRootComponent());
	}
	return AC;
}

float AWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (PawnOwner)
	{
		UAnimMontage* AnimMont = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (AnimMont)
		{
			Duration = PawnOwner->PlayAnimMontage(AnimMont);
		}
	}
	return Duration;
}

void AWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (PawnOwner)
	{
		UAnimMontage* AnimMont = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (AnimMont)
		{
			PawnOwner->StopAnimMontage(AnimMont);
		}
	}
}

FVector AWeapon::GetCameraAim() const
{
	ASurvivalPlayerController* const PlayerController = GetInstigator() ? Cast<ASurvivalPlayerController>(GetInstigator()->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (GetInstigator())
	{
		FinalAim = GetInstigator()->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}

FHitResult AWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	FCollisionQueryParams QParams(SCENE_QUERY_STAT(WeaponTrace), true, GetInstigator());

	QParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, COLLISION_WEAPON, QParams);

	return Hit;
}
