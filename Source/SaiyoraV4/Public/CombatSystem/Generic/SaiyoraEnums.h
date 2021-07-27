// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
UENUM(BlueprintType)
enum class ESaiyoraPlane : uint8
{
 None = 0,
 Ancient = 1,
 Modern = 2,
 Both = 3,
 Neither = 4,
};

UENUM(BlueprintType)
enum class EModifierType : uint8
{
 Invalid = 0,
 Additive = 1,
 Multiplicative = 2,
};

UENUM()
enum class EReplicationNeeds : uint8
{
 NotReplicated = 0,
 OwnerOnly = 1,
 AllClients = 2,
};

UENUM(BlueprintType)
enum class ECombatParamType : uint8
{
 None,
 Custom,
 Origin,
 Target,
 ClassDefault,
};

UENUM()
enum class EActorNetPermission : uint8
{
 None = 0, //Simulated proxy. No authority over anything.
 PredictPlayer = 1, //Auto proxy. Can do client prediction.
 ListenServer = 2, //Authority, remote auto proxy, locally controlled. Acts as a server, but can call owning client code as well.
 ServerPlayer = 3, //Authority, remote auto proxy, not locally controlled. Server representation of a non-local player.
 Server = 4, //Authority, remote simulated proxy. Server representation of a non-player-controlled object.
};