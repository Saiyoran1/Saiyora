// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraResourceFunctions.h"

//TODO: Non-ability resource usage.
/*FResourceEvent USaiyoraResourceFunctions::ModifyResource(FGameplayTag const& ResourceTag, AActor* AppliedBy,
    AActor* AppliedTo, float const Value, UObject* Source, bool const bIgnoreModifiers)
{
    FResourceEvent Result;
    Result.Info.Amount = Value;
    Result.Info.ResourceTag = ResourceTag;
    Result.Info.SpendingSource = Source;
    Result.Info.CastID = 0;
    Result.Result.Success = false;
    Result.Result.NewValue = 0.0f;
    Result.Result.PreviousValue = 0.0f;
    
    if (!ResourceTag.IsValid() || !ResourceTag.MatchesTag(UResourceHandler::GenericResourceTag))
    {
        return Result;
    }
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source))
    {
        return Result;
    }
    UResourceHandler* Handler = GetResourceHandler(AppliedTo);
    if (!IsValid(Handler))
    {
        return Result;
    }
    Handler->SpendResource(Result, bIgnoreModifiers);
    return Result;
}*/
