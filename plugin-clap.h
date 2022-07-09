
#pragma once

#include "clap-glue.h"
#include "dust/gui/window.h"

// This wrapper implements dust-toolkit specific functionality.
//
// For the toolkit-independent base-wrappers, see clap-glue.h
namespace dust
{

    // Some basic stuff that can often be left empty.
    //
    // Eventually this should probably implement some basic parameter handling?
    //
    struct ClapBasePlugin
    {
        void plug_deactivate() {}
        bool plug_start_processing() { return true; }
        bool plug_stop_processing() { return true; }
        void plug_on_main_thread() {}
    };

    // Implementation of CLAP 'gui' extension for dust-toolkit.
    //
    // The idea is that all one needs to do is make this a base-class of the
    // plugin class and then use 'plugin_editor' as the parent for the GUI.
    //
    // The wrapper theoretically takes care of the rest..
    //
    struct ClapBaseGUI : WindowDelegate
    {
        Panel   plugin_editor;  // Top level editor Panel; use as a parent.

        ClapBaseGUI(const clap_host * host)
        {
            plugin_editor.style.rule = LayoutStyle::FILL;
            _gui_data.host = host;
        }
        
        bool plug_gui_is_api_supported(const char *api, bool is_floating);
        bool plug_gui_get_preferred_api(const char **api, bool *is_floating);
        bool plug_gui_create(const char *api, bool is_floating);

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

                if(!_gui_data.hostGUI) return;

                int szX = (_gui_data.sizeX * _gui_data.scale) / 100;
                int szY = (_gui_data.sizeY * _gui_data.scale) / 100;
            
                _gui_data.hostGUI->request_resize(_gui_data.host, szX, szY);
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
            
            const clap_host     *host       = 0;
            const clap_host_gui *hostGUI    = 0;
            
        } _gui_data;
    };

};