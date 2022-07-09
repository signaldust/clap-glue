
#pragma once

#include "clap/include/clap/clap.h"

#include <cstring>

// clap-glue.h / clap-glue.cpp
// ---------------------------
//
// "zero-cost" C++ wrappers around base CLAP API and selected extensions.
//
// The concept here is that the plugin becomes a member of a templated
// wrapper and the C-compatible wrapper functions should all either
// inline the actual implementation, or compile into trivial tail-calls
// as long as the plugin itself doesn't declare anything virtual.
//
// To implement a plugin, write a class that implements the API methods
// (preferably non-virtual) with names from the CLAP structures prefixed
// with plug_ or plug_some_ext_ and query extensions in plug_get_extension()
// by calling:
//
//   ext = ClapExt_Something<MyPluginType>::check(id);
//
// The reference to ClapExt_Something<MyPluginType> here triggers the
// generation of the extension table, so methods need only be implemented
// when the extension is actually supported.
//
// Finally declare a non-local factory wrapper for each plugin type:
//
//   static ClapFactory<MyPluginType> myplug_factory;
//
// This will automatically register the new plugin type as one of the plugins
// that can be instantiated by the CLAP entry point in plugin-clap-glue.cpp
// 
namespace dust
{
    template <typename Plugin>
    struct ClapWrapper : clap_plugin
    {
        Plugin  plugin;

        ClapWrapper(const clap_host * hostPtr) : plugin(hostPtr)
        {
            desc                = &plugin.plug_desc;
            plugin_data         = 0;
            init                = _init;
            destroy             = _destroy;
            activate            = _activate;
            deactivate          = _deactivate;
            start_processing    = _start_processing;
            stop_processing     = _stop_processing;
            reset               = _reset;
            process             = _process;
            get_extension       = _get_extension;
            on_main_thread      = _on_main_thread;
        }

        static ClapWrapper * _cast(const clap_plugin *self)
        { return static_cast<ClapWrapper*>(const_cast<clap_plugin*>(self)); }
        
    private:
        static bool _init(const clap_plugin *self)
        { return _cast(self)->plugin.plug_init(); }
        
        static void _destroy(const clap_plugin *self)
        { delete _cast(self); }
        
        static bool _activate(const clap_plugin *self,
            double sr, uint32_t minf, uint32_t maxf)
        { return _cast(self)->plugin.plug_activate(sr, minf, maxf); }
        
        static void _deactivate(const clap_plugin *self)
        { _cast(self)->plugin.plug_deactivate(); }

        static bool _start_processing(const clap_plugin *self)
        { return _cast(self)->plugin.plug_start_processing(); }

        static void _stop_processing(const clap_plugin *self)
        { _cast(self)->plugin.plug_stop_processing(); }

        static void _reset(const clap_plugin *self)
        { _cast(self)->plugin.plug_reset(); }

        static clap_process_status _process(
            const clap_plugin *self, const clap_process * proc)
        { return _cast(self)->plugin.plug_process(proc); }

        static const void* _get_extension(const clap_plugin *self, const char * id)
        { return _cast(self)->plugin.plug_get_extension(id); }

        static void _on_main_thread(const clap_plugin *self)
        { _cast(self)->plugin.plug_on_main_thread(); }
    };
    
    // Note ports
    template <typename Plugin>
    struct ClapExt_NotePorts
    {
        static void * check(const char * id)
        { return (!strcmp(id, CLAP_EXT_NOTE_PORTS)) ? (void*) &ext : 0; }

    private:
        static const clap_plugin_note_ports ext;
        
        static ClapWrapper<Plugin> * _cast(const clap_plugin *self)
        { return ClapWrapper<Plugin>::_cast(self); }
        
        static uint32_t _count(const clap_plugin *self, bool is_input)
        { return _cast(self)->plugin.plug_note_ports_count(is_input); }

        static bool _get(const clap_plugin *self,
            uint32_t index, bool is_input, clap_note_port_info *info)
        { return _cast(self)->plugin.plug_note_ports_get(index, is_input, info); }
    };
    
    template <typename Plugin>
    const clap_plugin_note_ports ClapExt_NotePorts<Plugin>::ext =
    {
        .count  = ClapExt_NotePorts<Plugin>::_count,
        .get    = ClapExt_NotePorts<Plugin>::_get,
    };

    // Audio ports
    template <typename Plugin>
    struct ClapExt_AudioPorts
    {
        static void * check(const char * id)
        { return (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) ? (void*) &ext : 0; }

    private:
        static const clap_plugin_audio_ports ext;
        
        static ClapWrapper<Plugin> * _cast(const clap_plugin *self)
        { return ClapWrapper<Plugin>::_cast(self); }
        
        static uint32_t _count(const clap_plugin *self, bool is_input)
        { return _cast(self)->plugin.plug_audio_ports_count(is_input); }

        static bool _get(const clap_plugin *self,
            uint32_t index, bool is_input, clap_audio_port_info *info)
        { return _cast(self)->plugin.plug_audio_ports_get(index, is_input, info); }
    };
    
    template <typename Plugin>
    const clap_plugin_audio_ports ClapExt_AudioPorts<Plugin>::ext =
    {
        .count  = ClapExt_AudioPorts<Plugin>::_count,
        .get    = ClapExt_AudioPorts<Plugin>::_get,
    };

    // Latency
    template <typename Plugin>
    struct ClapExt_Latency
    {
        static void * check(const char * id)
        { return (!strcmp(id, CLAP_EXT_LATENCY)) ? (void*) &ext : 0; }

    private:
        static const clap_plugin_latency ext;
        
        static ClapWrapper<Plugin> * _cast(const clap_plugin *self)
        { return ClapWrapper<Plugin>::_cast(self); }
        
        static uint32_t _get(const clap_plugin *self)
        { return _cast(self)->plugin.plug_latency_get(); }
    };
    
    template <typename Plugin>
    const clap_plugin_latency ClapExt_Latency<Plugin>::ext =
    {
        .get    = ClapExt_Latency<Plugin>::_get,
    };

    // Params
    template <typename Plugin>
    struct ClapExt_Params
    {
        static void * check(const char * id)
        { return (!strcmp(id, CLAP_EXT_LATENCY)) ? (void*) &ext : 0; }

    private:
        static const clap_plugin_params ext;
        
        static ClapWrapper<Plugin> * _cast(const clap_plugin *self)
        { return ClapWrapper<Plugin>::_cast(self); }
        
        static uint32_t _count(const clap_plugin *self)
        { return _cast(self)->plugin.plug_params_count(); }

        static bool _get_info(const clap_plugin *self,
            uint32_t index, clap_param_info *info)
        { return _cast(self)->plugin.plug_param_get_info(index, info); }

        static bool _get_value(const clap_plugin *self, clap_id id, double *value)
        { return _cast(self)->plugin.plug_param_get_value(id, value); }

        static bool _value_to_text(const clap_plugin *self,
            clap_id id, double val, char *txt, uint32_t sz)
        { return _cast(self)->plugin.plug_param_value_to_text(id, val, txt, sz); }
        
        static bool _text_to_value(const clap_plugin *self,
            clap_id id, const char *txt, double *val)
        { return _cast(self)->plugin.plug_param_text_to_value(id, txt, val); }

        static void _flush(const clap_plugin *self,
            const clap_input_events *in, const clap_output_events *out)
        { return _cast(self)->plugin.plug_param_flush(in, out); }
    };
    
    template <typename Plugin>
    const clap_plugin_params ClapExt_Params<Plugin>::ext =
    {
        .count          = ClapExt_Params<Plugin>::_count,
        .get_info       = ClapExt_Params<Plugin>::_get_info,
        .get_value      = ClapExt_Params<Plugin>::_get_value,
        .value_to_text  = ClapExt_Params<Plugin>::_value_to_text,
        .text_to_value  = ClapExt_Params<Plugin>::_text_to_value,
        .flush          = ClapExt_Params<Plugin>::_flush,
    };

    // GUI
    template <typename Plugin>
    struct ClapExt_GUI
    {
        static void * check(const char * id)
        { return (!strcmp(id, CLAP_EXT_GUI)) ? (void*) &ext : 0; }

    private:
        static const clap_plugin_gui ext;
        
        static ClapWrapper<Plugin> * _cast(const clap_plugin *self)
        { return ClapWrapper<Plugin>::_cast(self); }
        
        static bool _is_api_supported(const clap_plugin *self,
            const char *api, bool is_floating)
        { return _cast(self)->plugin.plug_gui_is_api_supported(api, is_floating); }

        static bool _get_preferred_api(const clap_plugin *self,
            const char **api, bool *is_floating)
        { return _cast(self)->plugin.plug_gui_get_preferred_api(api, is_floating); }
        
        static bool _create(const clap_plugin *self,
            const char *api, bool is_floating)
        { return _cast(self)->plugin.plug_gui_create(api, is_floating); }

        static void _destroy(const clap_plugin *self)
        { _cast(self)->plugin.plug_gui_destroy(); }

        static bool _set_scale(const clap_plugin *self, double scale)
        { return _cast(self)->plugin.plug_gui_set_scale(scale); }

        static bool _get_size(const clap_plugin *self, uint32_t *w, uint32_t *h)
        { return _cast(self)->plugin.plug_gui_get_size(w,h); }

        static bool _can_resize(const clap_plugin *self)
        { return _cast(self)->plugin.plug_gui_can_resize(); }

        static bool _get_resize_hints(
            const clap_plugin *self, clap_gui_resize_hints *hints)
        { return _cast(self)->plugin.plug_gui_get_resize_hints(hints); }

        static bool _adjust_size(const clap_plugin *self, uint32_t *w, uint32_t *h)
        { return _cast(self)->plugin.plug_gui_adjust_size(w,h); }
        
        static bool _set_size(const clap_plugin *self, uint32_t w, uint32_t h)
        { return _cast(self)->plugin.plug_gui_set_size(w,h); }

        static bool _set_parent(const clap_plugin *self, const clap_window *win)
        { return _cast(self)->plugin.plug_gui_set_parent(win); }
        static bool _set_transient(const clap_plugin *self, const clap_window *win)
        { return _cast(self)->plugin.plug_gui_set_transient(win); }
        
        static void _suggest_title(const clap_plugin *self, const char *title)
        { _cast(self)->plugin.plug_gui_suggest_title(title); }

        static bool _show(const clap_plugin *self)
        { return _cast(self)->plugin.plug_gui_show(); }
        static bool _hide(const clap_plugin *self)
        { return _cast(self)->plugin.plug_gui_hide(); }
    };
    
    template <typename Plugin>
    const clap_plugin_gui ClapExt_GUI<Plugin>::ext =
    {
        .is_api_supported   = ClapExt_GUI<Plugin>::_is_api_supported,
        .get_preferred_api  = ClapExt_GUI<Plugin>::_get_preferred_api,
        .create             = ClapExt_GUI<Plugin>::_create,
        .destroy            = ClapExt_GUI<Plugin>::_destroy,
        .set_scale          = ClapExt_GUI<Plugin>::_set_scale,
        .get_size           = ClapExt_GUI<Plugin>::_get_size,
        .can_resize         = ClapExt_GUI<Plugin>::_can_resize,
        .get_resize_hints   = ClapExt_GUI<Plugin>::_get_resize_hints,
        .adjust_size        = ClapExt_GUI<Plugin>::_adjust_size,
        .set_size           = ClapExt_GUI<Plugin>::_set_size,
        .set_parent         = ClapExt_GUI<Plugin>::_set_parent,
        .set_transient      = ClapExt_GUI<Plugin>::_set_transient,
        .suggest_title      = ClapExt_GUI<Plugin>::_suggest_title,
        .show               = ClapExt_GUI<Plugin>::_show,
        .hide               = ClapExt_GUI<Plugin>::_hide,
    };

    // see ClapFactory
    struct ClapFactoryBase
    {
        virtual clap_plugin_descriptor * get_descriptor() = 0;
        virtual clap_plugin * create(const clap_host * host) = 0;

        static unsigned get_count() { return factory_count; }
        static ClapFactoryBase * get_list() { return factory_list; }
        
        ClapFactoryBase * get_next() { return next; }
        
    protected:
        ~ClapFactoryBase() {}

        // singly linked list
        ClapFactoryBase * next = 0;

        // these are zero-initialized
        static ClapFactoryBase    *factory_list;
        static unsigned                 factory_count;
    };
    
    // One should declare one static factory object per plugin.
    // These automatically registers themselves into a simple list,
    // which is then enumerated by the actual plugin entry point.
    //
    // One should never create or destroy these dynamically.
    template <typename Plugin>
    struct ClapFactory : ClapFactoryBase
    {
        ClapFactory()
        {
            next = factory_list;
            factory_list = this;
            ++factory_count;
        }
        
        clap_plugin_descriptor * get_descriptor()
        {
            return &Plugin::plug_desc;
        }
        
        clap_plugin * create(const clap_host * host)
        {
            return new ClapWrapper<Plugin>(host);
        }
    };
};