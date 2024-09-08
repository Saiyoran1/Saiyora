#include "PlayerHUD/ActionSlot.h"
#include "AbilityComponent.h"
#include "CombatAbility.h"
#include "Image.h"
#include "SaiyoraPlayerCharacter.h"
#include "SaiyoraUIDataAsset.h"
#include "TextBlock.h"
#include "UIFunctionLibrary.h"

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

	bInitialized = true;
}

void UActionSlot::OnMappingChanged(const ESaiyoraPlane Plane, const int32 Index, TSubclassOf<UCombatAbility> AbilityClass)
{
	if (Plane == AssignedPlane && Index == AssignedIdx)
	{
		SetAbilityClass(AbilityClass);
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
	if (NewAbility == AssociatedAbility)
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
	if (IsValid(KeybindText))
	{
		KeybindText->SetColorAndOpacity(bCastable ? FLinearColor::White : FLinearColor::Red);
	}
	if (IsValid(CooldownText))
	{
		CooldownText->SetColorAndOpacity(bCastable ? FLinearColor::White : FLinearColor::Red);
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
}

void UActionSlot::UpdateCooldown()
{
	if (IsValid(CooldownText))
	{
		if (AssociatedAbility->IsCooldownActive() && AssociatedAbility->IsCooldownAcked() && AssociatedAbility->GetRemainingCooldown() > 0.0f)
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

	if ((AssociatedAbility->IsCooldownActive() && AssociatedAbility->GetRemainingCooldown() > 0.0f)
		|| (AssociatedAbility->HasGlobalCooldown() && OwnerAbilityComp->IsGlobalCooldownActive()))
	{
		const float CooldownPercent = 1.0f - FMath::Clamp(AssociatedAbility->GetRemainingCooldown() / AssociatedAbility->GetCooldownLength(), 0.0f, 1.0f);
		const float GlobalPercent = AssociatedAbility->HasGlobalCooldown() && OwnerAbilityComp->IsGlobalCooldownActive() ?
			1.0f - FMath::Clamp(OwnerAbilityComp->GetGlobalCooldownTimeRemaining() / OwnerAbilityComp->GetCurrentGlobalCooldownLength(), 0.0f, 1.0f) : 1.0f;
		ImageInstance->SetScalarParameterValue(FName("CooldownPercent"), FMath::Min(CooldownPercent, GlobalPercent));
	}
	else
	{
		ImageInstance->SetScalarParameterValue(FName("CooldownPercent"), 1.0f);
	}

	if (AssociatedAbility->GetCurrentCharges() < AssociatedAbility->GetChargeCost())
	{
		ImageInstance->SetScalarParameterValue(FName("OnCooldownAlpha"), 0.0f);
	}
	else
	{
		ImageInstance->SetScalarParameterValue(FName("OnCooldownAlpha"), 1.0f);
	}
}