// EMorphus Software 2022


#include "Items/Weapons/Weapon.h"
#include "Characters/SlashCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/HitInterface.h"
#include "NiagaraComponent.h"

AWeapon::AWeapon( )
{
	WeaponBox = CreateDefaultSubobject<UBoxComponent>( TEXT( "WeaponBox" ) );
	WeaponBox->SetupAttachment( GetRootComponent( ) );
	WeaponBox->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	WeaponBox->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Overlap );
	WeaponBox->SetCollisionResponseToChannel( ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore );

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>( TEXT( "BoxTraceStart" ) );
	BoxTraceStart->SetupAttachment( GetRootComponent( ) );
	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>( TEXT( "BoxTraceEnd" ) );
	BoxTraceEnd->SetupAttachment( GetRootComponent( ) );
}
 
void AWeapon::BeginPlay( )
{
	Super::BeginPlay( );

	WeaponBox->OnComponentBeginOverlap.AddDynamic( this, &AWeapon::OnBoxOverlap );
}

void AWeapon::Equip( USceneComponent* InParent, FName SocketName, AActor* NewOwner, APawn* NewInstigator )
{
	SetOwner( NewOwner );
	SetInstigator( NewInstigator );
	AttachMeshToSocket( InParent, SocketName );
	ItemState = EItemState::EIS_Equipped;
	if ( EquipSound )
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquipSound,
			GetActorLocation( )
			);
	}
	if ( Sphere )
	{
		Sphere->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	}

	if ( EmbersEffect )
	{
		EmbersEffect->Deactivate();
	}
}

void AWeapon::AttachMeshToSocket( USceneComponent* InParent, FName SocketName )
{
	FAttachmentTransformRules TransformRules( EAttachmentRule::SnapToTarget, true );
	ItemMesh->AttachToComponent( InParent, TransformRules, SocketName );
}

void AWeapon::OnSphereOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	Super::OnSphereOverlap( OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult );
}

void AWeapon::OnSphereEndOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex )
{
	Super::OnSphereEndOverlap( OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex );
}

void AWeapon::OnBoxOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	const FVector Start = BoxTraceStart->GetComponentLocation( );
  	const FVector End = BoxTraceEnd->GetComponentLocation( );

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add( this );

	for ( AActor* Actor : IgnoreActors )
	{
		ActorsToIgnore.AddUnique( Actor );
	}
	FHitResult BoxHit;

	UKismetSystemLibrary::BoxTraceSingle(
		this,
		Start,
		End,
		FVector( 5.f, 5.f, 5.f ),
		BoxTraceStart->GetComponentRotation( ),
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		BoxHit,
		true 
	);
	if ( BoxHit.GetActor( ) )
	{
		UGameplayStatics::ApplyDamage(
			BoxHit.GetActor( ),
			Damage,
			GetInstigator( )->GetController( ),
			this,
			UDamageType::StaticClass( )
		);

		IHitInterface* HitInterface = Cast<IHitInterface>( BoxHit.GetActor( ) );
		if ( HitInterface )
		{
			HitInterface->Execute_GetHit( BoxHit.GetActor( ), BoxHit.ImpactPoint );
		}
		IgnoreActors.AddUnique( BoxHit.GetActor( ) );
		
		 CreateFields( BoxHit.ImpactPoint );

		 
	}
}
