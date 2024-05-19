// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSION_API UMenu : public UUserWidget
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberofPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

public:
	UPROPERTY()
	class UUserWidget * UserWidgetInstance;
	  
	
protected:

	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	//
	// Callbacks for the custom delegates on the MultiplayerSessionSubsystem
	//

	UFUNCTION()
	void OnCreateSession(bool bWasSuccessfull);
	
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>&SessionResults, bool bWasSuccessful);
	void OnJoinSessions(EOnJoinSessionCompleteResult::Type Result);
	
	UFUNCTION()
	void OnDestroySessions(bool bWasSuccessful);
	
	UFUNCTION()
	void OnStartSessions(bool bWasSuccessful);


private:

	UPROPERTY(meta=(BindWidget))
	class UButton * HostButton;
	
	UPROPERTY(meta=(BindWidget))
	class UButton * JoinButton;

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	void MenuTearDown();

	// The subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem * MultiplayerSessionsSubsystem;

	int32 NumPublicConnections{4};
	FString MatchType{TEXT("FreeForAll")};
	FString PathToLobby{TEXT("")};
	
};