// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SurvivalCharacter.generated.h"

class USkeletalMeshComponent;
class UInteractionComponent;
class UInventoryComponent;
class USpringArmComponent;
class ASurvivalCharacter;
class UCameraComponent;
class UEquippableItem;
class UThrowableItem;
class AWeapon;
class UAnimMontage;
class APickup;

USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()
	
	FInteractionData()
	{
		ViewedInteractionComponent = nullptr;
		LastInterationCheckTime = 0.0f;
		bInteractableHeld = false;
	}

	//the current intractable component were viewing, if there is one
	UPROPERTY()
	UInteractionComponent* ViewedInteractionComponent;

	// last time we checked for an intractable
	UPROPERTY()
	float LastInterationCheckTime;
	
	//Whether the local player is holding the interaction Key	
	UPROPERTY()
	bool bInteractableHeld;

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquippableItemsChanged, const E_EquippableSlot, Slot, const UEquippableItem*, Item);

UCLASS()
class SURVIVALGAME_API ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

	/*--------------FUNCS--------------*/
	public:
		ASurvivalCharacter();
		virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
		float GetRemainingInteractionTime() const;

		UFUNCTION(BlueprintCallable, Category = "items") void UseItem(class UItem* Item);
		UFUNCTION(Server, Reliable, WithValidation)	void ServerUseItem(class UItem* Item);
		
		UFUNCTION(BlueprintCallable, Category = "items") void DropItem(class UItem* Item, const int32 Quantity);
		UFUNCTION(Server, Reliable, WithValidation)	void ServerDropItem(class UItem* Item, const int32 Quantity);

		UFUNCTION(BlueprintCallable, Category = "items") float ModifyHealth(const float Delta);

		/*---------------------+Getters+---------------------*/
		//Helper function to make getting the interactables easier

		FORCEINLINE UInteractionComponent* GetInteracable() const { return InteractionData.ViewedInteractionComponent; }

		UFUNCTION(BlueprintCallable, Category = "Weapons")	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
		UFUNCTION(BlueprintPure, Category = "Weapons")	FORCEINLINE bool IsAiming()  const { return bIsAiming; }
		UFUNCTION(BlueprintPure) FORCEINLINE bool IsAlive() const { return Killer == nullptr; }

		UFUNCTION(BlueprintPure) FORCEINLINE TMap<E_EquippableSlot, UEquippableItem*> GetEquippedItems() const { return EquippedItems; };
		UFUNCTION(BlueprintPure) USkeletalMeshComponent* GetSlotSkeletalMeshComponent(const E_EquippableSlot Slot) const;
		UThrowableItem* GetThrowable() const;
		bool IsInteracting() const;

		/*---------------------~Getters~---------------------*/

		/*---------------------+Looting+---------------------*/
		UFUNCTION(BlueprintCallable, Category = "Looting")
			void SetLootSource(class UInventoryComponent* NewLootSource);
		UFUNCTION(BlueprintPure, Category = "Looting")
			bool IsLooting() const;
		UFUNCTION(BlueprintCallable, Category = "Looting")
			void LootItem(class UItem* ItemToGive);
		UFUNCTION(Server, Reliable, WithValidation)
			void ServerLootItem(class UItem* ItemToLoot);
		/*---------------------~Looting~---------------------*/

		/*---------------------+Interaction+---------------------*/
		void PerformInteractionCheck();
		void FoundNewInteractable(UInteractionComponent* Interactable);
		void CouldntFindInteractable();
		void BeginInteract();
		void EndInteract();
		UFUNCTION(Server, Reliable, WithValidation)	void ServerBeginInteract();
		UFUNCTION(Server, Reliable, WithValidation)	void ServerEndInteract();

		void Interact();
		/*---------------------~Interaction~---------------------*/

		/*---------------------+Items+---------------------*/

		bool EquipItem(UEquippableItem* Item);
		bool UnEquipItem(UEquippableItem* Item);

		void EquipGear(class UGearItem* Item);
		void UnEquipGear(const E_EquippableSlot Slot);

		void EquipWeapon(class UWeaponItem* Weapon);
		void UnEquipWeapon();
		void StartReloadWep();
		void StartFire();
		void StopFire();
		void BeginMeleeAttack();

		bool CanAim() const;
		void StartAiming();
		void StopAiming();
		void SetAiming(const bool bNewAiming);
		UFUNCTION(Server, Reliable) void ServerSetAiming(const  bool NewAiming);

		void UseThrowable();
		void SpawnThrowable();

		bool CanUseThrowable() const;
		UFUNCTION(Server, Reliable)	void ServerUseThrowable();
		UFUNCTION(NetMulticast, UnReliable) void MulticastPlayThrowableFX(class UAnimMontage* MontageToPlay);
		/*---------------------~Items~---------------------*/


		UFUNCTION(BlueprintImplementableEvent)
			void OnHealthModified(const float HealthDelta);

	protected:
		virtual void BeginPlay() override;
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		virtual void Tick(float DeltaTime) override;
		
		virtual void Restart() override;
		virtual void SetActorHiddenInGame(bool bNewHidden) override;
		virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

		UFUNCTION() void BeginLootingPlayer(ASurvivalCharacter* Character);
		UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
			void ServerSetLootSource(UInventoryComponent* NewLootSource);

		UFUNCTION()	void OnLootSourceOwnerDestroyed(AActor* DestroyedActor);

		void Suicide(struct FDamageEvent const& DamageEvent, const AActor* DamageCauser);
		void KilledByPlayer(struct FDamageEvent const& DamageEvent, class ASurvivalCharacter* Character, const AActor* DamageCauser);


		UFUNCTION()	void OnRep_Killer();
		UFUNCTION()	void OnRep_LootSource();
		UFUNCTION()	void OnRep_Health(float OldHealth);
		UFUNCTION() void OnRep_EquippedWeapon();

		UFUNCTION(Server, Reliable)	void ServerProcessMeleeHit(const FHitResult& MeleeHit);
		UFUNCTION(NetMulticast, UnReliable)	void MulticastPlayMeleeFX();
		UFUNCTION(BlueprintImplementableEvent) void OnDeath();

		bool CanSprint() const;
		void StartSprinting(); //local
		void StopSprinting(); //local

		UFUNCTION(Server, Reliable, WithValidation) void ServerSetSprinting(const bool bNewSprinting); // server
		void SetSprinting(const bool bNewSprinting); // Server + local

		void MoveForward(float Val);
		void MoveRight(float Val);

		void LookUp(float Val);
		void TurnRight(float Val);

		void StartCrouching();
		void StopCrouching();
	private:
	/*---------------------------------*/
	
	/*---------------VARS--------------*/
	public:
		UPROPERTY(BlueprintReadOnly, Category = Mesh)
			TMap<E_EquippableSlot, USkeletalMesh*> NakedMeshes;
		UPROPERTY(BlueprintReadOnly, Category = Mesh)
			TMap<E_EquippableSlot, USkeletalMeshComponent*> PlayerMeshes;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
			UInventoryComponent* PlayerInventoryComponent;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
			UInteractionComponent* LootPlayerInteractionComponent;
		
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
			USpringArmComponent* SpringArm;
		UPROPERTY(EditAnywhere, Category = "Components")
			UCameraComponent* CameraComponent;
		UPROPERTY(EditAnywhere, Category = "Components")
			USkeletalMeshComponent* HelmetMesh;
		UPROPERTY(EditAnywhere, Category = "Components")
			USkeletalMeshComponent* ChestMesh;
		UPROPERTY(EditAnywhere, Category = "Components")
			USkeletalMeshComponent* LegsMesh;
		UPROPERTY(EditAnywhere, Category = "Components")
			USkeletalMeshComponent* FeetMesh;
		UPROPERTY(EditAnywhere, Category = "Components")
			USkeletalMeshComponent* VestMesh;
		UPROPERTY(EditAnywhere, Category = "Components")
			USkeletalMeshComponent* HandsMesh;
		UPROPERTY(EditAnywhere, Category = "Components")
			USkeletalMeshComponent* BackpackMesh;

		FTimerHandle TimerHandle_Interact;
		//Information about the current state of the player
		UPROPERTY() FInteractionData InteractionData;

		TMap<E_EquippableSlot, UEquippableItem*> EquippedItems;

		UPROPERTY(BlueprintAssignable)
			FOnEquippableItemsChanged OnEquippableItemsChanged;

		UPROPERTY(EditDefaultsOnly, Category = "items")
			TSubclassOf<APickup> PickupClass;

	protected:
		UPROPERTY(ReplicatedUsing = OnRep_Killer)
			ASurvivalCharacter* Killer;
		UPROPERTY(ReplicatedUsing = OnRep_LootSource, BlueprintReadOnly)
			UInventoryComponent* LootSource;
		UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_EquippedWeapon)
			AWeapon* EquippedWeapon;

		UPROPERTY(ReplicatedUsing = OnRep_Health, EditDefaultsOnly, Category = "Health")
			float Health;
		UPROPERTY(EditDefaultsOnly, Category = "Health")
			float MaxHealth;

		//how often we check for interactables
		UPROPERTY(EditDefaultsOnly, Category = "Interaction")
			float InteractionCheckFrequency;
		UPROPERTY(EditDefaultsOnly, Category = "Interaction")
			float InteractionCheckDistance;

		UPROPERTY(Transient, Replicated)
			bool bIsAiming;

		UPROPERTY()	float LastMeleeAttackTime;
		UPROPERTY(EditDefaultsOnly, Category = "Melee") float MeleeAttackDistance;
		UPROPERTY(EditDefaultsOnly, Category = "Melee")	float MeleeAttackDamage;
		UPROPERTY(EditDefaultsOnly, Category = "Melee")	UAnimMontage* MeleeAttackMontage;

		UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")	bool bSprinting;
		UPROPERTY(EditDefaultsOnly, Category = "Movement") float SprintSpeed;
		UPROPERTY()	float WalkSpeed;
	private:
	/*---------------------------------*/
};
