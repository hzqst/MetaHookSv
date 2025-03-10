#pragma once

#include <com_model.h>

void CounterStrike_RedirectPlayerModelPath(const char* name, int PlayerID, int TeamID, char* pszModel, size_t cbModel);
model_t* CounterStrike_RedirectPlayerModel(model_t* original_model, int PlayerNumber, int* modelindex);