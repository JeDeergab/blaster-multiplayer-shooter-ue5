// Fill out your copyright notice in the Description page of Project Settings.


#include "null_x.h"

// Sets default values
Anull_x::Anull_x()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void Anull_x::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void Anull_x::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void Anull_x::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

