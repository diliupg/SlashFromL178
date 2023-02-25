// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"	
#include "BaseCharacter.generated.h"

class AWeapon;
class UAttributeComponent;
class UAnimMontage;

UCLASS()
class SLASH_API ABaseCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	virtual void Tick( float DeltaTime ) override;

	UFUNCTION( BlueprintCallable )
	void SetWeaponCollisionEnabled( ECollisionEnabled::Type CollisionEnabled );

protected:

	virtual void BeginPlay() override;

	virtual void Attack( );

	
	virtual void Die( );

	virtual void PlayAttackMontage( );
	void PlayHitReactMontage( const FName SectionName );
	void DirectionalHitReact( const FVector& ImpactPoint );
	void PlayHitSound( const FVector& ImpactPoint );
	void SpawnJHitParticles( const FVector& ImpactPoint );
	virtual void HandleDamage( float DamageAmount );

	virtual bool CanAttack( );
	bool IsAlive();

	UFUNCTION( BlueprintCallable )
	virtual void AttackEnd( );

	UPROPERTY( VisibleAnywhere, Category = Weapon )
	AWeapon* EquippedWeapon;

  /*
  *Animation Montages
  */
	UPROPERTY( EditDefaultsOnly, Category = Montages )
	 UAnimMontage* AttackMontage;

	UPROPERTY( EditDefaultsOnly, Category = Montages )
	UAnimMontage* HitReactMontage;

	UPROPERTY( EditDefaultsOnly, Category = Montages )
	UAnimMontage* DeathMontage;

 /*
 * COMPONENTS
 */
	UPROPERTY( VisibleAnywhere )
	UAttributeComponent* Attributes;

private:

	UPROPERTY( EditAnywhere, Category = Sounds )
	USoundBase* HitSound;

	UPROPERTY( EditAnywhere, Category = VisualEffects )
	UParticleSystem* HitParticles;
  
};
