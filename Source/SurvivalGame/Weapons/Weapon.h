// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle,
	Firing,
	Reloading,
	Equipping
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	int32 AmmoPerClip;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	TSubclassOf<class UAmmoItem> AmmoClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	float TimeBetweenShots;
	
	FWeaponData()
	{
		AmmoPerClip = 20;
		TimeBetweenShots = 0.169f;
	}
};

USTRUCT(BlueprintType)
struct FWeaponAnim
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* Pawn1P;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* Pawn3P;
};

USTRUCT(BlueprintType)
struct FHitScanConfig
{
	GENERATED_USTRUCT_BODY()
	
	FHitScanConfig()
	{
		Distance = 10000.f;
		Damage = 25.f;
		Radius = 0.f;
		DamageType = /*class*/ UDamageType::StaticClass();
	}

	UPROPERTY(EditDefaultsOnly, Category = "TraceInfo")
	TMap<FName, float> BoneDamageModifier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceInfo")
	float Distance;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceInfo")
	float Damage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceInfo")
	float Radius;
	
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<class UDamageType> DamageType;

};
UCLASS()
class SURVIVALGAME_API AWeapon : public AActor
{
	GENERATED_BODY()

	friend class ASurvivalCharacter;

public:	
	// Sets default values for this actor's properties
	AWeapon();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;
	virtual void Destroyed() override;

protected:

	void UseClipAmmo();
	void ConsumeAmmo(const int32 Amount);
	void ReturnAmmoToInventory(); // server

	virtual void OnEquip();
	virtual void OnEquipFinished();
	virtual void OnUnEquip();
	bool IsEquipped() const;
	bool IsAttachedToPawn() const;

	///////////////////////////////////////
	//INPUT//

	virtual void StartFire(); // local, server
	virtual void StopFire(); // local, server

	virtual void StartReloadWep(bool bFromReplication = false); // all
	virtual void StopReload(); // local, server

	virtual void ReloadWeapon(); // server  Actual reload
	
	UFUNCTION(reliable, client)
	void ClientStartReload(); // client reload

	bool CanFire() const;
	bool CanReload() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EWeaponState GetCurrentState() const;
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const;
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmoInClip() const;

	int32 GetAmmoPerClip() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const;
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	class ASurvivalCharacter* GetPawnOwner() const;
	
	void SetPawnOwner(class ASurvivalCharacter* SChar);

	float GetEquipStartedTime() const;
	float GetEquipDuration() const;

protected:

	UPROPERTY(Replicated, BlueprintReadOnly, Transient)
	class UWeaponItem* Item;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_PawnOwner)
	class ASurvivalCharacter* PawnOwner;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FWeaponData WeaponConfig;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FHitScanConfig HitScanConfig;
	

public:

	UPROPERTY(EditAnywhere, Category = Components)
	USkeletalMeshComponent* WeaponMesh;

protected:
	UPROPERTY(Transient)
	float TimerIntervalAdjustment;

	UPROPERTY(Config)
	bool bAllowAutomaticWeaponCatchup = true;

	UPROPERTY(Transient)
	class UAudioComponent* FireAC;
	
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName MuzzleAttachPoint;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName AttachSocket1P;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName AttachSocket3P;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	class UParticleSystem* MuzzleFX;
	
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(Transient)
	class UParticleSystemComponent* MuzzlePSC;

	UPROPERTY(Transient)
	class UParticleSystemComponent* MuzzlePSCSecondary;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<class UMatineeCameraShake> FireCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float ADSTime;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	class UCurveVector* RecoilCurve;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilSpeed;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilSpeedReset;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	class UForceFeedbackEffect* FireForceFeedback;
	
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* FireSound;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* FireLoopSound;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* FireFinishSound;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* OutOfAmmoSound;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* ReloadSound;
	
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim ReloadAnim;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* EquipSound;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim EquipAnim;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim FireAnim;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim FireAimingAnim;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	uint32 bLoopedMuzzleFX : 1;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	uint32 bLoopedFireSound : 1;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	uint32 bLoopedFireAnim : 1;

	uint32 bPlayingFireAnim : 1;

	uint32 bIsEquipped : 1;

	uint32 bWantsToFire : 1;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
	uint32 bPendingReload : 1;

	uint32 bPendingEquip : 1;

	uint32 bRefiring;

	float LastFireTime;

	float EquipStartedTime;

	float EquipDuration;

	EWeaponState CurrentState;
	
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmoInClip;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter;
	
	FTimerHandle TimerHandle_OnEquipFinished;

	FTimerHandle TimerHandle_StopReload;

	FTimerHandle TimerHandle_ReloadWeapon;

	FTimerHandle TimerHandle_HandleFiring;
	//////////////////////////////////////////////////////////////////////////
// Input - server side

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartFire();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStopFire();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartReload();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStopReload();

	//////////////////////////////////////////////////////////////////////////
// Replication & effects
	UFUNCTION()
	void OnRep_PawnOwner();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	virtual void SimulateWeaponFire();

	virtual void StopSimulatingWeaponFire();
	//////////////////////////////////////////////////////////////////////////
// Weapon usage

	void HandleHit(const FHitResult& Hit, class ASurvivalCharacter* SHitPlayer = nullptr);
	
	UFUNCTION(Server, reliable, WithValidation)
	void ServerHandleHit(const FHitResult& Hit, class ASurvivalCharacter* SHitPlayer = nullptr);

	virtual void FireShot();// Local

	UFUNCTION(reliable, server, WithValidation)
	void ServerHandleFiring();// Server

	void HandleReFiring();// Local + Server

	void HandleFiring();// Local + Server

	virtual void OnBurstStarted();// Local + Server

	virtual void OnBurstFinished();// Local + Server

	void SetWeaponState(EWeaponState NewState);

	void DetermineWeaponState();

	void AttachMeshToPawn();

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	class UAudioComponent* PlayWeaponSound(class USoundCue* Sound);

	float PlayWeaponAnimation(const FWeaponAnim& Animation);

	void StopWeaponAnimation(const FWeaponAnim& Animation);

	FVector GetCameraAim() const;

	FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const;
};
