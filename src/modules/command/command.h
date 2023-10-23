#pragma once

#include "fastfetch.h"

#define FF_COMMAND_MODULE_NAME "Command"

void ffPrintCommand(FFCommandOptions* options);
void ffInitCommandOptions(FFCommandOptions* options);
bool ffParseCommandCommandOptions(FFCommandOptions* options, const char* key, const char* value);
void ffDestroyCommandOptions(FFCommandOptions* options);
void ffParseCommandJsonObject(FFCommandOptions* options, yyjson_val* module);
void ffGenerateCommandJsonResult(FFCommandOptions* options, yyjson_mut_doc* doc, yyjson_mut_val* module);
void ffPrintCommandHelpFormat(void);
