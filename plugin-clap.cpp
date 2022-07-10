
#include "plugin-clap.h"

// Bunch of boring sanity checking.. :)
#ifdef _WIN32
const char * clap_gui_platform_api = CLAP_WINDOW_API_WIN32;
#endif

#ifdef __APPLE__
const char * clap_gui_platform_api = CLAP_WINDOW_API_COCOA;
#endif

using namespace dust;

bool ClapBaseGUI::plug_gui_is_api_supported(const char *api, bool is_floating)
{
    // we'll just refuse floating windows for now
    if(is_floating) return false;
    
    if(!strcmp(api, clap_gui_platform_api)) return true;
    
    return false;
}

bool ClapBaseGUI::plug_gui_get_preferred_api(const char **api, bool *is_floating)
{
    *api = clap_gui_platform_api;
    *is_floating = false;
    return true;
}

bool ClapBaseGUI::plug_gui_create(const char *api, bool is_floating)
{
        DUST_TRACE
    if(strcmp(api, clap_gui_platform_api)) return false;
            DUST_TRACE

    _gui_data.hostGUI = (const clap_host_gui*)
        _gui_data.host->get_extension(_gui_data.host, CLAP_EXT_PARAMS);
            DUST_TRACE
    
    plugin_editor.computeSize(_gui_data.sizeX, _gui_data.sizeY);
            DUST_TRACE
    return true;
}
