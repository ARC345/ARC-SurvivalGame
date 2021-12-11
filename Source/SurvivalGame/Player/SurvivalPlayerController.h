// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ASurvivalPlayerController();
	
	virtual void SetupInputComponent() override;

	UFUNCTION(Client, Reliable, BlueprintCallable)
	void ClientShowNotification(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowNotification(const FText& Message);
	UFUNCTION(BlueprintImplementableEvent)
	void ShowDeathScreen(class ASurvivalCharacter* Killer);
	UFUNCTION(BlueprintImplementableEvent)
	void ShowLootScreen(const class UInventoryComponent* LootSource);
	UFUNCTION(BlueprintImplementableEvent)
	void ShowInGameUI();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HideLootMenu();
	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

	UFUNCTION(BlueprintCallable, Category = "PlayerController")
	void Respawn();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRespawn();

public:
	
	/**Applies recoil to the camera.
	@param RecoilAmount the amount to recoil by. X is the yaw, Y is the pitch
	@param RecoilSpeed the speed to bump the camera up per second from the recoil
	@param RecoilResetSpeed the speed the camera will return to center at per second after the recoil is finished
	@param Shake an optional camera shake to play with the recoil*/

	void ApplyRecoil(const FVector2D& RecoilAmount, const float RecoilSpeed, const float RecoilResetSpeed, TSubclassOf<class UMatineeCameraShake> Shake = nullptr);

	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilBumpAmount;
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilResetAmount;
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilSpeed;
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilResetSpeed;
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	float LastRecoilTime;

	void TurnRight(float Rate);
	void LookUp(float Rate);

	void StartReload();
};
