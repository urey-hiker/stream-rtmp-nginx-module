
/*
 * Copyright (C) Roman Arutyunyan
 */


#include <ngx_config.h>
#include <ngx_core.h>


#include "ngx_rtmp.h"


static ngx_int_t ngx_rtmp_init_process(ngx_cycle_t *cycle);


#if (nginx_version >= 1007011)
ngx_queue_t                         ngx_rtmp_init_queue;
#elif (nginx_version >= 1007005)
ngx_thread_volatile ngx_queue_t     ngx_rtmp_init_queue;
#else
ngx_thread_volatile ngx_event_t    *ngx_rtmp_init_queue;
#endif


ngx_uint_t  ngx_rtmp_max_module;


static ngx_core_module_t  ngx_rtmp_module_ctx = {
    ngx_string("rtmp"),
    NULL,
    NULL
};


ngx_module_t  ngx_rtmp_module = {
    NGX_MODULE_V1,
    &ngx_rtmp_module_ctx,                  /* module context */
    NULL,                                  /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    ngx_rtmp_init_process,                 /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


ngx_int_t
ngx_rtmp_init_events(ngx_conf_t *cf, ngx_rtmp_core_main_conf_t *cmcf)
{
    size_t                      n;

    for(n = 0; n < NGX_RTMP_MAX_EVENT; ++n) {
        if (ngx_array_init(&cmcf->events[n], cf->pool, 1,
                sizeof(ngx_rtmp_handler_pt)) != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    if (ngx_array_init(&cmcf->amf, cf->pool, 1,
                sizeof(ngx_rtmp_amf_handler_t)) != NGX_OK)
    {
        return NGX_ERROR;
    }

    return NGX_OK;
}


ngx_int_t
ngx_rtmp_init_event_handlers(ngx_conf_t *cf, ngx_rtmp_core_main_conf_t *cmcf)
{
    ngx_hash_init_t             calls_hash;
    ngx_rtmp_handler_pt        *eh;
    ngx_rtmp_amf_handler_t     *h;
    ngx_hash_key_t             *ha;
    size_t                      n, m;

    static size_t               pm_events[] = {
        NGX_RTMP_MSG_CHUNK_SIZE,
        NGX_RTMP_MSG_ABORT,
        NGX_RTMP_MSG_ACK,
        NGX_RTMP_MSG_ACK_SIZE,
        NGX_RTMP_MSG_BANDWIDTH
    };

    static size_t               amf_events[] = {
        NGX_RTMP_MSG_AMF_CMD,
        NGX_RTMP_MSG_AMF_META,
        NGX_RTMP_MSG_AMF_SHARED,
        NGX_RTMP_MSG_AMF3_CMD,
        NGX_RTMP_MSG_AMF3_META,
        NGX_RTMP_MSG_AMF3_SHARED
    };

    /* init standard protocol events */
    for(n = 0; n < sizeof(pm_events) / sizeof(pm_events[0]); ++n) {
        eh = ngx_array_push(&cmcf->events[pm_events[n]]);
        *eh = ngx_rtmp_protocol_message_handler;
    }

    /* init amf events */
    for(n = 0; n < sizeof(amf_events) / sizeof(amf_events[0]); ++n) {
        eh = ngx_array_push(&cmcf->events[amf_events[n]]);
        *eh = ngx_rtmp_amf_message_handler;
    }

    /* init user protocol events */
    eh = ngx_array_push(&cmcf->events[NGX_RTMP_MSG_USER]);
    *eh = ngx_rtmp_user_message_handler;

    /* aggregate to audio/video map */
    eh = ngx_array_push(&cmcf->events[NGX_RTMP_MSG_AGGREGATE]);
    *eh = ngx_rtmp_aggregate_message_handler;

    /* init amf callbacks */
    ngx_array_init(&cmcf->amf_arrays, cf->pool, 1, sizeof(ngx_hash_key_t));

    h = cmcf->amf.elts;
    for(n = 0; n < cmcf->amf.nelts; ++n, ++h) {
        ha = cmcf->amf_arrays.elts;
        for(m = 0; m < cmcf->amf_arrays.nelts; ++m, ++ha) {
            if (h->name.len == ha->key.len
                    && !ngx_strncmp(h->name.data, ha->key.data, ha->key.len))
            {
                break;
            }
        }
        if (m == cmcf->amf_arrays.nelts) {
            ha = ngx_array_push(&cmcf->amf_arrays);
            ha->key = h->name;
            ha->key_hash = ngx_hash_key_lc(ha->key.data, ha->key.len);
            ha->value = ngx_array_create(cf->pool, 1,
                    sizeof(ngx_rtmp_handler_pt));
            if (ha->value == NULL) {
                return NGX_ERROR;
            }
        }

        eh = ngx_array_push((ngx_array_t*)ha->value);
        *eh = h->handler;
    }

    calls_hash.hash = &cmcf->amf_hash;
    calls_hash.key = ngx_hash_key_lc;
    calls_hash.max_size = 512;
    calls_hash.bucket_size = ngx_cacheline_size;
    calls_hash.name = "amf_hash";
    calls_hash.pool = cf->pool;
    calls_hash.temp_pool = NULL;

    if (ngx_hash_init(&calls_hash, cmcf->amf_arrays.elts, cmcf->amf_arrays.nelts)
            != NGX_OK)
    {
        return NGX_ERROR;
    }

    return NGX_OK;
}


ngx_int_t
ngx_rtmp_fire_event(ngx_rtmp_session_t *s, ngx_uint_t evt,
        ngx_rtmp_header_t *h, ngx_chain_t *in)
{
    ngx_rtmp_core_main_conf_t      *cmcf;
    ngx_array_t                    *ch;
    ngx_rtmp_handler_pt            *hh;
    size_t                          n;

    cmcf = ngx_rtmp_get_module_main_conf(s, ngx_rtmp_core_module);

    ch = &cmcf->events[evt];
    hh = ch->elts;
    for(n = 0; n < ch->nelts; ++n, ++hh) {
        if (*hh && (*hh)(s, h, in) != NGX_OK) {
            return NGX_ERROR;
        }
    }
    return NGX_OK;
}


void *
ngx_rtmp_rmemcpy(void *dst, const void* src, size_t n)
{
    u_char     *d, *s;

    d = dst;
    s = (u_char*)src + n - 1;

    while(s >= (u_char*)src) {
        *d++ = *s--;
    }

    return dst;
}


static ngx_int_t
ngx_rtmp_init_process(ngx_cycle_t *cycle)
{
#if (nginx_version >= 1007005)
    ngx_queue_init(&ngx_rtmp_init_queue);
#endif
    return NGX_OK;
}
