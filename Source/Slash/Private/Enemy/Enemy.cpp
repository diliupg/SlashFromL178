// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "AIController.h"
#include "Components//SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/AttributeComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "HUD/HealthBarComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

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
}

void AEnemy::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

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

void AEnemy::MoveToTarget( AActor* Target )
{
	if ( EnemyController == nullptr || Target == nullptr ) return;
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor( Target );
	MoveRequest.SetAcceptanceRadius( 15.f );

	EnemyController->MoveTo( MoveRequest );
}

/*
*  enemy sees the player and starts chasing
*/
void AEnemy::PawnSeen( APawn* SeenPawn )
{
	if ( EnemyState == EEnemyState::EES_Chasing ) return;

	if ( SeenPawn->ActorHasTag( FName( "SlashCharacter" ) ) )
	{
		GetWorldTimerManager( ).ClearTimer( PatrolTimer );
		GetCharacterMovement( )->MaxWalkSpeed = RunSpeed;
		CombatTarget = SeenPawn;

		if ( EnemyState != EEnemyState::EES_Attacking )
		{
			EnemyState = EEnemyState::EES_Chasing;
			MoveToTarget( CombatTarget );
			UE_LOG( LogTemp, Warning, TEXT( "Pawn seen, chase player" ) );
		}	
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
	if ( !InTargetRange( CombatTarget, CombatRadius ) )
	{
		// Outside Combat radius, lose interest
		CombatTarget = nullptr;
		if ( HealthBarWidget )
		{
			HealthBarWidget->SetVisibility( false );
		}
		EnemyState = EEnemyState::EES_Patrolling;
		GetCharacterMovement( )->MaxWalkSpeed = 125.f;
		MoveToTarget( PatrolTarget );
		UE_LOG( LogTemp, Warning, TEXT( "Lose Interest" ) );
	}
	else if ( !InTargetRange( CombatTarget, AttackRadius ) && EnemyState != EEnemyState::EES_Chasing )
	{
		// outside attack range, chase player
		EnemyState = EEnemyState::EES_Chasing;
		GetCharacterMovement( )->MaxWalkSpeed = 300.f;
		MoveToTarget( CombatTarget );
		UE_LOG( LogTemp, Warning, TEXT( "Chase Player" ) );
	}
	else if ( InTargetRange( CombatTarget, AttackRadius ) && EnemyState != EEnemyState::EES_Attacking )
	{
		// inside attack range, attack player
		EnemyState = EEnemyState::EES_Attacking;
		// TODO Attack Montage
		UE_LOG( LogTemp, Warning, TEXT( "Attack Player" ) );
	}
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::GetHit_Implementation( const FVector& ImpactPoint ) 
{
	//DRAW_SPHERE_COLOR( ImpactPoint, FColor::Orange);
	if ( HealthBarWidget )
	{
		HealthBarWidget->SetVisibility( true );
	}

	if ( Attributes && Attributes->IsAlive() )
	{
		DirectionalHitReact( ImpactPoint );
	}
	else
	{
		Die( );
	}

	if ( HitSound )
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint
		);
	}
	if ( HitParticles )
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld( ),
			HitParticles,
			ImpactPoint
		);
	}
}

float AEnemy::TakeDamage( float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser )
{
	if ( Attributes && HealthBarWidget )
	{
		Attributes->ReceiveDamage( DamageAmount );
		HealthBarWidget->SetHealthPercent( Attributes->GetHealthPercent() );
	}
	CombatTarget = EventInstigator->GetPawn( );
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement( )->MaxWalkSpeed = 300.f;
	MoveToTarget( CombatTarget );
	return DamageAmount;
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

	if ( HealthBarWidget )
	{
		HealthBarWidget->SetVisibility( false );
	}

	GetCapsuleComponent( )->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	SetLifeSpan( 3.f );
}
