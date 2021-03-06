
#pragma once

#include "clap-glue.h"
#include "dust/gui/window.h"
#include "dust/thread/thread.h"
#include "dust/core/hash.h"

// This wrapper implements dust-toolkit specific functionality.
//
// For the toolkit-independent base-wrappers, see clap-glue.h
//
// This is absolutely work-in-progress and might get completely redesigned..
//
namespace dust
{
#ifdef _WIN32
    static const char * clap_gui_platform_api = CLAP_WINDOW_API_WIN32;
#endif
#ifdef __APPLE__
    static const char * clap_gui_platform_api = CLAP_WINDOW_API_COCOA;
#endif

    // FIXME: this is early draft - at least initialization logic needs thinking
    // .. also there's the question if this should be kept free of clap-deps?
    struct AudioParam
    {
        // default to automatable for now
        clap_param_info_flags   clap_flags = CLAP_PARAM_IS_AUTOMATABLE;
        clap_id                 id  = -1;   // filled by register_param

        // FIXME: do we want std::string here?
        const char  *name   = "<param>";
        const char  *module = "";

        // FIXME: this is a lot of useless copying, but .. idk..
        std::function<std::string(float)>   value_to_text
            = [](float v) { return strf("%.3f", v); };
            
        std::function<float(const char *)>  text_to_value
            = [](const char * txt) { return parseNumeric(txt); };

        // these are for GUI to call
        std::function<void(bool)>   setEdit = [](bool) { assert(false); };
        std::function<void(float)>  setValue = [](float) { assert(false); };

        bool        inGesture = false;  // DSP side, use setEdit() from GUI
        
        float       value           = .5f;
        float       value_default   = .5f;

        // helper
        static float parseNumeric(const char * txt)
        { float v; sscanf(txt, "%f", &v); return v; }
    };

    // This is mostly for internal GUI -> DSP use below.
    struct ClapEventQueue
    {
        // Send event
        template <typename T>
        bool send(T & ev)
        {
            clap_event_header * header = &ev.header;   // typecheck
            return queue.send((uint8_t*)header, header->size);
        }

        // Receive all events from the GUI
        template <typename Fn> void recv(Fn && fn) { recv(fn); }
        template <typename Fn> void recv(Fn & fn)
        {
            unsigned size = queue.recv(recv_buf, queue_size);
            unsigned offset = 0;
            while(offset < size)
            {
                auto * header = (clap_event_header*) (recv_buf + offset);
                offset += header->size;
                fn(header);
            }
            // assert(size == offset);
        }

        // helper
        bool setParamEditState(AudioParam & p, bool state)
        {
            clap_event_param_gesture ev =
            {
                .header = {
                    .size = sizeof(ev),
                    .time = 0,
                    .space_id = CLAP_CORE_EVENT_SPACE_ID,
                    .type = (uint16_t) (state
                        ? CLAP_EVENT_PARAM_GESTURE_BEGIN
                        : CLAP_EVENT_PARAM_GESTURE_END),
                    .flags = CLAP_EVENT_IS_LIVE,
                },
                .param_id = p.id,
            };

            return send(ev);
        }

        // helper
        bool setParamValue(AudioParam & p, float value)
        {
            clap_event_param_value ev =
            {
                .header = {
                    .size = sizeof(ev),
                    .time = 0,
                    .space_id = CLAP_CORE_EVENT_SPACE_ID,
                    .type = CLAP_EVENT_PARAM_VALUE,
                    .flags = CLAP_EVENT_IS_LIVE,
                },

                .param_id = p.id,
                .cookie = (void*) &p,
                
                .note_id = -1,
                .port_index = -1,
                .channel = -1,
                .key = -1,

                .value = value,
            };

            return send(ev);
        }

    private:
        static const unsigned queue_size = 4096;
        
        RTQueue<uint8_t, queue_size>    queue;
        uint8_t                         recv_buf[queue_size];
    };

    // Some basic stuff ...
    //
    // Eventually this should probably implement some basic parameter handling?
    //
    struct ClapBase : WindowDelegate
    {
        // Extension interfaces that we currently make use of..
        struct {
            const clap_host         *host;
            const clap_host_params  *host_params;
            const clap_host_gui     *host_gui;
        } clap = {};

        struct {
            // Names of ports - first one is always main
            // FIXME: might want to specify more stuff (eg. nChannels?)
            std::vector<const char*>    audioIn;
            std::vector<const char*>    audioOut;
        } properties;

        Panel           plug_editor;    // Top level plug_editor Panel; use as a parent.
        ClapEventQueue  gui_to_dsp;     // GUI to DSP event queue

        // short-hand for requesting flush
        void flush_events()
        {
            if(clap.host_params) clap.host_params->request_flush(clap.host);
        }

        void register_param(AudioParam & param)
        {
            auto * p = &param;

            p->id = plug_params.size();
            plug_params.push_back(p);

            p->setEdit = [this, p] (bool b)
            { gui_to_dsp.setParamEditState(*p, b); flush_events(); };

            p->setValue = [this, p] (float v)
            { gui_to_dsp.setParamValue(*p, v); flush_events(); };

        }
        
        void flush_gui_events(const clap_output_events *out)
        {
            auto parse = [this](const clap_event_header * header)
            {
                if(header->space_id != CLAP_CORE_EVENT_SPACE_ID) return;

                if(header->type == CLAP_EVENT_PARAM_GESTURE_BEGIN)
                {
                    auto * ev = (clap_event_param_gesture*) header;
                    plug_params[ev->param_id]->inGesture = true;
                    return;
                }
                
                if(header->type == CLAP_EVENT_PARAM_GESTURE_END)
                {
                    auto * ev = (clap_event_param_gesture*) header;
                    plug_params[ev->param_id]->inGesture = false;
                    return;
                }

                if(header->type != CLAP_EVENT_PARAM_VALUE) return;

                auto * ev = (clap_event_param_value*) header;

                // we don't allow any of this for automation
                if(ev->note_id != -1 || ev->port_index != -1
                || ev->channel != -1 || ev->key != -1) return;

                auto * p = plug_params[ev->param_id];
                p->value = ev->value;
            };

            // parse value events, then send everything to host?
            gui_to_dsp.recv([&](const clap_event_header * ev)
            { parse(ev); out->try_push(out, ev); });
        }

        ClapBase(const clap_host * _host)
        {
            clap.host = _host;
            plug_editor.style.rule = LayoutStyle::FILL;
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

        // parameter support
        uint32_t plug_params_count() { return plug_params.size(); }

        bool plug_params_get_info(uint32_t index, clap_param_info *info)
        {
            if(index >= plug_params.size()) return false;

            auto * p = plug_params[index];
            
            info->id = p->id;
            info->flags = p->clap_flags;
            info->cookie = (void*) p;
            
            strncpy(info->name, p->name, CLAP_NAME_SIZE);
            info->name[CLAP_NAME_SIZE-1] = 0;

            strncpy(info->module, p->module, CLAP_PATH_SIZE);
            info->module[CLAP_PATH_SIZE-1] = 0;

            info->min_value = 0;
            info->max_value = 1;
            info->default_value = p->value_default;

            return true;
        }

        bool plug_params_get_value(clap_id id, double *value)
        {
            if(id >= plug_params.size()) return false;
            auto * p = plug_params[id];
            memfence();
            *value = p->value;
            memfence();
            return true;
        }

        bool plug_params_value_to_text(clap_id id, double v, char *txt, uint32_t size)
        {
            if(id >= plug_params.size()) return false;

            auto s = plug_params[id]->value_to_text(v);
            
            strncpy(txt, s.c_str(), size);
            txt[size-1] = 0;

            return true;
        }

        bool plug_params_text_to_value(clap_id id, const char * txt, double *value)
        {
            if(id >= plug_params.size()) return false;
            *value = plug_params[id]->text_to_value(txt);
            return true;
        }

        void plug_params_flush(
            const clap_input_events *in, const clap_output_events *out)
        {
            flush_gui_events(out);
            
            auto parse = [this](const clap_event_header * header)
            {
                // supposedly we only care about parameters here
                if(header->space_id != CLAP_CORE_EVENT_SPACE_ID
                || header->type != CLAP_EVENT_PARAM_VALUE) return;

                auto * ev = (clap_event_param_value*) header;

                // we don't allow any of this for automation
                if(ev->note_id != -1 || ev->port_index != -1
                || ev->channel != -1 || ev->key != -1) return;

                auto * p = plug_params[ev->param_id];
                if(!p->inGesture) p->value = ev->value;
            };

            if(in)
            {
                auto inputSize = in->size(in);
                for(int i = 0; i < inputSize; ++i)
                {
                    parse(in->get(in, i));
                }
            }
        }
        
        
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
            
            plug_editor.computeSize(_gui_data.sizeX, _gui_data.sizeY);
            return true;
        }
        
        void plug_gui_destroy()
        {
            // should never happen, but just in case..
            auto * win = plug_editor.getWindow();
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
            if(plug_editor.getWindow()) return false;
    
            int szX = (_gui_data.sizeX * _gui_data.scale) / 100;
            int szY = (_gui_data.sizeY * _gui_data.scale) / 100;
        
            auto * win = createWindow(*this, _gui_data.parent, szX, szY);
            plug_editor.setParent(win);
            
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
            auto * win = plug_editor.getWindow();
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

        // references to parameters
        std::vector<AudioParam*>    plug_params;
    };

};