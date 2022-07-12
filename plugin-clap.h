
#pragma once

#include "clap-glue.h"
#include "dust/gui/window.h"

// This wrapper implements dust-toolkit specific functionality.
//
// For the toolkit-independent base-wrappers, see clap-glue.h
namespace dust
{

    // Bunch of boring sanity checking.. :)
#ifdef _WIN32
    static const char * clap_gui_platform_api = CLAP_WINDOW_API_WIN32;
#endif
#ifdef __APPLE__
    static const char * clap_gui_platform_api = CLAP_WINDOW_API_COCOA;
#endif

    // Some basic stuff ...
    //
    // Eventually this should probably implement some basic parameter handling?
    //
    struct ClapBase : WindowDelegate
    {
        struct {
            const clap_host         *host;
            const clap_host_params  *host_params;
            const clap_host_gui     *host_gui;      // loaded by ClapBaseGUI
        } clap = {};

        struct {
            // Names of ports - first one is always main
            std::vector<const char*>    audioIn;
            std::vector<const char*>    audioOut;
            
        } properties;

        Panel   plugin_editor;  // Top level editor Panel; use as a parent.
        
        ClapBase(const clap_host * _host)
        {
            clap.host = _host;
            
            plugin_editor.style.rule = LayoutStyle::FILL;
        }

        bool plug_init()
        {
            clap.host_params = (const clap_host_params*)
                clap.host->get_extension(clap.host, CLAP_EXT_PARAMS);
                
            clap.host_gui = (const clap_host_gui*)
                clap.host->get_extension(clap.host, CLAP_EXT_GUI);
                
            return true;
        }
        void plug_deactivate() {}
        bool plug_start_processing() { return true; }
        bool plug_stop_processing() { return true; }
        void plug_on_main_thread() {}
        
        // FIXME: make this support any number of ports
        uint32_t plug_audio_ports_count(bool input)
        {
            return input ? properties.audioIn.size() : properties.audioOut.size();
        }
        bool plug_audio_ports_get(
            uint32_t index, bool input, clap_audio_port_info * info)
        {
            auto & ports = input ? properties.audioIn : properties.audioOut;

            if(index >= ports.size()) return false;
            
            info->id = index | (input ? 0 : 0x10000);
        
            strcpy(info->name, ports[index]);
            
            info->flags = CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE;
            if(index == 0) info->flags |= CLAP_AUDIO_PORT_IS_MAIN;
        
            info->channel_count = 2;
            info->port_type = CLAP_PORT_STEREO;
            info->in_place_pair = CLAP_INVALID_ID;
    
            return true;
        }

        // FIXME: ...
        uint32_t plug_note_ports_count(bool input) { return 0; }
        bool plug_note_ports_get(
            uint32_t index, bool input, clap_note_port_info * info)
        {
            return false;
        }

        // GUI
        
        bool plug_gui_is_api_supported(const char *api, bool is_floating)
        {
            if(is_floating) return false;   // refuse floating for now
            if(!strcmp(api, clap_gui_platform_api)) return true;
            return false;
        }
        
        bool plug_gui_get_preferred_api(const char **api, bool *is_floating)
        {
            *api = clap_gui_platform_api;
            *is_floating = false;
            return true;
        }

        bool plug_gui_create(const char *api, bool is_floating)
        {
            if(strcmp(api, clap_gui_platform_api)) return false;
            
            plugin_editor.computeSize(_gui_data.sizeX, _gui_data.sizeY);
            return true;
        }
        
        void plug_gui_destroy()
        {
            // should never happen, but just in case..
            auto * win = plugin_editor.getWindow();
            if(win) win->closeWindow();
        }

        // unsupported .. really should implement this..
        bool plug_gui_set_scale(double scale) { return false; }

        bool plug_gui_get_size(uint32_t *w, uint32_t *h)
        {
            *w = (_gui_data.sizeX * _gui_data.scale) / 100;
            *h = (_gui_data.sizeY * _gui_data.scale) / 100;
            return true;
        }

        // unsupported .. maybe some day
        bool plug_gui_can_resize() { return false; }
        bool plug_gui_get_resize_hints(clap_gui_resize_hints *) { return false; }
        bool plug_gui_adjust_size(uint32_t *w, uint32_t *h) { return false; }
        bool plug_gui_set_size(uint32_t w, uint32_t h) { return false; }

        // store parent for show() to create the actual OS window
        bool plug_gui_set_parent(const clap_window *win)
        { _gui_data.parent = win->ptr; return true; }

        // unsupported .. maybe some day
        bool plug_gui_set_transient(const clap_window *win) { return false; }
        void plug_gui_suggest_title(const char *title) { }

        bool plug_gui_show()
        {
            if(plugin_editor.getWindow()) return false;
    
            int szX = (_gui_data.sizeX * _gui_data.scale) / 100;
            int szY = (_gui_data.sizeY * _gui_data.scale) / 100;
        
            auto * win = createWindow(*this, _gui_data.parent, szX, szY);
            plugin_editor.setParent(win);
            
            win->setScale(_gui_data.scale);
            win->onScaleChange = [this, win]()
            {
                _gui_data.scale = win->getScale();

                if(!clap.host_gui) return;

                int szX = (_gui_data.sizeX * _gui_data.scale) / 100;
                int szY = (_gui_data.sizeY * _gui_data.scale) / 100;
            
                clap.host_gui->request_resize(clap.host, szX, szY);
            };

            return true;
        }
        
        bool plug_gui_hide()
        {
            auto * win = plugin_editor.getWindow();
            if(!win) return false;
            
            win->closeWindow();
            return true;
        }

    private:
        struct {
            // automatically computed on create
            uint32_t    sizeX   = 0;
            uint32_t    sizeY   = 0;
            uint32_t    scale   = 100;
            void                *parent     = 0;
        } _gui_data;
    };

};