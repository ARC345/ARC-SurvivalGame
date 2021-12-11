// Fill out your copyright notice in the Description page of Project Settings.

#include "SurvivalPlayerController.h"
#include "Player/SurvivalCharacter.h"

ASurvivalPlayerController::ASurvivalPlayerController()
{

}

void ASurvivalPlayerController::ClientShowNotification_Implementation(const FText& Message)
{
	ShowNotification(Message);
}

void ASurvivalPlayerController::ApplyRecoil(const FVector2D& RecoilAmount, const float RecoilSpeed, const float RecoilResetSpeed, TSubclassOf<class UMatineeCameraShake> Shake)
{
	if (IsLocalPlayerController())
	{
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraShake(Shake);
		}

		RecoilBumpAmount += RecoilAmount;
		RecoilResetAmount += -RecoilAmount;

		CurrentRecoilSpeed = RecoilSpeed;
		CurrentRecoilResetSpeed = RecoilResetSpeed;

		LastRecoilTime = GetWorld()->GetTimeSeconds();
	}
}

void ASurvivalPlayerController::TurnRight(float Rate)
{
	if (!FMath::IsNearlyZero(RecoilResetAmount.X, 0.01f))
	{
		if (RecoilResetAmount.X > 0.f && Rate > 0.f)
		{
			RecoilResetAmount.X = FMath::Max(0.f, RecoilResetAmount.X - Rate);
		}
		else if (RecoilResetAmount.X < 0.f && Rate < 0.f)
		{
			RecoilResetAmount.X = FMath::Min(0.f, RecoilResetAmount.X - Rate);
		}
	}
	if (!FMath::IsNearlyZero(RecoilBumpAmount.X, 0.1f))
	{
		FVector2D LastCurrentRecoilAMT = RecoilBumpAmount;
		RecoilBumpAmount.X = FMath::FInterpTo(RecoilBumpAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		AddYawInput(LastCurrentRecoilAMT.X - RecoilBumpAmount.X);
	}
	FVector2D LastCurrentResetRecoilAMT = RecoilResetAmount;
	RecoilResetAmount.X = FMath::FInterpTo(RecoilResetAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);

	AddYawInput(LastCurrentResetRecoilAMT.X - RecoilResetAmount.X);

	AddYawInput(Rate);
}

void ASurvivalPlayerController::LookUp(float Rate)
{
	if (!FMath::IsNearlyZero(RecoilResetAmount.Y, 0.01f))
	{
		if (RecoilResetAmount.Y > 0.f && Rate > 0.f)
		{
			RecoilResetAmount.Y = FMath::Max(0.f, RecoilResetAmount.Y - Rate);
		}
		else if (RecoilResetAmount.Y < 0.f && Rate < 0.f)
		{
			RecoilResetAmount.Y = FMath::Min(0.f, RecoilResetAmount.Y - Rate);
		}
	}
	if (!FMath::IsNearlyZero(RecoilBumpAmount.Y, 0.01f))
	{
		FVector2D LastCurrentRecoilAMT = RecoilBumpAmount;
		RecoilBumpAmount.Y = FMath::FInterpTo(RecoilBumpAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		AddPitchInput(LastCurrentRecoilAMT.Y - RecoilBumpAmount.Y);
	}
	FVector2D LastCurrentResetRecoilAMT = RecoilResetAmount;
	RecoilResetAmount.Y = FMath::FInterpTo(RecoilResetAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);

	AddPitchInput(LastCurrentResetRecoilAMT.Y - RecoilResetAmount.Y);

	AddPitchInput(Rate);
}

void ASurvivalPlayerController::StartReload()
{
	if (ASurvivalCharacter* SC = Cast<ASurvivalCharacter>(GetPawn()))
	{
		if (SC->IsAlive())
		{
			SC->StartReloadWep();
		}
		else
		{
			Respawn();
		}
	}
}

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis("LookUp", this, &ASurvivalPlayerController::LookUp);
	InputComponent->BindAxis("TurnRight", this, &ASurvivalPlayerController::TurnRight);

	InputComponent->BindAction("Reload", IE_Pressed , this, &ASurvivalPlayerController::StartReload);
}

void ASurvivalPlayerController::Respawn()
{
	UnPossess();
	ChangeState(NAME_Inactive);

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerRespawn();
	}
	else
	{
		ServerRestartPlayer();
	}
}

void ASurvivalPlayerController::ServerRespawn_Implementation()
{
	Respawn();
}

bool ASurvivalPlayerController::ServerRespawn_Validate()
{
	return true;
}
