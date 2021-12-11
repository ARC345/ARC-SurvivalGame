// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionWidget.h"
#include "../Components/InteractionComponent.h"


void UInteractionWidget::UpdateInteractionWidget(class UInteractionComponent* InteractionComponent)
{
	OwningInteractionComponent = InteractionComponent;
	OnUpdateInteractionWidget();
}
