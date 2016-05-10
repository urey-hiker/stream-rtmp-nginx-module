
/*
 * Copyright (C) Roman Arutyunyan
 */


#include <ngx_config.h>
#include <ngx_core.h>


#include "ngx_rtmp.h"


static void *ngx_rtmp_core_create_main_conf(ngx_conf_t *cf);
static void *ngx_rtmp_core_create_srv_conf(ngx_conf_t *cf);
static char *ngx_rtmp_core_merge_srv_conf(ngx_conf_t *cf, void *parent,
    void *child);
static void *ngx_rtmp_core_create_app_conf(ngx_conf_t *cf);
static char *ngx_rtmp_core_merge_app_conf(ngx_conf_t *cf, void *parent,
    void *child);
static void ngx_rtmp_core_handler(ngx_stream_session_t *s);
static char *ngx_rtmp_core_rtmp(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_rtmp_core_application(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
ngx_int_t ngx_rtmp_init_events(ngx_conf_t *cf,
    ngx_rtmp_core_main_conf_t *cmcf);


static ngx_command_t  ngx_rtmp_core_commands[] = {

    { ngx_string("rtmp"),
      NGX_RTMP_SRV_CONF|NGX_CONF_NOARGS,
      ngx_rtmp_core_rtmp,
      0,
      0,
      NULL },

    { ngx_string("rtmp_application"),
      NGX_RTMP_SRV_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE1,
      ngx_rtmp_core_application,
      NGX_RTMP_SRV_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("rtmp_proxy_protocol"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, proxy_protocol),
      NULL },

    { ngx_string("rtmp_timeout"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, timeout),
      NULL },

    { ngx_string("rtmp_ping"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, ping),
      NULL },

    { ngx_string("rtmp_ping_timeout"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, ping_timeout),
      NULL },

    { ngx_string("rtmp_max_streams"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, max_streams),
      NULL },

    { ngx_string("rtmp_ack_window"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, ack_window),
      NULL },

    { ngx_string("rtmp_chunk_size"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, chunk_size),
      NULL },

    { ngx_string("rtmp_max_message"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, max_message),
      NULL },

    { ngx_string("rtmp_out_queue"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, out_queue),
      NULL },

    { ngx_string("rtmp_out_cork"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, out_cork),
      NULL },

    { ngx_string("rtmp_busy"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, busy),
      NULL },

    /* time fixes are needed for flash clients */
    { ngx_string("rtmp_play_time_fix"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_RTMP_APP_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, play_time_fix),
      NULL },

    { ngx_string("rtmp_publish_time_fix"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_RTMP_APP_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, publish_time_fix),
      NULL },

    { ngx_string("rtmp_buflen"),
      NGX_RTMP_MAIN_CONF|NGX_RTMP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_RTMP_SRV_CONF_OFFSET,
      offsetof(ngx_rtmp_core_srv_conf_t, buflen),
      NULL },

      ngx_null_command
};


static ngx_rtmp_module_t  ngx_rtmp_core_module_ctx = {
    NULL,                                   /* postconfiguration */
    ngx_rtmp_core_create_main_conf,         /* create main configuration */
    NULL,                                   /* init main configuration */
    ngx_rtmp_core_create_srv_conf,          /* create server configuration */
    ngx_rtmp_core_merge_srv_conf,           /* merge server configuration */
};


ngx_module_t  ngx_rtmp_core_module = {
    NGX_MODULE_V1,
    &ngx_rtmp_core_module_ctx,             /* module context */
    ngx_rtmp_core_commands,                /* module directives */
    NGX_RTMP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    (uintptr_t)ngx_rtmp_core_create_app_conf,
    (uintptr_t)ngx_rtmp_core_merge_app_conf,
    NGX_RTMP_MODULE_V1_PADDING
};


static void *
ngx_rtmp_core_create_main_conf(ngx_conf_t *cf)
{
    ngx_rtmp_core_main_conf_t  *cmcf;

    cmcf = ngx_pcalloc(cf->pool, sizeof(ngx_rtmp_core_main_conf_t));
    if (cmcf == NULL) {
        return NULL;
    }

    cmcf->cf = cf;

    if (ngx_rtmp_init_events(cf, cmcf) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return cmcf;
}


static void *
ngx_rtmp_core_create_srv_conf(ngx_conf_t *cf)
{
    ngx_rtmp_core_srv_conf_t   *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_rtmp_core_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    if (ngx_array_init(&conf->applications, cf->pool, 4,
                       sizeof(ngx_rtmp_core_app_conf_t *))
        != NGX_OK)
    {
        return NULL;
    }

    conf->timeout = NGX_CONF_UNSET_MSEC;
    conf->ping = NGX_CONF_UNSET_MSEC;
    conf->ping_timeout = NGX_CONF_UNSET_MSEC;
    conf->proxy_protocol = NGX_CONF_UNSET;
    conf->max_streams = NGX_CONF_UNSET;
    conf->chunk_size = NGX_CONF_UNSET;
    conf->ack_window = NGX_CONF_UNSET_UINT;
    conf->max_message = NGX_CONF_UNSET_SIZE;
    conf->out_queue = NGX_CONF_UNSET_SIZE;
    conf->out_cork = NGX_CONF_UNSET_SIZE;
    conf->play_time_fix = NGX_CONF_UNSET;
    conf->publish_time_fix = NGX_CONF_UNSET;
    conf->buflen = NGX_CONF_UNSET_MSEC;
    conf->busy = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_rtmp_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_rtmp_core_srv_conf_t *prev = parent;
    ngx_rtmp_core_srv_conf_t *conf = child;

    ngx_conf_merge_msec_value(conf->timeout, prev->timeout, 60000);
    ngx_conf_merge_msec_value(conf->ping, prev->ping, 60000);
    ngx_conf_merge_msec_value(conf->ping_timeout, prev->ping_timeout, 30000);

    ngx_conf_merge_value(conf->proxy_protocol, prev->proxy_protocol, 0);
    ngx_conf_merge_value(conf->max_streams, prev->max_streams, 32);
    ngx_conf_merge_value(conf->chunk_size, prev->chunk_size, 4096);
    ngx_conf_merge_uint_value(conf->ack_window, prev->ack_window, 5000000);
    ngx_conf_merge_size_value(conf->max_message, prev->max_message,
            1 * 1024 * 1024);
    ngx_conf_merge_size_value(conf->out_queue, prev->out_queue, 256);
    ngx_conf_merge_size_value(conf->out_cork, prev->out_cork,
            conf->out_queue / 8);
    ngx_conf_merge_value(conf->play_time_fix, prev->play_time_fix, 1);
    ngx_conf_merge_value(conf->publish_time_fix, prev->publish_time_fix, 1);
    ngx_conf_merge_msec_value(conf->buflen, prev->buflen, 1000);
    ngx_conf_merge_value(conf->busy, prev->busy, 0);

    if (prev->pool == NULL) {
        prev->pool = ngx_create_pool(4096, &cf->cycle->new_log);
        if (prev->pool == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    conf->pool = prev->pool;

    return NGX_CONF_OK;
}


static void *
ngx_rtmp_core_create_app_conf(ngx_conf_t *cf)
{
    ngx_rtmp_core_app_conf_t   *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_rtmp_core_app_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    if (ngx_array_init(&conf->applications, cf->pool, 1,
                       sizeof(ngx_rtmp_core_app_conf_t *))
        != NGX_OK)
    {
        return NULL;
    }

    return conf;
}


static char *
ngx_rtmp_core_merge_app_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_rtmp_core_app_conf_t *prev = parent;
    ngx_rtmp_core_app_conf_t *conf = child;

    (void)prev;
    (void)conf;

    return NGX_CONF_OK;
}


static void
ngx_rtmp_core_handler(ngx_stream_session_t *s)
{
    ngx_rtmp_init_connection(s->connection);
}


static char *
ngx_rtmp_core_rtmp(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_stream_core_srv_conf_t  *cscf;

    cscf = ngx_stream_conf_get_module_srv_conf(cf, ngx_stream_core_module);

    cscf->handler = ngx_rtmp_core_handler;

    return NGX_CONF_OK;
}


static char *
ngx_rtmp_core_application(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char                       *rv;
    ngx_int_t                   i;
    ngx_str_t                  *value;
    ngx_conf_t                  save;
    ngx_stream_conf_ctx_t      *pctx;
    ngx_rtmp_conf_ctx_t        *ctx;
    ngx_rtmp_core_srv_conf_t   *cscf;
    ngx_rtmp_core_app_conf_t   *cacf, **cacfp;

    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_rtmp_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    pctx = cf->ctx;
    ctx->stream_ctx = pctx;

    ctx->app_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_stream_max_module);
    if (ctx->app_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    for (i = 0; cf->cycle->modules[i]; i++) {
        if (cf->cycle->modules[i]->type != NGX_RTMP_MODULE) {
            continue;
        }

        ngx_rtmp_create_app_pt create_app_conf =
                (ngx_rtmp_create_app_pt)cf->cycle->modules[i]->spare_hook0;

        if (create_app_conf == NULL) {
            continue;
        }

        ctx->app_conf[cf->cycle->modules[i]->ctx_index] =
                create_app_conf(cf);

        if (ctx->app_conf[cf->cycle->modules[i]->ctx_index] == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    cacf = ctx->app_conf[ngx_rtmp_core_module.ctx_index];
    cacf->app_conf = ctx->app_conf;

    value = cf->args->elts;

    cacf->name = value[1];
    cscf = pctx->srv_conf[ngx_rtmp_core_module.ctx_index];

    cacfp = ngx_array_push(&cscf->applications);
    if (cacfp == NULL) {
        return NGX_CONF_ERROR;
    }

    *cacfp = cacf;

    save = *cf;
    cf->ctx = ctx;
    cf->cmd_type = NGX_RTMP_APP_CONF;

    rv = ngx_conf_parse(cf, NULL);

    *cf= save;

    return rv;
}
