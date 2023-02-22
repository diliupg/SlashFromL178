// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Components/AttributeComponent.h"
#include <Kismet/GameplayStatics.h>

ABaseCharacter::ABaseCharacter()
{ 
	PrimaryActorTick.bCanEverTick = true;
	
	Attributes = CreateDefaultSubobject<UAttributeComponent>( TEXT( "Attributes" ) );
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseCharacter::Attack( )
{

}

void ABaseCharacter::Die( )
{

}

void ABaseCharacter::PlayHitReactMontage( const FName SectionName )
{
	UAnimInstance* AnimInstance = GetMesh( )->GetAnimInstance( );
	if ( AnimInstance && HitReactMontage )
	{
		AnimInstance->Montage_Play( HitReactMontage );
		AnimInstance->Montage_JumpToSection( SectionName, HitReactMontage );
	}
}

void ABaseCharacter::DirectionalHitReact( const FVector& ImpactPoint )
{
	const FVector Forward = GetActorForwardVector( );
	// lower impact point to the enemy's ActorLocation.Z
	const FVector ImpactLowered( ImpactPoint.X, ImpactPoint.Y, GetActorLocation( ).Z );
	const FVector ToHit = (ImpactLowered - GetActorLocation( )).GetSafeNormal( ); // normalized 

	//Forward * ToHit = |Forward||ToHit|*cos(theta)
	// |Forward| = 1, |ToHit| = 1, so Forward * ToHit = cos(theta)
	const double CosTheta = FVector::DotProduct( Forward, ToHit );
	// take the inverse cosine (arc-cosine) of cos(theta) to get theta
	double Theta = FMath::Acos( CosTheta );
	// convert from radians to degrees
	Theta = FMath::RadiansToDegrees( Theta );

	// if cross product points down, theta should be negative
	const FVector CrossProduct = FVector::CrossProduct( Forward, ToHit );
	if ( CrossProduct.Z < 0 )
	{
		Theta *= -1.f;
	}
	FName Section( "FromBack" );

	if ( Theta >= -45.f && Theta < 45.f )
	{
		Section = ("FromFront");
	}
	else if ( Theta >= -135.f && Theta < -45.f )
	{
		Section = ("FromLeft");
	}
	else if ( Theta >= 45.f && Theta < 135.f )
	{
		Section = ("FromRight");
	}

	PlayHitReactMontage( Section );
}

void ABaseCharacter::PlayHitSound( const FVector& ImpactPoint )
{
	if ( HitSound )
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint
		);
	}
}

void ABaseCharacter::SpawnJHitParticles( const FVector& ImpactPoint )
{
	if ( HitParticles )
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld( ),
			HitParticles,
			ImpactPoint
		);
	}
}

void ABaseCharacter::HandleDamage( float DamageAmount )
{
	if ( Attributes )
	{
		Attributes->ReceiveDamage( DamageAmount );
	}
}

void ABaseCharacter::PlayAttackMontage( )
{

}

bool ABaseCharacter::CanAttack( )
{
	return false;
}

bool ABaseCharacter::IsAlive( )
{
	return Attributes && Attributes->IsAlive( ) 
}

void ABaseCharacter::AttackEnd( )
{

}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseCharacter::SetWeaponCollisionEnabled( ECollisionEnabled::Type CollisionEnabled )
{
	if ( EquippedWeapon && EquippedWeapon->GetWeaponBox( ) )
	{
		EquippedWeapon->GetWeaponBox( )->SetCollisionEnabled( CollisionEnabled );
		EquippedWeapon->IgnoreActors.Empty( );
	}
}

