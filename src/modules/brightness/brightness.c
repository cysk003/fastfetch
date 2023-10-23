#include "common/bar.h"
#include "common/printing.h"
#include "common/jsonconfig.h"
#include "detection/brightness/brightness.h"
#include "modules/brightness/brightness.h"
#include "util/stringUtils.h"

#define FF_BRIGHTNESS_NUM_FORMAT_ARGS 5

void ffPrintBrightness(FFBrightnessOptions* options)
{
    FF_LIST_AUTO_DESTROY result = ffListCreate(sizeof(FFBrightnessResult));

    const char* error = ffDetectBrightness(options, &result);

    if(error)
    {
        ffPrintError(FF_BRIGHTNESS_MODULE_NAME, 0, &options->moduleArgs, "%s", error);
        return;
    }

    if(result.length == 0)
    {
        ffPrintError(FF_BRIGHTNESS_MODULE_NAME, 0, &options->moduleArgs, "No result is detected.");
        return;
    }

    FF_STRBUF_AUTO_DESTROY key = ffStrbufCreate();

    uint32_t index = 0;
    FF_LIST_FOR_EACH(FFBrightnessResult, item, result)
    {
        if(options->moduleArgs.key.length == 0)
        {
            ffStrbufAppendF(&key, "%s (%s)", FF_BRIGHTNESS_MODULE_NAME, item->name.chars);
        }
        else
        {
            uint32_t moduleIndex = result.length == 1 ? 0 : index + 1;
            ffParseFormatString(&key, &options->moduleArgs.key, 2, (FFformatarg[]){
                {FF_FORMAT_ARG_TYPE_UINT, &moduleIndex},
                {FF_FORMAT_ARG_TYPE_STRBUF, &item->name}
            });
        }

        const double percent = (item->current - item->min) / (item->max - item->min) * 100;

        if(options->moduleArgs.outputFormat.length == 0)
        {
            FF_STRBUF_AUTO_DESTROY str = ffStrbufCreate();
            ffPrintLogoAndKey(key.chars, 0, &options->moduleArgs, FF_PRINT_TYPE_NO_CUSTOM_KEY);

            if (instance.config.percentType & FF_PERCENTAGE_TYPE_BAR_BIT)
            {
                ffAppendPercentBar(&str, percent, 0, 100, 100);
            }

            if(instance.config.percentType & FF_PERCENTAGE_TYPE_NUM_BIT)
            {
                if(str.length > 0)
                    ffStrbufAppendC(&str, ' ');

                ffAppendPercentNum(&str, percent, 10, 10, str.length > 0);
            }

            ffStrbufPutTo(&str, stdout);
        }
        else
        {
            FF_STRBUF_AUTO_DESTROY valueStr = ffStrbufCreate();
            ffAppendPercentNum(&valueStr, percent, 10, 10, false);
            ffPrintFormatString(key.chars, 0, &options->moduleArgs, FF_PRINT_TYPE_NO_CUSTOM_KEY, FF_BRIGHTNESS_NUM_FORMAT_ARGS, (FFformatarg[]) {
                {FF_FORMAT_ARG_TYPE_STRBUF, &valueStr},
                {FF_FORMAT_ARG_TYPE_STRBUF, &item->name},
                {FF_FORMAT_ARG_TYPE_DOUBLE, &item->max},
                {FF_FORMAT_ARG_TYPE_DOUBLE, &item->min},
                {FF_FORMAT_ARG_TYPE_DOUBLE, &item->current},
            });
        }

        ffStrbufClear(&key);
        ffStrbufDestroy(&item->name);
        ++index;
    }
}

void ffInitBrightnessOptions(FFBrightnessOptions* options)
{
    ffOptionInitModuleBaseInfo(&options->moduleInfo, FF_BRIGHTNESS_MODULE_NAME, ffParseBrightnessCommandOptions, ffParseBrightnessJsonObject, ffPrintBrightness, ffGenerateBrightnessJsonResult, ffPrintBrightnessHelpFormat);
    ffOptionInitModuleArg(&options->moduleArgs);

    options->ddcciSleep = 10;
}

bool ffParseBrightnessCommandOptions(FFBrightnessOptions* options, const char* key, const char* value)
{
    const char* subKey = ffOptionTestPrefix(key, FF_BRIGHTNESS_MODULE_NAME);
    if (!subKey) return false;
    if (ffOptionParseModuleArgs(key, subKey, value, &options->moduleArgs))
        return true;

    if (ffStrEqualsIgnCase(key, "ddcci-sleep"))
    {
        options->ddcciSleep = ffOptionParseUInt32(key, value);
        return true;
    }

    return false;
}

void ffDestroyBrightnessOptions(FFBrightnessOptions* options)
{
    ffOptionDestroyModuleArg(&options->moduleArgs);
}

void ffParseBrightnessJsonObject(FFBrightnessOptions* options, yyjson_val* module)
{
    yyjson_val *key_, *val;
    size_t idx, max;
    yyjson_obj_foreach(module, idx, max, key_, val)
    {
        const char* key = yyjson_get_str(key_);
        if(ffStrEqualsIgnCase(key, "type"))
            continue;

        if (ffJsonConfigParseModuleArgs(key, val, &options->moduleArgs))
            continue;

        if (ffStrEqualsIgnCase(key, "ddcciSleep"))
        {
            options->ddcciSleep = (uint32_t) yyjson_get_uint(val);
            continue;
        }

        ffPrintError(FF_BRIGHTNESS_MODULE_NAME, 0, &options->moduleArgs, "Unknown JSON key %s", key);
    }
}

void ffGenerateBrightnessJsonResult(FF_MAYBE_UNUSED FFBrightnessOptions* options, yyjson_mut_doc* doc, yyjson_mut_val* module)
{
    FF_LIST_AUTO_DESTROY result = ffListCreate(sizeof(FFBrightnessResult));

    const char* error = ffDetectBrightness(options, &result);

    if (error)
    {
        yyjson_mut_obj_add_str(doc, module, "error", error);
        return;
    }

    if(result.length == 0)
    {
        yyjson_mut_obj_add_str(doc, module, "error", "No result is detected.");
        return;
    }

    yyjson_mut_val* arr = yyjson_mut_arr(doc);
    yyjson_mut_obj_add_val(doc, module, "result", arr);

    FF_LIST_FOR_EACH(FFBrightnessResult, item, result)
    {
        yyjson_mut_val* obj = yyjson_mut_arr_add_obj(doc, arr);
        yyjson_mut_obj_add_strbuf(doc, obj, "name", &item->name);
        yyjson_mut_obj_add_real(doc, obj, "max", item->max);
        yyjson_mut_obj_add_real(doc, obj, "min", item->min);
        yyjson_mut_obj_add_real(doc, obj, "current", item->current);
    }

    FF_LIST_FOR_EACH(FFBrightnessResult, item, result)
    {
        ffStrbufDestroy(&item->name);
    }
}

void ffPrintBrightnessHelpFormat(void)
{
    ffPrintModuleFormatHelp(FF_BRIGHTNESS_MODULE_NAME, "{1}", FF_BRIGHTNESS_NUM_FORMAT_ARGS, (const char* []) {
        "Screen brightness (percentage)",
        "Screen name",
        "Maximum brightness value",
        "Minimum brightness value",
        "Current brightness value",
    });
}
