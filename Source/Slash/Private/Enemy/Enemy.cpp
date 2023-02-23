// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "AIController.h"
#include "Components//SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/AttributeComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "HUD/HealthBarComponent.h"
#include "Items/Weapons/Weapon.h" 
#include "Kismet/KismetSystemLibrary.h"

#include "Slash/DebugMacros.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh( )->SetCollisionObjectType( ECollisionChannel::ECC_WorldDynamic );
	GetMesh( )->SetCollisionResponseToChannel( ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block );
	GetMesh( )->SetCollisionResponseToChannel( ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore );
	GetMesh( )->SetGenerateOverlapEvents( true );
	GetCapsuleComponent()->SetCollisionResponseToChannel( ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore );
	
	
	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>( TEXT( "Health Bar" ) );
	HealthBarWidget->SetupAttachment( GetRootComponent( ) );

	GetCharacterMovement( )->bOrientRotationToMovement = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>( TEXT( "Pawn Sensing" ) );
	PawnSensing->SightRadius = 4000.f;
	PawnSensing->SetPeripheralVisionAngle( 45.f );
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	if ( HealthBarWidget )
	{
		HealthBarWidget->SetVisibility( false );
	}
	 
	EnemyController = Cast<AAIController>( GetController( ) );
	
	MoveToTarget( PatrolTarget );

	if ( PawnSensing )
	{
		PawnSensing->OnSeePawn.AddDynamic( this, &AEnemy::PawnSeen );
	}

	// attach weapon to enemy hand 
	UWorld* World = GetWorld( );
	if ( World && WeaponClass )
	{
		AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>( WeaponClass );
		DefaultWeapon->Equip( GetMesh( ), FName( "RightHandSocket" ), this, this );
		EquippedWeapon = DefaultWeapon;
	}  
}

void AEnemy::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if ( IsDead()) return;

	if ( EnemyState > EEnemyState::EES_Patrolling )
	{
		CheckCombatTarget( );
	}
	else
	{
		CheckPatrolTarget( );
	}
}

void AEnemy::PatrolTimerFinished( )
{
	MoveToTarget( PatrolTarget );
}

void AEnemy::HideHealthBar( )
{
	if ( HealthBarWidget )
	{
		HealthBarWidget->SetVisibility( false );
	}
}

void AEnemy::ShowHealthBar( )
{
	if ( HealthBarWidget )
	{
		HealthBarWidget->SetVisibility( true );
	}
}

void AEnemy::LoseInterest( )
{
	CombatTarget = nullptr;
	HideHealthBar( );
}

void AEnemy::StartPatrolling( )
{
	EnemyState = EEnemyState::EES_Patrolling;
	GetCharacterMovement( )->MaxWalkSpeed = PatrollingSpeed;
	MoveToTarget( PatrolTarget );
}

void AEnemy::ChaseTarget( )
{
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement( )->MaxWalkSpeed = chaseSpeed;
	MoveToTarget( CombatTarget );
}

bool AEnemy::IsOutsideCombatRadius( )
{
	return !InTargetRange( CombatTarget, CombatRadius );
}

bool AEnemy::IsOutsideAttackRadius( )
{
	return !InTargetRange( CombatTarget, AttackRadius );
}

bool AEnemy::IsInsideAttackRadius( )
{
	return InTargetRange( CombatTarget, AttackRadius );
}

bool AEnemy::IsChasing( )
{
	return EnemyState == EEnemyState::EES_Chasing;
}

bool AEnemy::IsAttacking  ( )
{
	return EnemyState == EEnemyState::EES_Attacking; 
}

bool AEnemy::IsDead( )
{
	return EnemyState == EEnemyState::EES_Dead;
}

bool AEnemy::IsEngaged( )
{
	return EnemyState == EEnemyState::EES_Engaged;
}

void AEnemy::ClearPatrolTimer( )
{
	GetWorldTimerManager( ).ClearTimer( PatrolTimer );
}

void AEnemy::StartAttackTimer( )
{
	EnemyState = EEnemyState::EES_Attacking;
	const float AttackTime = FMath::RandRange( AttackMin, AttackMax );
	GetWorldTimerManager().SetTimer( AttackTimer, this, &AEnemy::Attack, AttackTime );
}

void AEnemy::ClearAttackTimer( )
{
	GetWorldTimerManager( ).ClearTimer( AttackTimer );
}

AActor* AEnemy::ChoosePatrolTarget( )
{
	TArray<AActor*> ValidTargets;
	for ( AActor* Target : PatrolTargets )
	{
		if ( Target != PatrolTarget )
		{
			ValidTargets.AddUnique( Target );
		}
	}

	const int32 NumPatrolTargets = ValidTargets.Num( );
	if ( NumPatrolTargets > 0 )
	{
		const int32 TargetSelection = FMath::RandRange( 0, NumPatrolTargets - 1 );
		return PatrolTargets[TargetSelection];
	}
	return nullptr;
}

void AEnemy::Attack( )
{
	Super::Attack( );
	PlayAttackMontage( );
}

bool AEnemy::CanAttack( )
{
	bool bCanAttack = IsInsideAttackRadius( ) &&
		!IsAttacking( ) &&
		!IsDead();
	return bCanAttack;
}

void AEnemy::HandleDamage( float DamageAmount )
{
	Super::HandleDamage( DamageAmount );
	if ( Attributes && HealthBarWidget )
	{
		HealthBarWidget->SetHealthPercent( Attributes->GetHealthPercent( ) );
	}
}

void AEnemy::MoveToTarget( AActor* Target )
{
	if ( EnemyController == nullptr || Target == nullptr ) return;
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor( Target );
	MoveRequest.SetAcceptanceRadius( 60.f );

	EnemyController->MoveTo( MoveRequest );
}

/*
*  enemy sees the player and starts chasing
*/
void AEnemy::PawnSeen( APawn* SeenPawn )
{
	const bool bShouldChaseTarget =
		EnemyState != EEnemyState::EES_Dead &&
		EnemyState != EEnemyState::EES_Chasing &&
		EnemyState < EEnemyState::EES_Attacking &&
		SeenPawn->ActorHasTag( FName( "SlashCharacter" ) );

	if ( bShouldChaseTarget )
	{
		CombatTarget = SeenPawn;
		ClearPatrolTimer( );
		ChaseTarget( );

	}
}

bool AEnemy::InTargetRange( AActor* Target, double Radius )
{
	if ( Target == nullptr ) return false;
	const double DistanceToTarget = (Target->GetActorLocation( ) - GetActorLocation( )).Size( );
	//DRAW_SPHERE_SingleFrame( GetActorLocation( ) );
	//DRAW_SPHERE_SingleFrame( Target->GetActorLocation( ) );

	return DistanceToTarget <= Radius;
}

void AEnemy::CheckPatrolTarget( )
{
	if ( InTargetRange( PatrolTarget, PatrolRadius ) )
	{
		PatrolTarget = ChoosePatrolTarget( );
		const float WaitTime = FMath::RandRange( WaitMin, WaitMax );
		GetWorldTimerManager( ).SetTimer( PatrolTimer, this, &AEnemy::PatrolTimerFinished, WaitTime );
	}
}

void AEnemy::CheckCombatTarget( )
{
	if ( IsOutsideCombatRadius() )
	{
		ClearAttackTimer( );
		LoseInterest( );
		if ( !IsEngaged( ) ) StartPatrolling( );
	}
	else if ( IsOutsideAttackRadius( ) && !IsChasing() )
	{
		ClearAttackTimer( );
		if( !IsEngaged() ) ChaseTarget( );
	}
	else if ( CanAttack() )
	{
		StartPatrolling( );
	}
}

void AEnemy::GetHit_Implementation( const FVector& ImpactPoint ) 
{
	ShowHealthBar( );

	if ( IsAlive( ) )
	{
		DirectionalHitReact( ImpactPoint );
	}
	else Die( );

	PlayHitSound( ImpactPoint );
	SpawnJHitParticles( ImpactPoint );
}

float AEnemy::TakeDamage( float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser )
{
	HandleDamage( DamageAmount );
	CombatTarget = EventInstigator->GetPawn( );
	ChaseTarget( );
	return DamageAmount;
}

void AEnemy::Destroyed( )
{
	if ( EquippedWeapon )
	{
		EquippedWeapon->Destroy( );
	}
}

void AEnemy::Die( )
{
	UAnimInstance* AnimInstance = GetMesh( )->GetAnimInstance( );
	if ( AnimInstance && DeathMontage )
	{
		AnimInstance->Montage_Play( DeathMontage );

		const int32 Selection = FMath::RandRange( 0, 5 );
		FName SectionName = FName( );
		switch ( Selection )
		{
		case 0:
			SectionName = FName( "Death1" );
			DeathPose = EDeathPose::EDP_Death1;
			break;
		case 1:
			SectionName = FName( "Death2" );
			DeathPose = EDeathPose::EDP_Death2;
			break;
		case 2:
			SectionName = FName( "Death3" );
			DeathPose = EDeathPose::EDP_Death3;
			break;
		case 3:
			SectionName = FName( "Death4" );
			DeathPose = EDeathPose::EDP_Death4;
			break;
		case 4:
			SectionName = FName( "Death5" );
			DeathPose = EDeathPose::EDP_Death5;
			break;
		case 5:
			SectionName = FName( "Death6" );
			DeathPose = EDeathPose::EDP_Death6;
			break;
		default:
			break;
		}

		AnimInstance->Montage_JumpToSection( SectionName, DeathMontage );
	}

	HideHealthBar( );

	GetCapsuleComponent( )->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	SetLifeSpan( 3.f );
}
