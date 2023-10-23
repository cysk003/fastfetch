#pragma once

#include "fastfetch.h"

#define FF_USERS_MODULE_NAME "Users"

void ffPrintUsers(FFUsersOptions* options);
void ffInitUsersOptions(FFUsersOptions* options);
bool ffParseUsersCommandOptions(FFUsersOptions* options, const char* key, const char* value);
void ffDestroyUsersOptions(FFUsersOptions* options);
void ffParseUsersJsonObject(FFUsersOptions* options, yyjson_val* module);
void ffGenerateUsersJsonResult(FFUsersOptions* options, yyjson_mut_doc* doc, yyjson_mut_val* module);
void ffPrintUsersHelpFormat(void);
