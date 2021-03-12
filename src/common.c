#include "fastfetch.h"

#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

void ffInitState(FFstate* state)
{
    state->current_row = 0;
    state->passwd = getpwuid(getuid());
    uname(&state->utsname);
    sysinfo(&state->sysinfo);
    state->terminal.value = NULL;
    state->terminal.error = NULL;
}

void ffDefaultConfig(FFconfig* config)
{
    config->color[0] = '\0';
    config->logo_spacer = 4;
    strcpy(config->seperator, ": ");
    config->offsetx = 0;
    config->titleLength = 20; // This is overwritten by ffPrintTitle
    config->colorLogo = true;
    config->showErrors = false;
    config->recache = false;
    config->cacheSave = true;

    config->osShowArchitecture = true;
    config->osFormat[0] = '\0';

    config->hostShowVersion = true;

    config->kernelShowRelease = true;
    config->kernelShowVersion = false;

    config->packagesCombined = false;
    config->packagesCombinedNames = true;
    config->packagesShowPacman = true;
    config->packagesShowFlatpak = true;
    config->packagesFormat[0] = '\0';

    config->shellShowPath = false;

    config->resolutionShowRefreshRate = true;
    config->resolutionLibX11[0] = '\0';
    config->resolutionLibXrandr[0] = '\0';
    config->resolutionFormat[0] = '\0';

    config->batteryShowManufacturer = true;
    config->batteryShowModel = true;
    config->batteryShowTechnology = true;
    config->batteryShowCapacity = true;
    config->batteryShowStatus = true;
    config->batteryFormat[0] = '\0';
}

void ffPrintKey(FFconfig* config, const char* key)
{
    printf(FASTFETCH_TEXT_MODIFIER_BOLT"%s%s"FASTFETCH_TEXT_MODIFIER_RESET"%s", config->color, key, config->seperator);
}

void ffPrintLogoAndKey(FFinstance* instance, const char* key)
{
    ffPrintLogoLine(instance);
    ffPrintKey(&instance->config, key);
}

void ffParsePropFile(const char* fileName, const char* regex, char* buffer)
{
    buffer[0] = '\0'; //If an error occures, this is the indicator

    char* line = NULL;
    size_t len;

    FILE* file = fopen(fileName, "r");
    if(file == NULL)
        return; // handle errors in higher functions

    while (getline(&line, &len, file) != -1)
    {
        if (sscanf(line, regex, buffer) > 0)
            break;
    }

    fclose(file);
    free(line);
}

void ffParsePropFileHome(FFinstance* instance, const char* relativeFile, const char* regex, char* buffer)
{
    char absolutePath[512];
    strcpy(absolutePath, instance->state.passwd->pw_dir);
    strcat(absolutePath, "/");
    strcat(absolutePath, relativeFile);

    ffParsePropFile(absolutePath, regex, buffer);
}

static inline bool strSet(const char* str)
{
    return str != NULL && str[0] != '\0';
}

void ffPrintGtkPretty(const char* gtk2, const char* gtk3, const char* gtk4)
{
    if(strSet(gtk2) && strSet(gtk3) && strSet(gtk4))
    {
        if((strcmp(gtk2, gtk3) == 0) && (strcmp(gtk2, gtk4) == 0))
            printf("%s [GTK2/3/4]", gtk2);
        else if(strcmp(gtk2, gtk3) == 0)
            printf("%s [GTK2/3], %s [GTK4]", gtk2, gtk4);
        else if(strcmp(gtk3, gtk4) == 0)
            printf("%s [GTK2], %s [GTK3/4]", gtk2, gtk3);
        else
            printf("%s [GTK2], %s [GTK3], %s [GTK4]", gtk2, gtk3, gtk4);
    }
    else if(strSet(gtk2) && strSet(gtk3))
    {
        if(strcmp(gtk2, gtk3) == 0)
            printf("%s [GTK2/3]", gtk2);
        else
            printf("%s [GTK2], %s [GTK3]", gtk2, gtk3);
    }
    else if(strSet(gtk3) && strSet(gtk4))
    {
        if(strcmp(gtk3, gtk4) == 0)
            printf("%s [GTK3/4]", gtk3);
        else
            printf("%s [GTK3], %s [GTK4]", gtk3, gtk4);
    }
    else if(strSet(gtk2))
    {
        printf("%s [GTK2]", gtk2);
    }
    else if(strSet(gtk3))
    {
        printf("%s [GTK3]", gtk3);
    }
    else if(strSet(gtk4))
    {
        printf("%s [GTK4]", gtk4);
    }
}

void ffParseFont(char* font, char* buffer)
{
    char name[64];
    char size[32];
    int scanned = sscanf(font, "%[^,], %[^,]", name, size);
    
    if(scanned == 0)
        strcpy(buffer, font);
    else if(scanned == 1)
        strcpy(buffer, name);
    else
        sprintf(buffer, "%s (%spt)", name, size);
}

void ffPrintError(FFinstance* instance, const char* key, const char* message)
{
    if(!instance->config.showErrors)
        return;

    ffPrintLogoAndKey(instance, key);
    printf(FASTFETCH_TEXT_MODIFIER_ERROR"%s"FASTFETCH_TEXT_MODIFIER_RESET"\n", message);
}

void ffTrimTrailingWhitespace(char* buffer)
{
    uint32_t end = 0;

    for(uint32_t i = 0; buffer[i] != '\0'; i++)
    {
        if(buffer[i] != ' ')
            end = i;
    }

    if(buffer[end + 1] == ' ')
        buffer[end + 1] = '\0';
}

void ffGetFileContent(const char* fileName, char* buffer, uint32_t bufferSize)
{
    buffer[0] = '\0';

    int fd = open(fileName, O_RDONLY);
    if(fd == -1)
        return;

    ssize_t readed = read(fd, buffer, bufferSize - 1);
    if(readed < 1)
        return;

    close(fd);

    if(buffer[readed - 1] == '\n')
        buffer[readed - 1] = '\0';
    else
        buffer[readed] = '\0';

    ffTrimTrailingWhitespace(buffer);
}

static const char* getCacheDir(FFinstance* instance)
{
    static char cache[256];
    static bool init = false;
    if(init)
        return cache;

    const char* xdgCache = getenv("XDG_CACHE_HOME");
    if(xdgCache == NULL)
    {
        strcpy(cache, instance->state.passwd->pw_dir);
        strcat(cache, "/.cache");
    }
    else
    {
        strcpy(cache, xdgCache);
    }

    mkdir(cache, S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH); //I hope everybody has a cache folder but whow knews
    
    strcat(cache, "/fastfetch/");
    mkdir(cache, S_IRWXU | S_IRGRP | S_IROTH);

    init = true;
    return cache;
}

bool ffPrintCachedValue(FFinstance* instance, const char* key)
{
    if(instance->config.recache)
        return false;

    char fileName[256];
    strcpy(fileName, getCacheDir(instance));
    strcat(fileName, key);

    char value[1024];
    ffGetFileContent(fileName, value, sizeof(value));

    if(value[0] == '\0')
        return false;

    ffPrintLogoAndKey(instance, key);
    puts(value);

    return true;
}

void ffPrintAndSaveCachedValue(FFinstance* instance, const char* key, const char* value)
{
    ffPrintLogoAndKey(instance, key);
    puts(value);

    if(!instance->config.cacheSave)
        return;

    char fileName[256];
    strcpy(fileName, getCacheDir(instance));
    strcat(fileName, key);

    int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd == -1)
        return;

    size_t len = strlen(value);

    bool failed = write(fd, value, len) != len;

    close(fd);

    if(failed)
        unlink(fileName);
}