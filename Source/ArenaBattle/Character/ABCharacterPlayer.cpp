// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ABCharacterControlData.h"
#include "UI/ABHUDWidget.h"
#include "CharacterStat/ABCharacterStatComponent.h"
#include "Interface/ABGameInterface.h"
#include "ArenaBattle.h"
#include "Components/CapsuleComponent.h"
#include "Physics/ABCollision.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "ABCharacterMovementComponent.h"

AABCharacterPlayer::AABCharacterPlayer(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UABCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Camera
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Input
	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionJumpRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_Jump.IA_Jump'"));
	if (nullptr != InputActionJumpRef.Object)
	{
		JumpAction = InputActionJumpRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputChangeActionControlRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ChangeControl.IA_ChangeControl'"));
	if (nullptr != InputChangeActionControlRef.Object)
	{
		ChangeControlAction = InputChangeActionControlRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionShoulderMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ShoulderMove.IA_ShoulderMove'"));
	if (nullptr != InputActionShoulderMoveRef.Object)
	{
		ShoulderMoveAction = InputActionShoulderMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionShoulderLookRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ShoulderLook.IA_ShoulderLook'"));
	if (nullptr != InputActionShoulderLookRef.Object)
	{
		ShoulderLookAction = InputActionShoulderLookRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionQuaterMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_QuaterMove.IA_QuaterMove'"));
	if (nullptr != InputActionQuaterMoveRef.Object)
	{
		QuaterMoveAction = InputActionQuaterMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionAttackRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_Attack.IA_Attack'"));
	if (nullptr != InputActionAttackRef.Object)
	{
		AttackAction = InputActionAttackRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionTeleportRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_Teleport.IA_Teleport'"));
	if (nullptr != InputActionTeleportRef.Object)
	{
		TeleportAction = InputActionTeleportRef.Object;
	}

	CurrentCharacterControlType = ECharacterControlType::Quater;

	bCanAttack = true;
}

void AABCharacterPlayer::BeginPlay()
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));
	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		EnableInput(PlayerController);
	}

	SetCharacterControl(CurrentCharacterControlType);
}

void AABCharacterPlayer::SetDead()
{
	Super::SetDead();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		DisableInput(PlayerController);
	}
}

void AABCharacterPlayer::PossessedBy(AController* NewController)
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		AB_LOG(LogABNetwork, Log, TEXT("Owner : %s"), *OwnerActor->GetName());
	}
	else
	{
		AB_LOG(LogABNetwork, Log, TEXT("%s"),TEXT("No Owner"));
	}
	Super::PossessedBy(NewController);

	OwnerActor = GetOwner();
	if (OwnerActor)
	{
		AB_LOG(LogABNetwork, Log, TEXT("Owner : %s"), *OwnerActor->GetName());
	}
	else
	{
		AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("No Owner"));
	}

	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AABCharacterPlayer::OnRep_Owner()
{
	AB_LOG(LogABNetwork, Log, TEXT("%s %s"),*GetName(), TEXT("Begin"));
	Super::OnRep_Owner();

	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		AB_LOG(LogABNetwork, Log, TEXT("Owner : %s"), *OwnerActor->GetName());
	}
	else
	{
		AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("No Owner"));
	}


	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AABCharacterPlayer::PostNetInit()
{
	AB_LOG(LogABNetwork, Log, TEXT("%s %s"), TEXT("Begin"), *GetName());
	
	Super::PostNetInit();

	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AABCharacterPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInputComponent->BindAction(ChangeControlAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ChangeCharacterControl);
	EnhancedInputComponent->BindAction(ShoulderMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderMove);
	EnhancedInputComponent->BindAction(ShoulderLookAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderLook);
	EnhancedInputComponent->BindAction(QuaterMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::QuaterMove);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::Attack);
	EnhancedInputComponent->BindAction(TeleportAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::Teleport);
}

void AABCharacterPlayer::ChangeCharacterControl()
{
	if (CurrentCharacterControlType == ECharacterControlType::Quater)
	{
		SetCharacterControl(ECharacterControlType::Shoulder);
	}
	else if (CurrentCharacterControlType == ECharacterControlType::Shoulder)
	{
		SetCharacterControl(ECharacterControlType::Quater);
	}
}

void AABCharacterPlayer::SetCharacterControl(ECharacterControlType NewCharacterControlType)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	UABCharacterControlData* NewCharacterControl = CharacterControlManager[NewCharacterControlType];
	check(NewCharacterControl);

	SetCharacterControlData(NewCharacterControl);

	APlayerController* PlayerController = CastChecked<APlayerController>(GetController());
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		UInputMappingContext* NewMappingContext = NewCharacterControl->InputMappingContext;
		if (NewMappingContext)
		{
			Subsystem->AddMappingContext(NewMappingContext, 0);
		}
	}

	CurrentCharacterControlType = NewCharacterControlType;
}

void AABCharacterPlayer::SetCharacterControlData(const UABCharacterControlData* CharacterControlData)
{
	Super::SetCharacterControlData(CharacterControlData);

	CameraBoom->TargetArmLength = CharacterControlData->TargetArmLength;
	CameraBoom->SetRelativeRotation(CharacterControlData->RelativeRotation);
	CameraBoom->bUsePawnControlRotation = CharacterControlData->bUsePawnControlRotation;
	CameraBoom->bInheritPitch = CharacterControlData->bInheritPitch;
	CameraBoom->bInheritYaw = CharacterControlData->bInheritYaw;
	CameraBoom->bInheritRoll = CharacterControlData->bInheritRoll;
	CameraBoom->bDoCollisionTest = CharacterControlData->bDoCollisionTest;
}

void AABCharacterPlayer::ShoulderMove(const FInputActionValue& Value)
{
	if (!bCanAttack)
	{
		return;
	}
	FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.X);
	AddMovementInput(RightDirection, MovementVector.Y);
}

void AABCharacterPlayer::ShoulderLook(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AABCharacterPlayer::QuaterMove(const FInputActionValue& Value)
{
	if (!bCanAttack)
	{
		return;
	}
	FVector2D MovementVector = Value.Get<FVector2D>();

	float InputSizeSquared = MovementVector.SquaredLength();
	float MovementVectorSize = 1.0f;
	float MovementVectorSizeSquared = MovementVector.SquaredLength();
	if (MovementVectorSizeSquared > 1.0f)
	{
		MovementVector.Normalize();
		MovementVectorSizeSquared = 1.0f;
	}
	else
	{
		MovementVectorSize = FMath::Sqrt(MovementVectorSizeSquared);
	}

	FVector MoveDirection = FVector(MovementVector.X, MovementVector.Y, 0.0f);
	GetController()->SetControlRotation(FRotationMatrix::MakeFromX(MoveDirection).Rotator());
	AddMovementInput(MoveDirection, MovementVectorSize);
}

void AABCharacterPlayer::Attack()
{
	//ProcessComboCommand();

	if (bCanAttack)
	{
		if (!HasAuthority())
		{
			bCanAttack = false;
			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
			FTimerHandle Handle;
			GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([&]
				{

					bCanAttack = true;
					GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
				}
			), AttackTime, false, -1.0f);

			PlayAttackAnimation();
		}

		// 현재 내가 공격하는 시간 정보를 서버에 보냄
		// 현재 월드의 시간 
		// 주의할 점) 서버 월드의 시간과 클라이언트 월드의 시간은 다름
		// 클라이언트의 월드는 서버를 시뮬레이션한 월드 , 우리가 클라이언트는 늦게 생성하기에 서버의 시간보다 늦게 흘러갈 수 밖에 없다.
		// 클라이언트 측에서 서버의 시간을 가져와서 서버에 넘겨줘야함 
		// ( 클라이언트의 시간 : GetWorld()->GetTimeSeconds(); , 서버측 시간 가져오기 : GetWorld()->GetGameState()->GetServerWorldTimeSeconds()
		
		ServerRPCAttack(GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
		
	}
}

void AABCharacterPlayer::PlayAttackAnimation()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	// 혹시나 몽타주가 실행중이면, 멈춤
	AnimInstance->StopAllMontages(0.0f);
	AnimInstance->Montage_Play(ComboActionMontage);
}

void AABCharacterPlayer::AttackHitConfirm(AActor* HitActor)
{
	// 서버, 클라에서도 호출 
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));
	if (HasAuthority())
	{
		//서버에서만 확정 처리
		
		const float AttackDamage = Stat->GetTotalStat().Attack;
		FDamageEvent DamageEvent;
		HitActor->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
		
	}
}

void AABCharacterPlayer::DrawDebugAttackRange(const FColor& DrawColor, FVector TraceStart, FVector TraceEnd, FVector Forward)
{
	//디버그 드로우
#if ENABLE_DRAW_DEBUG

	const float AttackRange = Stat->GetTotalStat().AttackRange;
	const float AttackRadius = Stat->GetAttackRadius();
	FVector CapsuleOrigin = TraceStart + (TraceEnd - TraceStart) * 0.5f;
	float CapsuleHalfHeight = AttackRange * 0.5f;
	DrawDebugCapsule(GetWorld(), CapsuleOrigin, CapsuleHalfHeight, AttackRadius, FRotationMatrix::MakeFromZ(GetActorForwardVector()).ToQuat(), DrawColor, false, 5.0f);

#endif
}


void AABCharacterPlayer::AttackHitCheck()
{
	if (IsLocallyControlled())
	{
		AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));

		FHitResult OutHitResult;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(Attack), false, this);

		const float AttackRange = Stat->GetTotalStat().AttackRange;
		const float AttackRadius = Stat->GetAttackRadius();
		const float AttackDamage = Stat->GetTotalStat().Attack;
		const FVector Forward = GetActorForwardVector();
		const FVector Start = GetActorLocation() + Forward * GetCapsuleComponent()->GetScaledCapsuleRadius();
		const FVector End = Start + GetActorForwardVector() * AttackRange;

		bool HitDetected = GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, CCHANNEL_ABACTION, FCollisionShape::MakeSphere(AttackRadius), Params);

		// 서버 시간
		float HitCheckTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

		if (!HasAuthority())
		{
			// 클라이언트
			// 판정 처리 후 결과를 서버에 전송 검증받기

			// 클라이언트 -> 서버 판정을 위한 RPC 헤더 선언
			if (HitDetected)
			{
				ServerRPCNotifyHit(OutHitResult, HitCheckTime);
			}
			else
			{
				ServerRPCNotifyMiss(Start, End, Forward, HitCheckTime);
			}

		}
		else
		{
			// 서버이자 플레이어
			// 패킷을 보낼 필요없이 바로 판정 처리
			FColor DebugColor = HitDetected ? FColor::Green : FColor::Red;
			DrawDebugAttackRange(DebugColor, Start, End, Forward);
			if (HitDetected)
			{
				AttackHitConfirm(OutHitResult.GetActor());
			}

		}


	}
	
}

bool AABCharacterPlayer::ServerRPCAttack_Validate(float AttackStartTime)
{
	// 클라이언트에 의존을 많이하다 보니 공격 타이밍에 대한 검증
	if (LastAttackStartTime == 0.0f)
	{
		return true;
	}
	// 마지막으로 공격한 시간과 다시 공격한 시간이 기본으로 설정된 공격 시간보다 작게 되면 어디 문제( 다른 사람보다 많이 공격하게 되는거이니 ) 가 있는것이니 
	return (AttackStartTime - LastAttackStartTime) > AttackTime;
}


void AABCharacterPlayer::ServerRPCAttack_Implementation(float AttackStartTime)
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));

	bCanAttack = false;
	OnRep_CanAttack();

	// 서버는 AttackTime이 클라이언트로부터 패킷을 받은 시간과 서버의 시간 차이 존재
	AttackTimeDifference = GetWorld()->GetTimeSeconds() - AttackStartTime;
	AB_LOG(LogABNetwork, Log, TEXT("LagTime : %f"), AttackTimeDifference);
	// 어택 타임을 모두 빼버리면 타이머가 동작 안할수도있으니 조금만 뺌
	AttackTimeDifference = FMath::Clamp(AttackTimeDifference, 0.0f, AttackTime - 0.01f);

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([&]
	{

			bCanAttack = true;
			OnRep_CanAttack();
	}
	), AttackTime - AttackTimeDifference, false, -1.0f);
	
	// 마지막으로 공격한 시간 기록
	LastAttackStartTime = AttackStartTime;

	// 서버에서 공격모션 애니메이션 재생
	PlayAttackAnimation();

	//MulticastRPCAttack();
	for (APlayerController* PlayerController : TActorRange<APlayerController>(GetWorld()))
	{
		if (PlayerController && GetController() != PlayerController)
		{
			// 현재 플레이어컨트롤러가 현재 내가 로직을 실행하는 플레이어컨트롤러와 다를경우는 Simulated Proxy , 즉 다른 클라이언트가 되니까 다 모션수행
			if (!PlayerController->IsLocalController())
			{
				//여기 온 플레이어컨트롤러는 서버도 아니고 공격수행을 내린 클라이언트도 아닌 Simulated Proxy
				AABCharacterPlayer* OtherPlayer = Cast<AABCharacterPlayer>(PlayerController->GetPawn());
				if (OtherPlayer)
				{
					OtherPlayer->ClientRPCPlayAnimation(this);
				}
			}
		}
	}
}

void AABCharacterPlayer::ClientRPCPlayAnimation_Implementation(AABCharacterPlayer* CharacterToPlay)
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));
	if (CharacterToPlay)
	{
		CharacterToPlay->PlayAttackAnimation();
	}
}


void AABCharacterPlayer::MulticastRPCAttack_Implementation()
{
	// 중요 로직보다는 무언가 재생하는 것들 Unrealiable 이기 때문에
	if (!IsLocallyControlled())
	{
		//이미 현재 클라이언트는 재생을 했기 때문에 나머지 클라이언트들의 애니메이션 재생만 해주자.
		PlayAttackAnimation();
	}
}

bool AABCharacterPlayer::ServerRPCNotifyHit_Validate(const FHitResult& HitResult, float HitCheckTime)
{

	return (HitCheckTime - LastAttackStartTime) > AcceptMinCheckTime;
}




void AABCharacterPlayer::ServerRPCNotifyHit_Implementation(const FHitResult& HitResult, float HitCheckTime)
{
	AActor* HitActor = HitResult.GetActor();
	if (::IsValid(HitActor))
	{
		const FVector HitLocation = HitResult.Location;
		const FBox HitBox = HitActor->GetComponentsBoundingBox();
		const FVector ActorBoxCenter = (HitBox.Min + HitBox.Max) * 0.5f;
		if (FVector::DistSquared(HitLocation, ActorBoxCenter) <= AcceptCheckDistance * AcceptCheckDistance)
		{
			AttackHitConfirm(HitActor);
		}
		else
		{
			AB_LOG(LogABNetwork, Warning, TEXT("%s"), TEXT("HitTest Rejected ! "));
		}
#if ENABLE_DRAW_DEBUG
		DrawDebugPoint(GetWorld(), ActorBoxCenter, 50.0f, FColor::Cyan, false, 5.0f);
		DrawDebugPoint(GetWorld(), HitLocation, 50.0f, FColor::Magenta, false, 5.0f);
#endif

		DrawDebugAttackRange(FColor::Green, HitResult.TraceStart, HitResult.TraceEnd, HitActor->GetActorForwardVector());
	}

}


bool AABCharacterPlayer::ServerRPCNotifyMiss_Validate(FVector_NetQuantizeNormal TraceStart, FVector_NetQuantizeNormal TraceEnd, FVector_NetQuantizeNormal TraceDir ,float HitCheckTime)
{

	return (HitCheckTime - LastAttackStartTime) > AcceptMinCheckTime;
}

void AABCharacterPlayer::ServerRPCNotifyMiss_Implementation(FVector_NetQuantizeNormal TraceStart, FVector_NetQuantizeNormal TraceEnd, FVector_NetQuantizeNormal TraceDir ,float HitCheckTime)
{
	DrawDebugAttackRange(FColor::Red, TraceStart, TraceEnd, TraceDir);
}

void AABCharacterPlayer::OnRep_CanAttack()
{
	if (!bCanAttack)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}
}

void AABCharacterPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AABCharacterPlayer, bCanAttack);
}

void AABCharacterPlayer::SetupHUDWidget(UABHUDWidget* InHUDWidget)
{
	if (InHUDWidget)
	{
		InHUDWidget->UpdateStat(Stat->GetBaseStat(), Stat->GetModifierStat());
		InHUDWidget->UpdateHpBar(Stat->GetCurrentHp());

		Stat->OnStatChanged.AddUObject(InHUDWidget, &UABHUDWidget::UpdateStat);
		Stat->OnHpChanged.AddUObject(InHUDWidget, &UABHUDWidget::UpdateHpBar);
	}
}

void AABCharacterPlayer::Teleport()
{
	AB_LOG(LogABTeleport, Log, TEXT("%s"), TEXT("Begin"));
	UABCharacterMovementComponent* ABMovementComp = Cast<UABCharacterMovementComponent>(GetCharacterMovement());
	if (ABMovementComp)
	{
		ABMovementComp->SetTeleportCommand();
	}
}
