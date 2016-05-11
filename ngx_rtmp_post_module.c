

/*
 * Copyright (C) Roman Arutyunyan
 */


#include <ngx_config.h>
#include <ngx_core.h>


#include "ngx_rtmp.h"


static ngx_int_t ngx_rtmp_post_postconfiguration(ngx_conf_t *cf);


ngx_int_t ngx_rtmp_init_event_handlers(ngx_conf_t *cf,
    ngx_rtmp_core_main_conf_t *cmcf);


static ngx_rtmp_module_t  ngx_rtmp_post_module_ctx = {
    ngx_rtmp_post_postconfiguration,     /* postconfiguration */

    NULL,                                /* create main configuration */
    NULL,                                /* init main configuration */

    NULL,                                /* create server configuration */
    NULL                                 /* merge server configuration */
};


ngx_module_t  ngx_rtmp_post_module = {
    NGX_MODULE_V1,
    &ngx_rtmp_post_module_ctx,             /* module context */
    NULL,                                  /* module directives */
    NGX_RTMP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_rtmp_post_postconfiguration(ngx_conf_t *cf)
{
    ngx_rtmp_core_main_conf_t *cmcf;

    cmcf = ngx_rtmp_conf_get_module_main_conf(cf, ngx_rtmp_core_module);

    if (ngx_rtmp_init_event_handlers(cf, cmcf) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}
