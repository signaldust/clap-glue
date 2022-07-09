
#include "plugin-clap.h"

// Bunch of boring sanity checking.. :)
#ifdef _WIN32
# define WINDOW_API CLAP_WINDOW_API_WIN32
#endif

#ifdef __APPLE__
# define WINDOW_API CLAP_WINDOW_API_COCOA
#endif

using namespace dust;

bool ClapBaseGUI::plug_gui_is_api_supported(const char *api, bool is_floating)
{
    // we'll just refuse floating windows for now
    if(is_floating) return false;
    
#ifdef WINDOW_API
    if(!strcmp(api, WINDOW_API)) return true;
#endif
    return false;
}

bool ClapBaseGUI::plug_gui_get_preferred_api(const char **api, bool *is_floating)
{
#ifdef WINDOW_API
    *api = WINDOW_API;
    *is_floating = false;
    return true;
#else
    return false;
#endif
}

bool ClapBaseGUI::plug_gui_create(const char *api, bool is_floating)
{
    if(strcmp(api, WINDOW_API)) return false;

    _gui_data.hostGUI = (const clap_host_gui*)
        _gui_data.host->get_extension(_gui_data.host, CLAP_EXT_PARAMS);
    
    plugin_editor.computeSize(_gui_data.sizeX, _gui_data.sizeY);
    return true;
}

#if 0

// TEST

using namespace dust;

struct Test : ClapBasePlugin, ClapBaseGUI
{
    static clap_plugin_descriptor plug_desc;

    const clap_host * host;

    Test(const clap_host * host) : ClapBaseGUI(host), host(host) {}

    bool plug_init() { return true; }

    bool plug_activate(double, uint32_t, uint32_t) { return true; }

    void plug_reset() {}

    clap_process_status plug_process(const clap_process *)
    {
        return CLAP_PROCESS_CONTINUE;
    }

    void* plug_get_extension(const char *id)
    {
        void * ext = ClapExt_note_ports<Test>::check(id);
        if(!ext) ext = ClapExt_audio_ports<Test>::check(id);
        if(!ext) ext = ClapExt_latency<Test>::check(id);
        if(!ext) ext = ClapExt_gui<Test>::check(id);
        
        return ext;
    }

    uint32_t plug_audio_ports_count(bool input) { return 0; }

    bool plug_audio_ports_get(
        uint32_t index, bool input, clap_audio_port_info * info)
    {
        return false;
    }

    uint32_t plug_note_ports_count(bool input) { return 0; }

    bool plug_note_ports_get(
        uint32_t index, bool input, clap_note_port_info * info)
    {
        return false;
    }

    uint32_t plug_latency_get() { return 0; }
};

clap_plugin_descriptor Test::plug_desc =
{
    .id             = "org.example.test",
    .name           = "test",
    .vendor         = "",
    .url            = "",
    .manual_url     = "",
    .support_url    = "",
    .version        = "0",
    .description    = "test compilation",
    .features       = (const char *[]){
        CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
        0
    }
};

static ClapFactory<Test>  test_factory;

#endif