// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Item.h"
#include "Slash/DebugMacros.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Characters/SlashCharacter.h"
#include "NiagaraComponent.h"


// Sets default values
AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "ItemMeshComponent" ) );
	RootComponent = ItemMesh;

	Sphere = CreateDefaultSubobject<USphereComponent>( TEXT( "Sphere" ) );
	Sphere->SetupAttachment( GetRootComponent( ) );

	EmbersEffect = CreateDefaultSubobject<UNiagaraComponent>( TEXT( "Embers" ) );
	EmbersEffect->SetupAttachment( GetRootComponent( ) );
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	/*int32 AvgInt = Avg<int32>( 5, 3 );
	UE_LOG( LogTemp, Warning, TEXT( "The average of 5 and 3 is: %d" ), AvgInt );*/

	Sphere->OnComponentBeginOverlap.AddDynamic( this, &AItem::OnSphereOverlap );
	Sphere->OnComponentEndOverlap.AddDynamic( this, &AItem::OnSphereEndOverlap );
}

float AItem::TransformedSin( )
{
	return Amplitude* FMath::Sin( RunningTime * TimeConstant );
}

float AItem::TransformedCos( )
{
	return Amplitude * FMath::Cos( RunningTime * TimeConstant );
}


void AItem::OnSphereOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	ASlashCharacter* SlashCharacter = Cast<ASlashCharacter>( OtherActor );
	if ( SlashCharacter )
	{
		SlashCharacter->SetOverlappingItem( this );
	}
}


void AItem::OnSphereEndOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex )
{
	ASlashCharacter* SlashCharacter = Cast<ASlashCharacter>( OtherActor );
	if ( SlashCharacter )
	{
		SlashCharacter->SetOverlappingItem( nullptr );
	}
}

void AItem::Tick(float DeltaTime)

{
	Super::Tick(DeltaTime);
	
	if ( ItemState == EItemState::EIS_Hovering )
	{
		RunningTime += DeltaTime;
		AddActorWorldOffset( FVector( 0.f, 0.f, TransformedSin( ) ) );
	}
	
}

