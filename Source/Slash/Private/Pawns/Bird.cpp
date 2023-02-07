// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/Bird.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

// Sets default values
ABird::ABird()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>( TEXT( "Capsule" ) );
	Capsule->SetCapsuleHalfHeight( 20.f );
	Capsule->SetCapsuleRadius( 15.f );
	SetRootComponent(Capsule);

	BirdMesh = CreateDefaultSubobject<USkeletalMeshComponent>( TEXT( "BirdMesh" ) );
	BirdMesh->SetupAttachment( GetRootComponent( ) );

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>( TEXT( "CameraBoom" ) );
	CameraBoom->SetupAttachment( GetRootComponent( ) );
	CameraBoom->TargetArmLength = 300.f;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>( TEXT( "ViewCamera" ) );
	ViewCamera->SetupAttachment( CameraBoom );

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void ABird::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>( GetController() );
	if ( PlayerController )
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>( PlayerController->GetLocalPlayer( ) );
		if ( Subsystem )
		{
			Subsystem->AddMappingContext( BirdMappingContext, 0 );
		}
	}
	
}

void ABird::Move( const FInputActionValue& Value )
{
	const float DirectionValue = Value.Get<float>( );
 
	if ( Controller && (DirectionValue != 0 ) )
	{
		FVector Forward = GetActorForwardVector( );
		AddMovementInput( Forward, DirectionValue );
	}
}

void ABird::Look( const FInputActionValue& Value )
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>( );

	if ( GetController( ) )
	{
		AddControllerYawInput( LookAxisValue.X );
		AddControllerPitchInput( LookAxisValue.Y );
	}
}

// Called every frame
void ABird::Tick(float DeltaTime)
{ 
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABird::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if ( UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>( PlayerInputComponent ) )
	{
		EnhancedInputComponent->BindAction( MoveAction, ETriggerEvent::Triggered, this, &ABird::Move );
		EnhancedInputComponent->BindAction( LookAction, ETriggerEvent::Triggered, this, &ABird::Look );
	}

}

