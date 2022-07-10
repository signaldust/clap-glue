
/* Copyright (C) 2022 pihlaja@signaldust.com, use as you please, no warranty */

#include "clap-glue.h"

// The mundate details of entry-point and factories..

dust::ClapFactoryBase *
    dust::ClapFactoryBase::factory_list = 0;
unsigned
    dust::ClapFactoryBase::factory_count = 0;

static const clap_plugin *factory_create_plug(
    const clap_plugin_factory_t *, const clap_host *host, const char *plug_id)
{
    if(!clap_version_is_compatible(host->clap_version)) return 0;

    auto * p = dust::ClapFactoryBase::get_list();
    while(p)
    {
        if(!strcmp(plug_id, p->get_descriptor()->id))
            return p->create(host);
        p = p->get_next();
    }
    return 0;
}

static uint32_t factory_get_count(const clap_plugin_factory_t *)
{
    return dust::ClapFactoryBase::get_count();
}

static const clap_plugin_descriptor_t * factory_get_desc(
    const clap_plugin_factory_t *, uint32_t i)
{
    auto * p = dust::ClapFactoryBase::get_list();
    while(i--) { p = p->get_next(); }
    return p->get_descriptor();
}

static const clap_plugin_factory_t plugin_factory =
{
    .get_plugin_count = factory_get_count,
    .get_plugin_descriptor = factory_get_desc,
    .create_plugin = factory_create_plug,
};

static bool entry_init(const char *plugin_path)
{
    return true;
}
static void entry_deinit(void) { }

static const void *entry_get_factory(const char *factory_id)
{
    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID)) return &plugin_factory;
    return 0;
}

CLAP_EXPORT const clap_plugin_entry clap_entry =
{
    .clap_version   = CLAP_VERSION,
    .init           = entry_init,
    .deinit         = entry_deinit,
    .get_factory    = entry_get_factory,
};

