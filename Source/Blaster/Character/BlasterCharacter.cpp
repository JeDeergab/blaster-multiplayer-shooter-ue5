
#include "BlasterCharacter.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "BlasterAnimInstance.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	GetMesh()->SetVisibility(true);

	// Construct the current weapon
	// We will set the settings inside FirstPersonView() and ThirdPersonView()
	CurrentWeapon = CreateDefaultSubobject<AWeapon>(TEXT("CurrentWeapon"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	
	CloseUpCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CloseUpCameraBoom"));
	CloseUpCameraBoom->TargetArmLength = 300.f;
	CloseUpCameraBoom->CameraLagSpeed = 10.f;
	CloseUpCameraBoom->bEnableCameraLag = true;
	CloseUpCameraBoom->bUsePawnControlRotation = false;
	
	// Initialize FollowCamera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Initialize CloseUpCamera
	CloseUpCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CloseUpCamera"));
	CloseUpCamera->SetupAttachment(GetMesh(), FName("head"));
	CloseUpCamera->bUsePawnControlRotation = true;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetBoxExtent(FVector(50.f, 50.f, 50.f)); // Set the size of the box collision

	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Enable collision queries (overlap tests)
	BoxComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Set the collision object type
	BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap); // Respond to all channels with overlap
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECR_Overlap); // Ensure it overlaps with physics bodies
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap); // Ensure it overlaps with pawns
	
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCharacterMovement()-> RotationRate = FRotator(0.f,850.f,0.f);
	
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
	
	
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUp);
	
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", EInputEvent::IE_Pressed, this, &ThisClass::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", EInputEvent::IE_Pressed, this, &ThisClass::CrouchButtonPressed);

	PlayerInputComponent->BindAction("Aim", EInputEvent::IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", EInputEvent::IE_Released, this, &ThisClass::AimButtonReleased);

	PlayerInputComponent->BindAction("Fire", EInputEvent::IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", EInputEvent::IE_Released, this, &ThisClass::FireButtonReleased);

	PlayerInputComponent->BindAction("FirstPerson", EInputEvent::IE_Pressed, this, &ThisClass::FirstPersonView);
	PlayerInputComponent->BindAction("ThirdPerson", EInputEvent::IE_Pressed, this, &ThisClass::ThirdPersonView);


	
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{

	if(Combat && Combat->EquippedWeapon == nullptr) return;
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAIr = GetCharacterMovement()->IsFalling();

	if(Speed == 0.f && !bIsInAIr) // Standing still, not jumping
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw,0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}

	if(Speed > 0.f || bIsInAIr) // Running or Jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	
	if(AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from range [270, 360] to [-90, 0]
		FVector2d InRange(270.f, 360.f);
		FVector2D OutRange (-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
	
}

void ABlasterCharacter::FireButtonPressed()
{
	if(Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if(Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::FirstPersonView()
{
	if(IsLocallyControlled())
	{
		// Assuming you want to switch to CloseUpCamera on any overlap, you can add checks here if needed
		FollowCamera->SetActive(false);
		CloseUpCamera->SetActive(true);
		CurrentWeapon->GetWeaponMesh()->SetVisibility(true); // Sets the visibility for the weapon
		CurrentWeapon->GetWeaponMesh()->SetIsReplicated(true); // Sets the replication for the weapon
	}
	// GetMesh()->SetVisibility(false);
	// TODO: GetMesh() is not set to replicated must be in Combat Component
}
void ABlasterCharacter::ThirdPersonView()
{
	if(IsLocallyControlled())
	{
		// Assuming you want to switch to CloseUpCamera on any overlap, you can add checks here if needed
		FollowCamera->SetActive(true);
		CloseUpCamera->SetActive(false);
		CurrentWeapon->GetWeaponMesh()->SetVisibility(false); // Sets the visibility for the weapon
		CurrentWeapon->GetWeaponMesh()->SetIsReplicated(true); // Sets the replication for the weapon
	}
	// GetMesh()->SetVisibility(true);
	// TODO: GetMesh() is not set to replicated must be in Combat Component
}

void ABlasterCharacter::Jump()
{
	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if(AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if(TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 10.f);
		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw) <15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
	
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(Combat)
	{
		Combat->Character = this;
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if(Combat)
	{
		if(HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if(bIsCrouched == true )
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if(Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if(Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance * AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleName") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
	
}

void ABlasterCharacter::MoveForward(float value)
{
	if(Controller != nullptr && value != 0.f)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, value);
	}
}

void ABlasterCharacter::MoveRight(float value)
{
	if (Controller != nullptr && value != 0.f)
	{
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, value);
	}
}

void ABlasterCharacter::Turn(float value)
{
	AddControllerYawInput(value);
}

void ABlasterCharacter::LookUp(float value)
{
	AddControllerPitchInput(value);
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
	
}



void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon * LastWeapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if(LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	
	OverlappingWeapon = Weapon;

	if(IsLocallyControlled())
	{
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if(Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

bool ABlasterCharacter::isWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon *ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}
