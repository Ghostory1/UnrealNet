// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterMovementComponent.h"
#include "ArenaBattle.h"
#include "GameFramework/Character.h"

UABCharacterMovementComponent::UABCharacterMovementComponent()
{
	bPressedTeleport = false;
	bDidTeleport = false;

	TeleportOffset = 600.0f;
	TeleportCooltime = 3.0f;
}

void UABCharacterMovementComponent::SetTeleportCommand()
{
	bPressedTeleport = true;
}

void UABCharacterMovementComponent::ABTeleport()
{
	if (CharacterOwner)
	{
		AB_SUBLOG(LogABTeleport, Log, TEXT("%s"), TEXT("Teleport Begin"));

		FVector TargetLocation = CharacterOwner->GetActorLocation() + CharacterOwner->GetActorForwardVector() * TeleportOffset;
		CharacterOwner->TeleportTo(TargetLocation, CharacterOwner->GetActorRotation(),false,true);
		bDidTeleport = true;

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([&]
			{
				bDidTeleport = false;
				AB_SUBLOG(LogABTeleport, Log, TEXT("%s"), TEXT("Teleport End"));
			}
			), TeleportCooltime, false, -1.0f );
	}
}

void UABCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	// 발동 조건 : 텔레포트가 눌렸는지
	if (bPressedTeleport && !bDidTeleport)
	{
		ABTeleport();
	}

	if (bPressedTeleport)
	{
		bPressedTeleport = false;
	}

}

void UABCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	// 디코딩
	Super::UpdateFromCompressedFlags(Flags);
	
	bPressedTeleport = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bDidTeleport = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;

	if (CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_Authority)
	{
		// 서버라면, 텔레포트가 실행 안됐다면 실행
		if (bPressedTeleport && !bDidTeleport)
		{
			AB_SUBLOG(LogABTeleport, Log, TEXT("%s"), TEXT("Teleport Begin in Server"));
			ABTeleport();
		}
	}
}

FNetworkPredictionData_Client* UABCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UABCharacterMovementComponent* MutableThis = const_cast<UABCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FABNetworkPredictionData_Client_Character(*this);
	}

	return ClientPredictionData;
}

FABNetworkPredictionData_Client_Character::FABNetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement)
	:Super(ClientMovement)
{

}

FSavedMovePtr FABNetworkPredictionData_Client_Character::AllocateNewMove()
{
	return FSavedMovePtr(new FABSavedMove_Character());
}

void FABSavedMove_Character::Clear()
{
	Super::Clear();

	bPressedTeleport = false;
	bDidTeleport = false;
}

void FABSavedMove_Character::SetInitialPosition(ACharacter* Character)
{
	Super::SetInitialPosition(Character);

	// 텔레포트 하기전에 지금 MovementComp에 설정된 속성 값을 캐릭터 움직임 데이터에 그대로 저장 할수 있게 되었음
	// 그 후, PerformMovement 함수를 통해 OnMovementUpdated 함수가 호출되는데 이러면 클라이언트에서 텔레포트는 진행이 되었음
	// 그러면 서버에서 ServerMoveRPC 를 통해 이 정보를 서버에 보내줘야되는데, 이 두가지 정보 ( bPressedTeleport, bDidTeleport )를 bool 변수로 저장하는것이 아님
	// GetCompressedFlags 에 넣어줘서 압축해줘야함
	UABCharacterMovementComponent* ABMovementComp = Cast<UABCharacterMovementComponent>(Character->GetCharacterMovement());
	if (ABMovementComp)
	{
		bPressedTeleport = ABMovementComp->bPressedTeleport;
		bDidTeleport = ABMovementComp->bDidTeleport;
	}
}

uint8 FABSavedMove_Character::GetCompressedFlags() const
{
	// 인코딩
	uint8 Result = Super::GetCompressedFlags();

	if (bPressedTeleport)
	{
		// 0 번 플래그를 사용함
		Result |= FLAG_Custom_0;
	}

	if (bDidTeleport)
	{
		// 0 번 플래그를 사용함
		Result |= FLAG_Custom_1;
	}
	// 이러면 클라이언트의 정보를 서버에 보냈음
	// 서버에서 받았으면 분석해줘야하는데 이는 함수 UpdateFromCompressedFlags 를 오버라이드해서 구현
	return Result;
}
