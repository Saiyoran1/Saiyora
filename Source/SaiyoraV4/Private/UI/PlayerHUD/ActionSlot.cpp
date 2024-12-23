#include "PlayerHUD/ActionSlot.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "Image.h"
#include "SaiyoraPlayerCharacter.h"
#include "SaiyoraUIDataAsset.h"
#include "TextBlock.h"
#include "UIFunctionLibrary.h"
#include "GameFramework/InputSettings.h"

void UActionSlot::InitActionSlot(UAbilityComponent* AbilityComponent, const ESaiyoraPlane Plane, const int32 SlotIdx)
{
	if (!IsValid(AbilityIconMaterial))
	{
		return;
	}
	if (!IsValid(AbilityComponent)
		|| (Plane != ESaiyoraPlane::Ancient && Plane != ESaiyoraPlane::Modern))
	{
		return;
	}
	OwnerAbilityComp = AbilityComponent;
	OwnerCharacter = Cast<ASaiyoraPlayerCharacter>(AbilityComponent->GetOwner());
	if (!IsValid(OwnerCharacter)
		|| (SlotIdx < 0 || SlotIdx > OwnerCharacter->MaxAbilityBinds - 1))
	{
		return;
	}
	AssignedPlane = Plane;
	AssignedIdx = SlotIdx;
	if (IsValid(AbilityIcon))
	{
		ImageInstance = UMaterialInstanceDynamic::Create(AbilityIconMaterial, this);
		if (IsValid(ImageInstance))
		{
			AbilityIcon->SetBrushFromMaterial(ImageInstance);
		}
	}
	OwnerAbilityComp->OnAbilityAdded.AddDynamic(this, &UActionSlot::OnAbilityAdded);
	OwnerAbilityComp->OnAbilityRemoved.AddDynamic(this, &UActionSlot::OnAbilityRemoved);
	OwnerCharacter->OnMappingChanged.AddDynamic(this, &UActionSlot::OnMappingChanged);
	OnMappingChanged(AssignedPlane, AssignedIdx, OwnerCharacter->GetAbilityMapping(AssignedPlane, AssignedIdx));
	TArray<FInputActionKeyMapping> Mappings;
	GetDefault<UInputSettings>()->GetActionMappingByName(FName(FString::Printf(L"Action%i", AssignedIdx)), Mappings);
	if (Mappings.Num() > 0)
	{
		UpdateKeybind(Mappings[0]);
	}
	else
	{
		UpdateKeybind(FInputActionKeyMapping());
	}

	bInitialized = true;
}

void UActionSlot::UpdateKeybind(const FInputActionKeyMapping& Mapping)
{
	if (!Mapping.Key.IsValid())
	{
		if (IsValid(KeybindText))
		{
			KeybindText->SetText(FText());
		}
		return;
	}
	if (IsValid(KeybindText))
	{
		KeybindText->SetText(FText::FromString(UUIFunctionLibrary::GetInputChordString(FInputChord(Mapping.Key, Mapping.bShift, Mapping.bCtrl, Mapping.bAlt, Mapping.bCmd))));
	}
}

void UActionSlot::SetActive(const float UpdateAlpha)
{
	if (IsValid(KeybindText))
	{
		KeybindText->SetRenderOpacity(UpdateAlpha);
	}
	if (IsValid(CooldownText))
	{
		CooldownText->SetRenderOpacity(UpdateAlpha);
	}
}

void UActionSlot::OnMappingChanged(const ESaiyoraPlane Plane, const int32 Index, TSubclassOf<UCombatAbility> NewAbilityClass)
{
	if (Plane == AssignedPlane && Index == AssignedIdx)
	{
		SetAbilityClass(NewAbilityClass);
	}
}

void UActionSlot::SetAbilityClass(const TSubclassOf<UCombatAbility> NewAbilityClass)
{
	if (!IsValid(OwnerAbilityComp) || (bInitialized && AbilityClass == NewAbilityClass))
	{
		return;
	}
	AbilityClass = NewAbilityClass;
	
	//Update the icon to reflect the ability instance or the invalid ability icon.
	if (IsValid(ImageInstance))
	{
		const USaiyoraUIDataAsset* UIDataAsset = UUIFunctionLibrary::GetUIDataAsset(GetWorld());
		if (IsValid(AbilityClass))
		{
			//Use the class default object to get the icon and school. The actual ability instance may not be valid yet.
			const UCombatAbility* ClassDefault = AbilityClass->GetDefaultObject<UCombatAbility>();
			if (IsValid(ClassDefault))
			{
				ImageInstance->SetTextureParameterValue(FName("IconTexture"), ClassDefault->GetAbilityIcon());
				if (IsValid(UIDataAsset))
				{
					ImageInstance->SetVectorParameterValue(FName("IconColor"), UIDataAsset->GetSchoolColor(ClassDefault->GetAbilitySchool()));
				}
			}
		}
		//If we have an invalid ability class, this slot is empty, so we'll use the invalid ability icon and tint.
		else
		{
			if (IsValid(UIDataAsset))
			{
				ImageInstance->SetTextureParameterValue(FName("IconTexture"), UIDataAsset->InvalidAbilityTexture);
				ImageInstance->SetVectorParameterValue(FName("IconColor"), UIDataAsset->InvalidAbilityTint);
				ImageInstance->SetScalarParameterValue(FName("CooldownPercent"), 1.0f);
				ImageInstance->SetScalarParameterValue(FName("SwipeAlpha"), 0.0f);
				ImageInstance->SetScalarParameterValue(FName("SwipePercent"), 1.0f);
				if (IsValid(CooldownText) && CooldownText->GetVisibility() != ESlateVisibility::Collapsed)
				{
					CooldownText->SetVisibility(ESlateVisibility::Collapsed);
				}
				if (IsValid(ChargesText) && ChargesText->GetVisibility() != ESlateVisibility::Collapsed)
				{
					ChargesText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}

	//Check if there's an instance of this new ability class. If not, we'll just get a callback from OnAbilityAdded and set it up then.
	UpdateAbilityInstance(OwnerAbilityComp->FindActiveAbility(AbilityClass));
}

void UActionSlot::OnAbilityAdded(UCombatAbility* NewAbility)
{
	if (IsValid(NewAbility) && NewAbility->GetClass() == AbilityClass)
	{
		UpdateAbilityInstance(NewAbility);
	}
}

void UActionSlot::OnAbilityRemoved(UCombatAbility* RemovedAbility)
{
	if (IsValid(RemovedAbility) && RemovedAbility == AssociatedAbility)
	{
		UpdateAbilityInstance(nullptr);
	}
}

void UActionSlot::UpdateAbilityInstance(UCombatAbility* NewAbility)
{
	if (NewAbility == AssociatedAbility && !bInitialized)
	{
		return;
	}
	if (IsValid(AssociatedAbility))
	{
		if (IsValid(AssociatedAbility))
		{
			AssociatedAbility->OnChargesChanged.RemoveDynamic(this, &UActionSlot::OnChargesChanged);
			AssociatedAbility->OnCastableChanged.RemoveDynamic(this, &UActionSlot::OnCastableChanged);
		}
	}
	AssociatedAbility = NewAbility;
	if (IsValid(AssociatedAbility))
	{
		AssociatedAbility->OnChargesChanged.AddDynamic(this, &UActionSlot::OnChargesChanged);
		OnChargesChanged(AssociatedAbility, 0, AssociatedAbility->GetCurrentCharges());
		AssociatedAbility->OnCastableChanged.AddDynamic(this, &UActionSlot::OnCastableChanged);
		TArray<ECastFailReason> FailReasons;
		OnCastableChanged(AssociatedAbility, AssociatedAbility->IsCastable(FailReasons), FailReasons);
	}
	else
	{
		OnChargesChanged(nullptr, 0, 0);
		OnCastableChanged(nullptr, false, TArray<ECastFailReason>());
		if (IsValid(CooldownText) && CooldownText->GetVisibility() != ESlateVisibility::Collapsed)
		{
			CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		}
		ImageInstance->SetScalarParameterValue(FName("CooldownPercent"), 1.0f);
	}
}

void UActionSlot::OnChargesChanged(UCombatAbility* Ability, const int32 PreviousCharges, const int32 NewCharges)
{
	if (IsValid(ChargesText))
	{
		if (NewCharges > 1)
		{
			if (ChargesText->GetVisibility() != ESlateVisibility::HitTestInvisible)
			{
				ChargesText->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			ChargesText->SetText(FText::FromString(FString::FromInt(NewCharges)));
		}
		else if (ChargesText->GetVisibility() != ESlateVisibility::Collapsed)
		{
			ChargesText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UActionSlot::OnCastableChanged(UCombatAbility* Ability, const bool bCastable, const TArray<ECastFailReason>& FailReasons)
{
	//Change keybind text color only if there is a restriction preventing ability use, like crowd control, resource costs, or the actual ability restrictions.
	bool bDisplayCastable = bCastable;
	if (!bCastable)
	{
		bool bFoundValidFailReason = false;
		for (const ECastFailReason FailReason : FailReasons)
		{
			if (FailReason == ECastFailReason::CustomRestriction
				|| FailReason == ECastFailReason::CrowdControl
				|| FailReason == ECastFailReason::AbilityConditionsNotMet
				|| FailReason == ECastFailReason::CostsNotMet)
			{
				bFoundValidFailReason = true;
				break;
			}
		}
		if (!bFoundValidFailReason)
		{
			bDisplayCastable = true;
		}
	}
	if (IsValid(KeybindText))
	{
		KeybindText->SetColorAndOpacity(bDisplayCastable ? FLinearColor::White : FLinearColor::Red);
	}
}

void UActionSlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(OwnerAbilityComp) || !IsValid(AssociatedAbility))
	{
		return;
	}
	
	UpdateCooldown();
	UpdateProc(InDeltaTime);
}

void UActionSlot::UpdateCooldown()
{
	const bool bChargeCostMet = AssociatedAbility->GetCurrentCharges() >= AssociatedAbility->GetChargeCost();
	if (IsValid(CooldownText))
	{
		//Display cooldown text if the cooldown is active, acked, and we don't have enough charges to cast.
		if (AssociatedAbility->IsCooldownActive() && AssociatedAbility->IsCooldownAcked()
			&& AssociatedAbility->GetRemainingCooldown() > 0.0f && !bChargeCostMet)
		{
			if (CooldownText->GetVisibility() != ESlateVisibility::HitTestInvisible)
			{
				CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			FNumberFormattingOptions FormatOptions;
			FormatOptions.MaximumFractionalDigits = 1;
			FormatOptions.MinimumFractionalDigits = 1;
			FormatOptions.RoundingMode = HalfFromZero;
			FormatOptions.AlwaysSign = false;
			FormatOptions.UseGrouping = false;
			CooldownText->SetText(FText::AsNumber(AssociatedAbility->GetRemainingCooldown(), &FormatOptions));
		}
		else if (CooldownText->GetVisibility() != ESlateVisibility::Collapsed)
		{
			CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	//Get the cooldown alpha. Unacked is 0, not on cooldown is 1.
	const float CooldownPercent = AssociatedAbility->IsCooldownActive()
		? AssociatedAbility->IsCooldownAcked() ? FMath::Clamp(1.0f - (AssociatedAbility->GetRemainingCooldown() / AssociatedAbility->GetCurrentCooldownLength()), 0.0f, 1.0f) : 0.0f
		: 1.0f;
	const float CooldownEndTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + AssociatedAbility->GetRemainingCooldown();
	const bool bOnGlobalCooldown = AssociatedAbility->HasGlobalCooldown() && OwnerAbilityComp->IsGlobalCooldownActive();
	const float GlobalPercent = bOnGlobalCooldown
		? FMath::Clamp(1.0f - (OwnerAbilityComp->GetGlobalCooldownTimeRemaining() / OwnerAbilityComp->GetCurrentGlobalCooldownLength()), 0.0f, 1.0f)
		: 1.0f;
	const float GlobalEndTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + OwnerAbilityComp->GetGlobalCooldownTimeRemaining();
	//Essentially we display whichever cooldown will end later, if either of them should even display.
	const float DisplayPercent = bOnGlobalCooldown && !bChargeCostMet
		? GlobalEndTime > CooldownEndTime ? GlobalPercent : CooldownPercent
		: bOnGlobalCooldown ? GlobalPercent
		: !bChargeCostMet ? CooldownPercent : 1.0f;
	
	//Set the cooldown fill percent to reflect the fill value of whichever ends later between the cooldown and GCD.
	ImageInstance->SetScalarParameterValue(FName("CooldownFillPercent"), DisplayPercent);
	
	//Use the swipe if we are on cooldown but have enough charges to still cast the ability.
	ImageInstance->SetScalarParameterValue(FName("SwipeAlpha"),
		AssociatedAbility->IsCooldownActive() && AssociatedAbility->IsCooldownAcked() && bChargeCostMet
		? 1.0f : 0.0f);
	//Set the swipe percent to match the actual cooldown, not the GCD.
	ImageInstance->SetScalarParameterValue(FName("SwipePercent"), CooldownPercent);
}

void UActionSlot::ApplyProc(UBuff* SourceBuff)
{
	if (IsValid(SourceBuff))
	{
		SourceBuff->OnRemoved.AddDynamic(this, &UActionSlot::RemoveProcFromBuff);
		ProcBuffs.AddUnique(SourceBuff);
	}
	if (!bProcActive && ProcBuffs.Num() > 0)
	{
		bProcActive = true;
		ProcStartAlpha = 0.0f;
	}
}

void UActionSlot::UpdateProc(const float DeltaTime)
{
	if (!bProcActive)
	{
		//TODO: Fade out proc.
		ImageInstance->SetScalarParameterValue(FName("ProcAlpha"), 0.0f);
		return;
	}
	ProcStartAlpha += FMath::Clamp(DeltaTime / ProcAnimationLength, 0.0f, 1.0f);
	ImageInstance->SetScalarParameterValue(FName("ProcAlpha"), ProcStartAlpha);
}

void UActionSlot::RemoveProcFromBuff(const FBuffRemoveEvent& Event)
{
	ProcBuffs.Remove(Event.RemovedBuff);
	if (bProcActive && ProcBuffs.Num() <= 0)
	{
		bProcActive = false;
	}
}