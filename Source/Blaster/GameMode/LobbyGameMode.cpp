// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

DECLARE_LOG_CATEGORY_EXTERN(MyLog, Warning, All);
DEFINE_LOG_CATEGORY(MyLog);

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	if(NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();

		if(World)
		{
			bUseSeamlessTravel = true;
			World->ServerTravel(FString("/Game/maps/BlasterMap?listen"));
		}
	}

}

void ALobbyGameMode::StartMatch()
{
	// Start the match logic
	Super::StartMatch();
}

void ALobbyGameMode::EndMatch()
{
	// Handle end of match logic
	Super::EndMatch();
}

void ALobbyGameMode::score()
{
	UE_LOG(MyLog, Warning, TEXT("This is just a test"));
}

void ALobbyGameMode::players()
{
}

void ALobbyGameMode::match()
{
}
