#pragma once
#include <3ds.h>

Result plgLdrInit(void);
void plgLdrExit(void);
Result PLGLDR__IsPluginLoaderEnabled(bool *isEnabled);
Result PLGLDR__DisplayMessage(const char *title, const char *body);
