// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ABCharacterMovementComponent.generated.h"


class FABSavedMove_Character : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;
public:
	virtual void Clear() override;
	virtual void SetInitialPosition(ACharacter* Character) override; 
	virtual uint8 GetCompressedFlags() const override;

	uint8 bPressedTeleport : 1;
	uint8 bDidTeleport : 1;
};

class FABNetworkPredictionData_Client_Character : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;
public:
	// 생성자
	FABNetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement);

	//중요한 함수
	virtual FSavedMovePtr AllocateNewMove() override;
};

/**
 * 
 */
UCLASS()
class ARENABATTLE_API UABCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	UABCharacterMovementComponent();
	
	void SetTeleportCommand();

protected:
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	// 후속 클래스를 위해 가상함수로 선언
	virtual void ABTeleport();
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
public:
	// 네트워크로 전송할 수 있으니 uint8형으로 Unsigned Int
	// 클라이언트의 입력이 들어오면 변수의 값을 true로
	uint8 bPressedTeleport : 1;
	// 텔레포트의 쿨타임이 중에는 true로 다시 텔레포트 가능할땐 false
	uint8 bDidTeleport : 1;

protected:
	// 텔레포트 할 길이
	UPROPERTY()
	float TeleportOffset;

	// 쿨타임
	UPROPERTY()
	float TeleportCooltime;
};
