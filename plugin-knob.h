
#pragma once

#include "plugin-clap.h"
#include "dust/gui/panel.h"

namespace dust
{
    // Basic knob logic
    struct PluginKnobBase : Panel
    {
        AudioParam  *param;

        Notify  onValueChanged = doNothing;

        float   mouseDiv = (1 / 600.f); // per-pixel mouse-drag adjustment
        
        bool ev_mouse(const MouseEvent & e)
        {
            if(!param) return true; // don't crash, we'll show this in GUI
            
            if(!hover) redraw(true);
            hover = true;
            
            if(e.type == MouseEvent::tDown && e.button == 1)
            {
                inDrag = true;
                if(!inScroll) param->setEdit(true);
                
                dragFrom = e.y;
                redraw(true);
                if(e.nClick > 1)
                {
                    value = param->value_default;
                    param->setValue(value);
                    onValueChanged();
                }
            }
    
            if(e.type == MouseEvent::tMove && inDrag == true)
            {
                float scale = (e.keymods & KEYMOD_SHIFT) ? 0.1f : 1.f;
                float delta = (dragFrom - e.y) * scale * mouseDiv;
                dragFrom = e.y;
    
                value += delta;
                if(value > 1.f) value = 1.f;
                if(value < 0.f) value = 0.f;
    
                param->setValue(value);
                onValueChanged();
                
                redraw(true);
            }
    
            if(e.type == MouseEvent::tScroll)
            {
                float scale = (e.keymods & KEYMOD_SHIFT) ? 0.1f : 1.f;
                value += e.scrollY * scale * mouseDiv;
                if(value > 1.f) value = 1.f;
                if(value < 0.f) value = 0.f;

                if(!inScroll && !inDrag)
                {
                    param->setEdit(true);
                }
                inScroll = true;
    
                param->setValue(value);
                onValueChanged();
                
                redraw(true);
            }
            else
            if(inScroll && !(e.flags & MouseEvent::Flags::hoverOnScroll))
            {
                // anything not scroll clear scroll-edit state
                inScroll = false;
                if(!inDrag)
                {
                    param->setEdit(false);
                    redraw(true);
                }
            }
            
            if(e.type == MouseEvent::tUp)
            {
                inDrag = false;
                param->setEdit(false);
                redraw(true);
            }
    
            return true;
        }

        void ev_mouse_exit()
        {
            hover = false;
            redraw(true);

            // shouldn't ever get exit while dragging
            // but might just as well check
            if(inScroll || inDrag)
            {
                inScroll = false;
                inDrag = false;
                param->setEdit(false);
            }
        }

        void ev_update()
        {
            if(!param) return;

            memfence();
            float v = param->value;
            memfence();
            
            if(v != value)
            {
                value = v;
                redraw(true);
                onValueChanged();
            }
        }

    protected:
        bool    hover = false;      // hover-state
        bool    inDrag = false;     // drag-state
        bool    inScroll = false;   // edit-state while scrolling

        float   value = .5;

    private:
        int     dragFrom = 0;
        
    };

    struct PluginKnob : PluginKnobBase
    {
        int         rangeDiv = 4;           // how many segments to draw
        std::string label;
        Font        font;
        
        PluginKnob()
        {
            // these are just some sensible defaults
            style.rule = LayoutStyle::WEST;
            style.minSizeX = 36;
            style.minSizeY = 48;
        }
        
        void ev_dpi(float dpi)
        {
            if(!font.valid(dpi)) font.loadDefaultFont(8.f, dpi);
        }

        // Perhaps replace this with something more basic?
        void render(RenderContext & rc)
        {
            Path p;
    
            float pt = getWindow()->pt();
            float deg = acos(-1.f)/180.f;
            float angle = deg * (-140.f + value * 280.f);
    
            // center point
            float cx = .5f * layout.w;
            float cy = .5f * layout.h - 6.f*pt;
    
            float rMarkInner = 3*pt;
            float rMarkOuter = 6*pt;
            float rKnob = 8*pt;
            float rRimInner = 11*pt;
            float rRimOuter = 13*pt;
    
            p.arc(cx, cy, rRimInner, -140.f*deg, 140.f*deg, true);
            for(int i = 0; i <= rangeDiv; ++i)
            {
                float ia = deg * (-140.f + (i/float(rangeDiv))*280.f);
                float ax = sin(ia);
                float ay = -cos(ia);
                p.move(cx+ax*rRimInner, cy+ay*rRimInner);
                p.line(cx+ax*rRimOuter, cy+ay*rRimOuter);
            }
            rc.strokePath(p, .75f*pt, paint::Color(theme.fgMidColor));
            p.clear();
    
            p.arc(cx, cy, rKnob, 0, 360.f*deg, true);
            p.close();
            
            // draw a shadow - use temp alpha mask (FIXME: cache?)
            {
                Rect sr(0,0,layout.w,layout.h);
                std::vector<Alpha> shadow(layout.w * layout.h);
                // do this early, so we have the right rect size
                auto paint = paint::ColorMask
                    (0xff<<24, shadow.data(), layout.w, sr);
                
                renderPathRef(p, sr, FILL_EVENODD,
                    shadow.data(), layout.w, 2, false);
    
                unsigned dec = (unsigned) (expf(-1.f/(3.f*pt)) * 0x10000);
    
                for(int y = 1; y < layout.h; ++y)
                {
                    unsigned a0 = 0, a1 = 0, a2 = 0;
                    for(int x = 0; x < layout.w; ++x)
                    {
                        a0 = a1; a1 = a2; a2 = shadow[x+(y-1)*layout.w];
    
                        if(x) shadow[(x-1)+y*layout.w] = std::max<uint8_t>
                            ((dec*(a0 + 2*a1 + a2)) >> 18, shadow[(x-1)+y*layout.w]);
                    }
                }
                rc.fill(paint);
            }
    
            rc.strokePath(p, 3.f*pt, paint::Color(theme.winColor));
            rc.fillPath(p, paint::Gradient2(
                theme.selColor, 0, 0,
                theme.midColor, 0, layout.h));
            rc.strokePath(p, 1.5*pt,
                paint::Gradient2(
                hover ? theme.fgColor : theme.fgMidColor, 0, 0,
                theme.bgColor, 0, layout.h));

            // don't draw a marker if param is null
            if(param)
            {
                p.clear();
                float ax = sin(angle);
                float ay = -cos(angle);
                p.move(cx+ax*rMarkInner, cy+ay*rMarkInner);
                p.line(cx+ax*rMarkOuter, cy+ay*rMarkOuter);
                rc.strokePath(p, 2.f*pt, paint::Color(theme.fgColor));
            }

            if(font.valid())
            {
                rc.drawCenteredText(font, label, paint::Color(theme.fgColor),
                    cx, layout.h - 9.f*pt + font->getVertOffset());
            }
        }
    };


};