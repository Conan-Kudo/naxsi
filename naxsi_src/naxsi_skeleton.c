// SPDX-FileCopyrightText: 2016-2019, Thibault 'bui' Koechlin <tko@nbs-system.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/*
** This files contains skeleton functions,
** such as registered handlers. Readers already
** aware of nginx's modules can skip most of this.
*/

#include <ngx_config.h>

#include <naxsi.h>
#include <naxsi_net.h>

#include <ctype.h>

#ifndef _WIN32
#include <strings.h>
#include <sys/times.h>
#else
#include <process.h>
#endif // !_WIN32

#define NAXSI_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/*
** Macro used to print incorrect configuration lines
*/
#define ngx_http_naxsi_line_conf_error(cf, value)                                                  \
  do {                                                                                             \
    ngx_conf_log_error(NGX_LOG_EMERG,                                                              \
                       cf,                                                                         \
                       0,                                                                          \
                       "Naxsi-Config : Incorrect line %V %V (%s:%d)...",                           \
                       &(value[0]),                                                                \
                       &(value[1]),                                                                \
                       NAXSI_FILENAME,                                                               \
                       __LINE__);                                                                  \
  } while (0)

/*
** Module's registered function/handlers.
*/
static ngx_int_t
ngx_http_naxsi_access_handler(ngx_http_request_t* r);
static ngx_int_t
ngx_http_naxsi_push_loc_conf(ngx_conf_t* cf, ngx_http_naxsi_loc_conf_t* conf);
static char*
ngx_http_naxsi_read_main_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static ngx_int_t
ngx_http_naxsi_init(ngx_conf_t* cf);
static char*
ngx_http_naxsi_read_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

static char*
ngx_http_naxsi_cr_loc_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

static char*
ngx_http_naxsi_ud_loc_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

static char*
ngx_http_naxsi_flags_loc_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

static void*
ngx_http_naxsi_create_loc_conf(ngx_conf_t* cf);
static char*
ngx_http_naxsi_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);
void*
ngx_http_naxsi_create_main_conf(ngx_conf_t* cf);
void
ngx_http_naxsi_payload_handler(ngx_http_request_t* r);

static char*
ngx_http_naxsi_log_loc_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
  ngx_http_naxsi_loc_conf_t* alcf = conf;
  return ngx_log_set_log(cf, &alcf->log);
}

static ngx_http_request_ctx_t*
recover_request_ctx(ngx_http_request_t* r);

static void
ngx_http_module_cleanup_handler(void* data);

static ngx_int_t
ngx_http_naxsi_add_variables(ngx_conf_t* cf);

static ngx_int_t
ngx_http_naxsi_server_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_int_t
ngx_http_naxsi_uri_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_int_t
ngx_http_naxsi_learning_variable(ngx_http_request_t*        r,
                                 ngx_http_variable_value_t* v,
                                 uintptr_t                  data);

static ngx_int_t
ngx_http_naxsi_block_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_int_t
ngx_http_naxsi_total_processed_variable(ngx_http_request_t*        r,
                                        ngx_http_variable_value_t* v,
                                        uintptr_t                  data);

static ngx_int_t
ngx_http_naxsi_total_blocked_variable(ngx_http_request_t*        r,
                                      ngx_http_variable_value_t* v,
                                      uintptr_t                  data);

static ngx_int_t
ngx_http_naxsi_score_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_int_t
ngx_http_naxsi_match_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_int_t
ngx_http_naxsi_attack_family_variable(ngx_http_request_t*        r,
                                      ngx_http_variable_value_t* v,
                                      uintptr_t                  data);

static ngx_int_t
ngx_http_naxsi_attack_action_variable(ngx_http_request_t*        r,
                                      ngx_http_variable_value_t* v,
                                      uintptr_t                  data);

static ngx_int_t
ngx_http_naxsi_request_id(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data);

/* command handled by the module */
static ngx_command_t ngx_http_naxsi_commands[] = {
  /* BasicRule (in main) */
  { ngx_string(TOP_MAIN_BASIC_RULE_T),
    NGX_HTTP_MAIN_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_read_main_conf,
    NGX_HTTP_MAIN_CONF_OFFSET,
    0,
    NULL },

  /* BasicRule (in main) - nginx style */
  { ngx_string(TOP_MAIN_BASIC_RULE_N),
    NGX_HTTP_MAIN_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_read_main_conf,
    NGX_HTTP_MAIN_CONF_OFFSET,
    0,
    NULL },

  /* BasicRule (in loc) */
  { ngx_string(TOP_BASIC_RULE_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_read_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* BasicRule (in loc) - nginx style */
  { ngx_string(TOP_BASIC_RULE_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_read_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* DeniedUrl */
  { ngx_string(TOP_DENIED_URL_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_ud_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* DeniedUrl - nginx style */
  { ngx_string(TOP_DENIED_URL_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_ud_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* WhitelistIP */
  { ngx_string(TOP_IGNORE_IP_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_read_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* WhitelistCIDR */
  { ngx_string(TOP_IGNORE_CIDR_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_read_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* CheckRule */
  { ngx_string(TOP_CHECK_RULE_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_cr_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* CheckRule  - nginx style*/
  { ngx_string(TOP_CHECK_RULE_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_cr_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },
  /*
  ** flag rules
  */

  /* Learning Flag */
  { ngx_string(TOP_LEARNING_FLAG_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* Learning Flag (nginx style) */
  { ngx_string(TOP_LEARNING_FLAG_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* EnableFlag */
  { ngx_string(TOP_ENABLED_FLAG_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* EnableFlag (nginx style) */
  { ngx_string(TOP_ENABLED_FLAG_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* DisableFlag */
  { ngx_string(TOP_DISABLED_FLAG_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* DisableFlag (nginx style) */
  { ngx_string(TOP_DISABLED_FLAG_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* LibInjectionSql */
  { ngx_string(TOP_LIBINJECTION_SQL_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* LibInjectionSql (nginx style) */
  { ngx_string(TOP_LIBINJECTION_SQL_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* LibInjectionXss */
  { ngx_string(TOP_LIBINJECTION_XSS_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* LibInjectionXss (nginx style) */
  { ngx_string(TOP_LIBINJECTION_XSS_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
      NGX_CONF_NOARGS,
    ngx_http_naxsi_flags_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* NaxsiLogfile */
  { ngx_string(TOP_NAXSI_LOGFILE_T),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_log_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  /* NaxsiLogfile - nginx style*/
  { ngx_string(TOP_NAXSI_LOGFILE_N),
    NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_1MORE,
    ngx_http_naxsi_log_loc_conf,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },

  ngx_null_command
};

static ngx_http_variable_t ngx_http_naxsi_variables[] = {
  { ngx_string("naxsi_server"),     /* Name */
    NULL,                           /* Set handler */
    ngx_http_naxsi_server_variable, /* Get handler */
    0,                              /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,       /* Flags */
    0 },                            /* Index */

  { ngx_string("naxsi_uri"),     /* Name */
    NULL,                        /* Set handler */
    ngx_http_naxsi_uri_variable, /* Get handler */
    0,                           /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,    /* Flags */
    0 },                         /* Index */

  { ngx_string("naxsi_learning"),     /* Name */
    NULL,                             /* Set handler */
    ngx_http_naxsi_learning_variable, /* Get handler */
    0,                                /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,         /* Flags */
    0 },                              /* Index */

  { ngx_string("naxsi_block"),     /* Name */
    NULL,                          /* Set handler */
    ngx_http_naxsi_block_variable, /* Get handler */
    0,                             /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,      /* Flags */
    0 },                           /* Index */

  { ngx_string("naxsi_total_processed"),     /* Name */
    NULL,                                    /* Set handler */
    ngx_http_naxsi_total_processed_variable, /* Get handler */
    0,                                       /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,                /* Flags */
    0 },                                     /* Index */

  { ngx_string("naxsi_total_blocked"),     /* Name */
    NULL,                                  /* Set handler */
    ngx_http_naxsi_total_blocked_variable, /* Get handler */
    0,                                     /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,              /* Flags */
    0 },                                   /* Index */

  { ngx_string("naxsi_score"),     /* Name */
    NULL,                          /* Set handler */
    ngx_http_naxsi_score_variable, /* Get handler */
    0,                             /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,      /* Flags */
    0 },                           /* Index */

  { ngx_string("naxsi_match"),     /* Name */
    NULL,                          /* Set handler */
    ngx_http_naxsi_match_variable, /* Get handler */
    0,                             /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,      /* Flags */
    0 },                           /* Index */

  { ngx_string("naxsi_attack_family"),     /* Name */
    NULL,                                  /* Set handler */
    ngx_http_naxsi_attack_family_variable, /* Get handler */
    0,                                     /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,              /* Flags */
    0 },                                   /* Index */

  { ngx_string("naxsi_attack_action"),     /* Name */
    NULL,                                  /* Set handler */
    ngx_http_naxsi_attack_action_variable, /* Get handler */
    0,                                     /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,              /* Flags */
    0 },                                   /* Index */

  { ngx_string("naxsi_request_id"), /* Name */
    NULL,                           /* Set handler */
    ngx_http_naxsi_request_id,      /* Get handler */
    0,                              /* Data */
    NGX_HTTP_VAR_NOCACHEABLE,       /* Flags */
    0 },                            /* Index */

  { ngx_null_string, NULL, NULL, 0, 0, 0 } /* Sentinel */
};

/*
** handlers for configuration phases of the module
*/

static ngx_http_module_t ngx_http_naxsi_module_ctx = {
  ngx_http_naxsi_add_variables,    /* preconfiguration */
  ngx_http_naxsi_init,             /* postconfiguration */
  ngx_http_naxsi_create_main_conf, /* create main configuration */
  NULL,                            /* init main configuration */
  NULL,                            /* create server configuration */
  NULL,                            /* merge server configuration */
  ngx_http_naxsi_create_loc_conf,  /* create location configuration */
  ngx_http_naxsi_merge_loc_conf    /* merge location configuration */
};

ngx_module_t ngx_http_naxsi_module = { NGX_MODULE_V1,
                                       &ngx_http_naxsi_module_ctx, /* module context */
                                       ngx_http_naxsi_commands,    /* module directives */
                                       NGX_HTTP_MODULE,            /* module type */
                                       NULL,                       /* init master */
                                       NULL,                       /* init module */
                                       NULL,                       /* init process */
                                       NULL,                       /* init thread */
                                       NULL,                       /* exit thread */
                                       NULL,                       /* exit process */
                                       NULL,                       /* exit master */
                                       NGX_MODULE_V1_PADDING };

#define DEFAULT_MAX_LOC_T 10

void*
ngx_http_naxsi_create_main_conf(ngx_conf_t* cf)
{
  ngx_http_naxsi_main_conf_t* mc;

  mc = ngx_pcalloc(cf->pool, sizeof(ngx_http_naxsi_main_conf_t));
  if (!mc)
    return (NGX_CONF_ERROR); /*LCOV_EXCL_LINE*/
  mc->locations = ngx_array_create(cf->pool, DEFAULT_MAX_LOC_T, sizeof(ngx_http_naxsi_loc_conf_t*));
  if (!mc->locations)
    return (NGX_CONF_ERROR); /*LCOV_EXCL_LINE*/
  return (mc);
}

/* create log conf struct */
static void*
ngx_http_naxsi_create_loc_conf(ngx_conf_t* cf)
{
  ngx_http_naxsi_loc_conf_t* conf;

  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_naxsi_loc_conf_t));
  if (conf == NULL)
    return NULL;
  return (conf);
}

/* Push loc conf to main conf */
static ngx_int_t
ngx_http_naxsi_push_loc_conf(ngx_conf_t* cf, ngx_http_naxsi_loc_conf_t* conf)
{
  ngx_http_naxsi_loc_conf_t **bar;
  ngx_http_naxsi_main_conf_t* main_cf;

  if (!conf->pushed) {
    main_cf = ngx_http_conf_get_module_main_conf(cf, ngx_http_naxsi_module);
    bar = ngx_array_push(main_cf->locations);
    if (!bar)
      return (NGX_ERROR);
    *bar         = conf;
    conf->pushed = 1;
  }
  return (NGX_OK);
}

/* merge loc conf */
/* NOTE/WARNING : This function wasn't tested correctly.
   Actually, we shouldn't merge anything, as configuration is
   specific 'per' location ? */
static char*
ngx_http_naxsi_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
  ngx_http_naxsi_loc_conf_t* prev = parent;
  ngx_http_naxsi_loc_conf_t* conf = child;

  if (conf->get_rules == NULL)
    conf->get_rules = prev->get_rules;
  if (conf->raw_body_rules == NULL)
    conf->raw_body_rules = prev->raw_body_rules;
  if (conf->whitelist_rules == NULL)
    conf->whitelist_rules = prev->whitelist_rules;
  if (conf->check_rules == NULL)
    conf->check_rules = prev->check_rules;
  if (conf->body_rules == NULL)
    conf->body_rules = prev->body_rules;
  if (conf->header_rules == NULL)
    conf->header_rules = prev->header_rules;
  if (conf->generic_rules == NULL)
    conf->generic_rules = prev->generic_rules;
  if (conf->tmp_wlr == NULL)
    conf->tmp_wlr = prev->tmp_wlr;
  if (conf->rxmz_wlr == NULL)
    conf->rxmz_wlr = prev->rxmz_wlr;
  if (conf->wlr_url_hash == NULL)
    conf->wlr_url_hash = prev->wlr_url_hash;
  if (conf->wlr_args_hash == NULL)
    conf->wlr_args_hash = prev->wlr_args_hash;
  if (conf->wlr_body_hash == NULL)
    conf->wlr_body_hash = prev->wlr_body_hash;
  if (conf->wlr_headers_hash == NULL)
    conf->wlr_headers_hash = prev->wlr_headers_hash;
  if (conf->ignore_ips == NULL)
    conf->ignore_ips = prev->ignore_ips;
  if (conf->ignore_ips_ha.hsize == 0)
    conf->ignore_ips_ha = prev->ignore_ips_ha;
  if (conf->ignore_cidrs == NULL)
    conf->ignore_cidrs = prev->ignore_cidrs;
  if (conf->disabled_rules == NULL)
    conf->disabled_rules = prev->disabled_rules;

  if (conf->error == 0)
    conf->error = prev->error;
  if (conf->persistant_data == NULL)
    conf->persistant_data = prev->persistant_data;
  if (conf->extensive == 0)
    conf->extensive = prev->extensive;
  if (conf->learning == 0)
    conf->learning = prev->learning;
  if (conf->enabled == 0)
    conf->enabled = prev->enabled;
  if (conf->force_disabled == 0)
    conf->force_disabled = prev->force_disabled;
  if (conf->libinjection_sql_enabled == 0)
    conf->libinjection_sql_enabled = prev->libinjection_sql_enabled;
  if (conf->libinjection_xss_enabled == 0)
    conf->libinjection_xss_enabled = prev->libinjection_xss_enabled;
  if (conf->denied_url == NULL)
    conf->denied_url = prev->denied_url;
  if (conf->flag_enable_h == 0)
    conf->flag_enable_h = prev->flag_enable_h;
  if (conf->flag_learning_h == 0)
    conf->flag_learning_h = prev->flag_learning_h;
  if (conf->flag_post_action_h == 0)
    conf->flag_post_action_h = prev->flag_post_action_h;
  if (conf->flag_extensive_log_h == 0)
    conf->flag_extensive_log_h = prev->flag_extensive_log_h;
  if (conf->flag_json_log_h == 0)
    conf->flag_json_log_h = prev->flag_json_log_h;
  if (conf->flag_libinjection_xss_h == 0)
    conf->flag_libinjection_xss_h = prev->flag_libinjection_xss_h;
  if (conf->flag_libinjection_sql_h == 0)
    conf->flag_libinjection_sql_h = prev->flag_libinjection_sql_h;
  if (conf->log == NULL)
    conf->log = prev->log;

  if (ngx_http_naxsi_push_loc_conf(cf, conf) != NGX_OK)
    return NGX_CONF_ERROR;

  return NGX_CONF_OK;
}

/*
** This function sets up handlers for ACCESS_PHASE,
** and will call the hashtable creation function
** (whitelist aggregation)
*/
static ngx_int_t
ngx_http_naxsi_init(ngx_conf_t* cf)
{
  ngx_http_handler_pt*        h;
  ngx_http_core_main_conf_t*  cmcf;
  ngx_http_naxsi_main_conf_t* main_cf;
  ngx_http_naxsi_loc_conf_t** loc_cf;
  unsigned int                i;

  cmcf    = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
  main_cf = ngx_http_conf_get_module_main_conf(cf, ngx_http_naxsi_module);
  if (cmcf == NULL || main_cf == NULL)
    return (NGX_ERROR); /*LCOV_EXCL_LINE*/

  /* Register for rewrite phase */
  h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
  if (h == NULL)
    return (NGX_ERROR); /*LCOV_EXCL_LINE*/

  *h = ngx_http_naxsi_access_handler;
  /* Go with each locations registered in the srv_conf. */
  loc_cf = main_cf->locations->elts;

  for (i = 0; i < main_cf->locations->nelts; i++) {
    if (loc_cf[i]->enabled && (!loc_cf[i]->denied_url || loc_cf[i]->denied_url->len <= 0)) {
      /* LCOV_EXCL_START */
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "Missing DeniedURL, abort.");
      return (NGX_ERROR);
      /* LCOV_EXCL_STOP */
    }

    loc_cf[i]->flag_enable_h   = ngx_hash_key_lc((u_char*)RT_ENABLE, strlen(RT_ENABLE));
    loc_cf[i]->flag_learning_h = ngx_hash_key_lc((u_char*)RT_LEARNING, strlen(RT_LEARNING));
    loc_cf[i]->flag_post_action_h =
      ngx_hash_key_lc((u_char*)RT_POST_ACTION, strlen(RT_POST_ACTION));
    loc_cf[i]->flag_extensive_log_h =
      ngx_hash_key_lc((u_char*)RT_EXTENSIVE_LOG, strlen(RT_EXTENSIVE_LOG));
    loc_cf[i]->flag_json_log_h = ngx_hash_key_lc((u_char*)RT_JSON_LOG, strlen(RT_JSON_LOG));
    loc_cf[i]->flag_libinjection_xss_h =
      ngx_hash_key_lc((u_char*)RT_LIBINJECTION_XSS, strlen(RT_LIBINJECTION_XSS));
    loc_cf[i]->flag_libinjection_sql_h =
      ngx_hash_key_lc((u_char*)RT_LIBINJECTION_SQL, strlen(RT_LIBINJECTION_SQL));

    if (ngx_http_naxsi_create_hashtables_n(loc_cf[i], cf) != NGX_OK) {
      /* LCOV_EXCL_START */
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "WhiteList Hash building failed");
      return (NGX_ERROR);
      /* LCOV_EXCL_STOP */
    }
  }

  /* initialize prng (used for fragmented logs) */
#ifndef _WIN32
  srandom(time(0) * getpid());
#else  // _WIN32
  srand(time(0) * _getpid());
#endif // !_WIN32

  /*
  ** initalise internal rules for libinjection sqli/xss
  ** (needs proper special scores)
  */
  nx_int__libinject_sql = ngx_pcalloc(cf->pool, sizeof(ngx_http_rule_t));
  nx_int__libinject_xss = ngx_pcalloc(cf->pool, sizeof(ngx_http_rule_t));
  if (!nx_int__libinject_xss || !nx_int__libinject_sql)
    return (NGX_ERROR);
  nx_int__libinject_sql->sscores = ngx_array_create(cf->pool, 2, sizeof(ngx_http_special_score_t));
  nx_int__libinject_xss->sscores = ngx_array_create(cf->pool, 2, sizeof(ngx_http_special_score_t));
  if (!nx_int__libinject_sql->sscores || !nx_int__libinject_xss->sscores)
    return (NGX_ERROR); /* LCOV_EXCL_LINE */
  /* internal ID sqli - 17*/
  nx_int__libinject_sql->rule_id = 17;
  /* internal ID xss - 18*/
  nx_int__libinject_xss->rule_id = 18;
  /* libinjection sqli/xss - special score init */
  ngx_http_special_score_t* libjct_sql = ngx_array_push(nx_int__libinject_sql->sscores);
  ngx_http_special_score_t* libjct_xss = ngx_array_push(nx_int__libinject_xss->sscores);
  if (!libjct_sql || !libjct_xss)
    return (NGX_ERROR); /* LCOV_EXCL_LINE */
  libjct_sql->sc_tag = ngx_pcalloc(cf->pool, sizeof(ngx_str_t));
  libjct_xss->sc_tag = ngx_pcalloc(cf->pool, sizeof(ngx_str_t));
  if (!libjct_sql->sc_tag || !libjct_xss->sc_tag)
    return (NGX_ERROR); /* LCOV_EXCL_LINE */
  libjct_sql->sc_tag->data = ngx_pcalloc(cf->pool, 18 /* LIBINJECTION_SQL */);
  libjct_xss->sc_tag->data = ngx_pcalloc(cf->pool, 18 /* LIBINJECTION_XSS */);
  if (!libjct_sql->sc_tag->data || !libjct_xss->sc_tag->data)
    return (NGX_ERROR); /* LCOV_EXCL_LINE */
  memcpy((char*)libjct_sql->sc_tag->data, (char*)"$LIBINJECTION_SQL", 17);
  memcpy((char*)libjct_xss->sc_tag->data, (char*)"$LIBINJECTION_XSS", 17);
  libjct_xss->sc_tag->len = 17;
  libjct_sql->sc_tag->len = 17;
  libjct_sql->sc_score    = 8;
  libjct_xss->sc_score    = 8;

  return (NGX_OK);
}

/*
** my hugly configuration parsing function.
** should be rewritten, cause code is hugly and not bof proof at all
** does : top level parsing config function,
**	  see foo_cfg_parse.c for stuff
*/
static char*
ngx_http_naxsi_read_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
  ngx_http_naxsi_loc_conf_t *alcf = conf;

  ngx_str_t*                  value;
  ngx_http_rule_t             rule, *rule_r;

#ifdef _debug_readconf
  if (cf) {
    value = cf->args->elts;
    NX_LOG_DEBUG(
      _debug_readconf, NGX_LOG_EMERG, cf, 0, "TOP READ CONF %V %V", &(value[0]), &(value[1]));
  }
#endif
  if (!alcf || !cf) {
    return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
  }

  if (ngx_http_naxsi_push_loc_conf(cf, alcf) != NGX_OK)
    return NGX_CONF_ERROR;

  value   = cf->args->elts;

  if (!alcf->ignore_cidrs) {
    alcf->ignore_cidrs = ngx_array_create(cf->pool, 1, sizeof(cidr_t));
    if (!alcf->ignore_cidrs) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "CIDRs array alloc failed"); /* LCOV_EXCL_LINE */
      return (NGX_CONF_ERROR);                                              /* LCOV_EXCL_LINE */
    }
  }

  if (!alcf->ignore_ips) {
    alcf->ignore_ips = (ngx_hash_t*)ngx_pcalloc(cf->pool, sizeof(ngx_hash_t));
    if (!alcf->ignore_ips) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "IPs hashtable alloc failed"); /* LCOV_EXCL_LINE */
      return (NGX_CONF_ERROR);                                                /* LCOV_EXCL_LINE */
    }
    alcf->ignore_ips_ha.pool      = cf->pool;
    alcf->ignore_ips_ha.temp_pool = cf->temp_pool;
    if (ngx_hash_keys_array_init(&alcf->ignore_ips_ha, NGX_HASH_SMALL) != NGX_OK) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "IPs hash keys init failed"); /* LCOV_EXCL_LINE */
      return (NGX_CONF_ERROR);                                               /* LCOV_EXCL_LINE */
    }
  }

  /*
  ** if it's a basic rule
  */
  if (!ngx_strcmp(value[0].data, TOP_BASIC_RULE_T) ||
      !ngx_strcmp(value[0].data, TOP_BASIC_RULE_N)) {
    memset(&rule, 0, sizeof(ngx_http_rule_t));
    if (ngx_http_naxsi_cfg_parse_one_rule(cf, value, &rule, cf->args->nelts) != NGX_CONF_OK) {
      /* LCOV_EXCL_START */
      ngx_http_naxsi_line_conf_error(cf, value);
      return (NGX_CONF_ERROR);
      /* LCOV_EXCL_STOP */
    }
    /* push in whitelist rules, as it have a whitelist ID array */
    if (rule.wlid_array && rule.wlid_array->nelts > 0) {
      if (alcf->whitelist_rules == NULL) {
        alcf->whitelist_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
        if (alcf->whitelist_rules == NULL) {
          return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
        }
      }
      rule_r = ngx_array_push(alcf->whitelist_rules);
      if (!rule_r) {
        return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
      }
      memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
    }
    /* else push in appropriate ruleset : it's a normal rule */
    else {
      if (rule.br->headers || rule.br->headers_var) {
        if (alcf->header_rules == NULL) {
          alcf->header_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
          if (alcf->header_rules == NULL)
            return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
        }
        rule_r = ngx_array_push(alcf->header_rules);
        if (!rule_r)
          return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
        memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
      }
      /* push in body match rules (PATCH/POST/PUT) */
      if (rule.br->body || rule.br->body_var) {
        if (alcf->body_rules == NULL) {
          alcf->body_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
          if (alcf->body_rules == NULL)
            return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
        }
        rule_r = ngx_array_push(alcf->body_rules);
        if (!rule_r)
          return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
        memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
      }
      /* push in raw body match rules (PATCH/POST/PUT) */
      if (rule.br->raw_body) {
        NX_LOG_DEBUG(_debug_readconf,
                     NGX_LOG_EMERG,
                     cf,
                     0,
                     "pushing rule %d in (read conf) raw_body rules",
                     rule.rule_id);
        if (alcf->raw_body_rules == NULL) {
          alcf->raw_body_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
          if (alcf->raw_body_rules == NULL)
            return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
        }
        rule_r = ngx_array_push(alcf->raw_body_rules);
        if (!rule_r)
          return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
        memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
      }
      /* push in generic rules, as it's matching the URI */
      if (rule.br->url) {
        NX_LOG_DEBUG(
          _debug_readconf, NGX_LOG_EMERG, cf, 0, "pushing rule %d in generic rules", rule.rule_id);
        if (alcf->generic_rules == NULL) {
          alcf->generic_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
          if (alcf->generic_rules == NULL)
            return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
        }
        rule_r = ngx_array_push(alcf->generic_rules);
        if (!rule_r)
          return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
        memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
      }
      /* push in GET arg rules, but we should push in POST rules too  */
      if (rule.br->args_var || rule.br->args) {
        NX_LOG_DEBUG(
          _debug_readconf, NGX_LOG_EMERG, cf, 0, "pushing rule %d in GET rules", rule.rule_id);
        if (alcf->get_rules == NULL) {
          alcf->get_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
          if (alcf->get_rules == NULL)
            return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
        }
        rule_r = ngx_array_push(alcf->get_rules);
        if (!rule_r)
          return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
        memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
      }
    }
    return (NGX_CONF_OK);
  } else if (!ngx_strcmp(value[0].data, TOP_IGNORE_IP_T) ||
             !ngx_strcmp(value[0].data, TOP_IGNORE_IP_N)) {

    char ip_str[INET6_ADDRSTRLEN] = { 0 };
    if (!naxsi_parse_ip(&value[1], NULL, ip_str)) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid IP"); /* LCOV_EXCL_LINE */
      return (NGX_CONF_ERROR);                                /* LCOV_EXCL_LINE */
    }
    ngx_str_t key = { .data = NULL, .len = strlen(ip_str) };
    key.data      = (unsigned char*)ngx_pcalloc(cf->pool, key.len);
    if (!key.data) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "cannot allocate memory"); /* LCOV_EXCL_LINE */
      return (NGX_CONF_ERROR);                                            /* LCOV_EXCL_LINE */
    }
    memcpy(key.data, ip_str, key.len);

    if (ngx_hash_add_key(&alcf->ignore_ips_ha, &key, (void*)1234, NGX_HASH_READONLY_KEY) !=
        NGX_OK) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "cannot add hash value"); /* LCOV_EXCL_LINE */
      return (NGX_CONF_ERROR);                                           /* LCOV_EXCL_LINE */
    }
    return (NGX_CONF_OK); /* LCOV_EXCL_LINE */
  } else if (!ngx_strcmp(value[0].data, TOP_IGNORE_CIDR_T) ||
             !ngx_strcmp(value[0].data, TOP_IGNORE_CIDR_N)) {

    char* smask   = NULL;
    int   is_ipv6 = strnchr((const char*)value[1].data, ':', value[1].len) != NULL;
    if ((!is_ipv6 && NULL != (smask = cstrfaststr(value[1].data, value[1].len, "/32"))) ||
        (is_ipv6 && NULL != (smask = cstrfaststr(value[1].data, value[1].len, "/128")))) {

      // add it directly to IgnoreIP list
      char   ip_str[INET6_ADDRSTRLEN] = { 0 };
      size_t orig_len                 = value[1].len;

      value[1].len = smask - (const char*)value[1].data;
      int ret      = naxsi_parse_ip(&value[1], NULL, ip_str);
      value[1].len = orig_len;
      if (!ret) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid IP in CIDR"); /* LCOV_EXCL_LINE */
        return (NGX_CONF_ERROR);                                        /* LCOV_EXCL_LINE */
      }

      ngx_str_t key = { .data = NULL, .len = strlen(ip_str) };
      key.data      = (unsigned char*)ngx_pcalloc(cf->pool, key.len);
      if (!key.data) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "cannot allocate memory"); /* LCOV_EXCL_LINE */
        return (NGX_CONF_ERROR);                                            /* LCOV_EXCL_LINE */
      }
      memcpy(key.data, ip_str, key.len);

      if (ngx_hash_add_key(&alcf->ignore_ips_ha, &key, (void*)1234, NGX_HASH_READONLY_KEY) !=
          NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "cannot add hash value"); /* LCOV_EXCL_LINE */
        return (NGX_CONF_ERROR);                                           /* LCOV_EXCL_LINE */
      }
      return (NGX_CONF_OK); /* LCOV_EXCL_LINE */
    }

    cidr_t cidr = cidr_zero;
    int    err  = naxsi_parse_cidr(&value[1], &cidr);
    switch (err) {
      case CIDR_OK:
        break;
      default:
        /* fall-thru */
      case CIDR_ERROR_MISSING_MASK:
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "missing CIDR mask"); /* LCOV_EXCL_LINE */
        return (NGX_CONF_ERROR);                                       /* LCOV_EXCL_LINE */
      case CIDR_ERROR_INVALID_IP_NET:
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid CIDR net"); /* LCOV_EXCL_LINE */
        return (NGX_CONF_ERROR);                                      /* LCOV_EXCL_LINE */
      case CIDR_ERROR_INVALID_CIDR_MASK:
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid CIDR mask"); /* LCOV_EXCL_LINE */
        return (NGX_CONF_ERROR);                                       /* LCOV_EXCL_LINE */
    }

    cidr_t* tmp = (cidr_t*)ngx_array_push(alcf->ignore_cidrs);
    if (!tmp) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "cannot allocate array value"); /* LCOV_EXCL_LINE */
      return (NGX_CONF_ERROR);                                                 /* LCOV_EXCL_LINE */
    }
    *tmp = cidr;
    return (NGX_CONF_OK);
  }
  ngx_http_naxsi_line_conf_error(cf, value);
  return (NGX_CONF_ERROR);
}

static char*
ngx_http_naxsi_cr_loc_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{

  ngx_http_naxsi_loc_conf_t * alcf = conf;
  ngx_str_t*                  value;
  ngx_http_check_rule_t*      rule_c;
  unsigned int                i;
  u_char*                     var_end;

  if (!alcf || !cf)
    return (NGX_CONF_ERROR);

  if (ngx_http_naxsi_push_loc_conf(cf, alcf) != NGX_OK)
    return NGX_CONF_ERROR;

  value   = cf->args->elts;

  if (ngx_strcmp(value[0].data, TOP_CHECK_RULE_T) && ngx_strcmp(value[0].data, TOP_CHECK_RULE_N))
    return (NGX_CONF_ERROR);

  /* #ifdef _debug_readconf */
  /*   ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,  */
  /* 		     "pushing rule %d in check rules", rule.rule_id);   */
  /* #endif */

  i = 0;
  if (!alcf->check_rules)
    alcf->check_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_check_rule_t));
  if (!alcf->check_rules)
    return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
  rule_c = ngx_array_push(alcf->check_rules);
  if (!rule_c)
    return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
  memset(rule_c, 0, sizeof(ngx_http_check_rule_t));
  /* process the first word : score rule */
  if (value[1].data[i] == '$') {
    var_end = (u_char*)ngx_strchr((value[1].data) + i, ' ');
    if (!var_end) { /* LCOV_EXCL_START */
      ngx_http_naxsi_line_conf_error(cf, value);
      return (NGX_CONF_ERROR);
      /* LCOV_EXCL_STOP */
    }
    rule_c->sc_tag.len  = var_end - value[1].data;
    rule_c->sc_tag.data = ngx_pcalloc(cf->pool, rule_c->sc_tag.len + 1);
    if (!rule_c->sc_tag.data)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    memcpy(rule_c->sc_tag.data, value[1].data, rule_c->sc_tag.len);
    i += rule_c->sc_tag.len + 1;
  } else {
    /* LCOV_EXCL_START */
    ngx_http_naxsi_line_conf_error(cf, value);
    return (NGX_CONF_ERROR);
    /* LCOV_EXCL_STOP */
  }
  // move to next word
  while (value[1].data[i] && value[1].data[i] == ' ')
    i++;
  // get the comparison type
  if (value[1].data[i] == '>' && value[1].data[i + 1] == '=')
    rule_c->cmp = SUP_OR_EQUAL;
  else if (value[1].data[i] == '>' && value[1].data[i + 1] != '=')
    rule_c->cmp = SUP;
  else if (value[1].data[i] == '<' && value[1].data[i + 1] == '=')
    rule_c->cmp = INF_OR_EQUAL;
  else if (value[1].data[i] == '<' && value[1].data[i + 1] != '=')
    rule_c->cmp = INF;
  else {
    ngx_http_naxsi_line_conf_error(cf, value);
    return (NGX_CONF_ERROR);
  }
  // move to next word
  while (value[1].data[i] && !(value[1].data[i] >= '0' && value[1].data[i] <= '9') &&
         (value[1].data[i] != '-'))
    i++;
  NX_LOG_DEBUG(_debug_readconf,
               NGX_LOG_EMERG,
               cf,
               0,
               "XX-special score in checkrule:%s from (%d)",
               value[1].data,
               atoi((const char*)value[1].data + i));
  // get the score
  rule_c->sc_score = atoi((const char*)(value[1].data + i));
  /* process the second word : Action rule */
  if (ngx_strstr(value[2].data, "BLOCK"))
    rule_c->block = 1;
  else if (ngx_strstr(value[2].data, "ALLOW"))
    rule_c->allow = 1;
  else if (ngx_strstr(value[2].data, "LOG"))
    rule_c->log = 1;
  else if (ngx_strstr(value[2].data, "DROP"))
    rule_c->drop = 1;
  else {
    /* LCOV_EXCL_START */
    ngx_http_naxsi_line_conf_error(cf, value);
    return (NGX_CONF_ERROR);
    /* LCOV_EXCL_STOP */
  }
  return (NGX_CONF_OK);
}

/*
** URL denied
*/
static char*
ngx_http_naxsi_ud_loc_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
  ngx_http_naxsi_loc_conf_t * alcf = conf;
  ngx_str_t*                  value;

  if (!alcf || !cf)
    return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */

  if (ngx_http_naxsi_push_loc_conf(cf, alcf) != NGX_OK)
    return NGX_CONF_ERROR;

  value   = cf->args->elts;

  /* store denied URL for location */
  if ((!ngx_strcmp(value[0].data, TOP_DENIED_URL_N) ||
       !ngx_strcmp(value[0].data, TOP_DENIED_URL_T)) &&
      value[1].len) {
    alcf->denied_url = ngx_pcalloc(cf->pool, sizeof(ngx_str_t));
    if (!alcf->denied_url)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    alcf->denied_url->data = ngx_pcalloc(cf->pool, value[1].len + 1);
    if (!alcf->denied_url->data)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    memcpy(alcf->denied_url->data, value[1].data, value[1].len);
    alcf->denied_url->len = value[1].len;
    return (NGX_CONF_OK);
  }

  return NGX_CONF_ERROR;
}

/*
** handle flags that can be set/modified at runtime
*/
static char*
ngx_http_naxsi_flags_loc_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
  ngx_http_naxsi_loc_conf_t * alcf = conf;
  ngx_str_t*                  value;

  if (!alcf || !cf)
    return (NGX_CONF_ERROR);

  if (ngx_http_naxsi_push_loc_conf(cf, alcf) != NGX_OK)
    return NGX_CONF_ERROR;

  value   = cf->args->elts;

  /* it's a flagrule, just a hack to enable/disable mod */
  if (!ngx_strcmp(value[0].data, TOP_ENABLED_FLAG_T) ||
      !ngx_strcmp(value[0].data, TOP_ENABLED_FLAG_N)) {
    alcf->enabled = 1;
    return (NGX_CONF_OK);
  } else
    /* it's a flagrule, just a hack to enable/disable mod */
    if (!ngx_strcmp(value[0].data, TOP_DISABLED_FLAG_T) ||
        !ngx_strcmp(value[0].data, TOP_DISABLED_FLAG_N)) {
      alcf->force_disabled = 1;
      return (NGX_CONF_OK);
    } else
      /* it's a flagrule, currently just a hack to enable/disable learning mode */
      if (!ngx_strcmp(value[0].data, TOP_LEARNING_FLAG_T) ||
          !ngx_strcmp(value[0].data, TOP_LEARNING_FLAG_N)) {
        alcf->learning = 1;
        return (NGX_CONF_OK);
      } else if (!ngx_strcmp(value[0].data, TOP_LIBINJECTION_SQL_T) ||
                 !ngx_strcmp(value[0].data, TOP_LIBINJECTION_SQL_N)) {
        NX_LOG_DEBUG(_debug_loc_conf, NGX_LOG_EMERG, cf, 0, "LibInjectionSql enabled");
        alcf->libinjection_sql_enabled = 1;
        return (NGX_CONF_OK);
      } else if (!ngx_strcmp(value[0].data, TOP_LIBINJECTION_XSS_T) ||
                 !ngx_strcmp(value[0].data, TOP_LIBINJECTION_XSS_N)) {
        alcf->libinjection_xss_enabled = 1;
        NX_LOG_DEBUG(_debug_loc_conf, NGX_LOG_EMERG, cf, 0, "LibInjectionXss enabled");
        return (NGX_CONF_OK);
      } else
        return (NGX_CONF_ERROR);
}

static char*
ngx_http_naxsi_read_main_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
  ngx_http_naxsi_main_conf_t* alcf = conf;
  ngx_str_t*                  value;
  ngx_http_rule_t             rule, *rule_r;

  if (!alcf || !cf)
    return (NGX_CONF_ERROR); /* alloc a new rule */

  value = cf->args->elts;
  /* parse the line, fill rule struct  */

  NX_LOG_DEBUG(_debug_main_conf, NGX_LOG_EMERG, cf, 0, "XX-TOP READ CONF %s", value[0].data);
  if (ngx_strcmp(value[0].data, TOP_MAIN_BASIC_RULE_T) &&
      ngx_strcmp(value[0].data, TOP_MAIN_BASIC_RULE_N)) {
    ngx_http_naxsi_line_conf_error(cf, value);
    return (NGX_CONF_ERROR);
  }
  memset(&rule, 0, sizeof(ngx_http_rule_t));

  if (ngx_http_naxsi_cfg_parse_one_rule(cf /*, alcf*/, value, &rule, cf->args->nelts) !=
      NGX_CONF_OK) {
    /* LCOV_EXCL_START */
    ngx_http_naxsi_line_conf_error(cf, value);
    return (NGX_CONF_ERROR);
    /* LCOV_EXCL_STOP */
  }

  if (rule.br->headers || rule.br->headers_var) {
    NX_LOG_DEBUG(
      _debug_main_conf, NGX_LOG_EMERG, cf, 0, "pushing rule %d in header rules", rule.rule_id);
    if (alcf->header_rules == NULL) {
      alcf->header_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
      if (alcf->header_rules == NULL)
        return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
    }
    rule_r = ngx_array_push(alcf->header_rules);
    if (!rule_r)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
  }
  /* push in body match rules (PATCH/POST/PUT) */
  if (rule.br->body || rule.br->body_var) {
    NX_LOG_DEBUG(
      _debug_main_conf, NGX_LOG_EMERG, cf, 0, "pushing rule %d in body rules", rule.rule_id);
    if (alcf->body_rules == NULL) {
      alcf->body_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
      if (alcf->body_rules == NULL)
        return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
    }
    rule_r = ngx_array_push(alcf->body_rules);
    if (!rule_r)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
  }
  /* push in raw body match rules (PATCH/POST/PUT) xx*/
  if (rule.br->raw_body) {
    NX_LOG_DEBUG(_debug_main_conf,
                 NGX_LOG_EMERG,
                 cf,
                 0,
                 "pushing rule %d in raw (main) body rules",
                 rule.rule_id);
    if (alcf->raw_body_rules == NULL) {
      alcf->raw_body_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
      if (alcf->raw_body_rules == NULL)
        return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
    }
    rule_r = ngx_array_push(alcf->raw_body_rules);
    if (!rule_r)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
  }
  /* push in generic rules, as it's matching the URI */
  if (rule.br->url) {
    NX_LOG_DEBUG(
      _debug_main_conf, NGX_LOG_EMERG, cf, 0, "pushing rule %d in generic rules", rule.rule_id);
    if (alcf->generic_rules == NULL) {
      alcf->generic_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
      if (alcf->generic_rules == NULL)
        return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
    }
    rule_r = ngx_array_push(alcf->generic_rules);
    if (!rule_r)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
  }
  /* push in GET arg rules, but we should push in POST rules too  */
  if (rule.br->args_var || rule.br->args) {
    NX_LOG_DEBUG(
      _debug_main_conf, NGX_LOG_EMERG, cf, 0, "pushing rule %d in GET rules", rule.rule_id);
    if (alcf->get_rules == NULL) {
      alcf->get_rules = ngx_array_create(cf->pool, 2, sizeof(ngx_http_rule_t));
      if (alcf->get_rules == NULL)
        return NGX_CONF_ERROR; /* LCOV_EXCL_LINE */
    }
    rule_r = ngx_array_push(alcf->get_rules);
    if (!rule_r)
      return (NGX_CONF_ERROR); /* LCOV_EXCL_LINE */
    memcpy(rule_r, &rule, sizeof(ngx_http_rule_t));
  }
  return (NGX_CONF_OK);
}

/*
** [ENTRY POINT] does : this is the function called by nginx :
** - Set up the context for the request
** - Check if the job is done and we're called again
** - if it's a PATCH/POST/PUT request, setup hook for body dataz
** - call dummy_data_parse
** - check our context struct (with scores & stuff) against custom check rules
** - check if the request should be denied
*/
static ngx_int_t
ngx_http_naxsi_access_handler(ngx_http_request_t* r)
{
  ngx_http_request_ctx_t*    ctx;
  ngx_int_t                  rc;
  ngx_http_naxsi_loc_conf_t* cf;
  struct tms                 tmsstart, tmsend;
  clock_t                    start, end;
  ngx_http_variable_value_t* lookup;

  static ngx_str_t learning_flag         = ngx_string(RT_LEARNING);
  static ngx_str_t enable_flag           = ngx_string(RT_ENABLE);
  static ngx_str_t post_action_flag      = ngx_string(RT_POST_ACTION);
  static ngx_str_t extensive_log_flag    = ngx_string(RT_EXTENSIVE_LOG);
  static ngx_str_t json_log_flag         = ngx_string(RT_JSON_LOG);
  static ngx_str_t libinjection_sql_flag = ngx_string(RT_LIBINJECTION_SQL);
  static ngx_str_t libinjection_xss_flag = ngx_string(RT_LIBINJECTION_XSS);

  ctx = ngx_http_get_module_ctx(r, ngx_http_naxsi_module);
  cf  = ngx_http_get_module_loc_conf(r, ngx_http_naxsi_module);

  if (ctx && ctx->over)
    return (NGX_DECLINED);
  if (ctx && ctx->wait_for_body) {
    NX_DEBUG(_debug_mechanics, NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "naxsi:NGX_AGAIN");
    return (NGX_DONE);
  }
  if (!cf)
    return (NGX_ERROR);
  /* the module is not enabled here */
  /* if enable directive is not present at all in the location,
     don't try to do dynamic lookup for "live" enabled
     naxsi, this would be very rude. */
  if (!cf->enabled)
    return (NGX_DECLINED);
  /* On the other hand, if naxsi has been explicitly disabled
     in this location (using naxsi directive), user is probably
     trying to do something.  */
  if (cf->force_disabled) {
    /* Look if the user did not try to enable naxsi dynamically */
    lookup = ngx_http_get_variable(r, &enable_flag, cf->flag_enable_h);
    if (lookup && !lookup->not_found && lookup->len > 0) {
      ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                    r->connection->log,
                    0,
                    "live enable is present %d",
                    lookup->data[0] - '0');
      if (lookup->data[0] - '0' != 1) {
        return (NGX_DECLINED);
      }
    } else
      return (NGX_DECLINED);
  }
  /* don't process internal requests. */
  if (r->internal) {
    NX_DEBUG(_debug_mechanics,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-DON'T PROCESS (%V)|CTX:%p|ARGS:%V|METHOD=%s|INTERNAL:%d",
             &(r->uri),
             ctx,
             &(r->args),
             r->method == NGX_HTTP_PATCH  ? "PATCH"
             : r->method == NGX_HTTP_POST ? "POST"
             : r->method == NGX_HTTP_PUT  ? "PUT"
             : r->method == NGX_HTTP_GET  ? "GET"
                                          : "UNKNOWN!!",
             r->internal);
    return (NGX_DECLINED);
  }
  NX_DEBUG(_debug_mechanics,
           NGX_LOG_DEBUG_HTTP,
           r->connection->log,
           0,
           "XX-processing (%V)|CTX:%p|ARGS:%V|METHOD=%s|INTERNAL:%d",
           &(r->uri),
           ctx,
           &(r->args),
           r->method == NGX_HTTP_PATCH  ? "PATCH"
           : r->method == NGX_HTTP_POST ? "POST"
           : r->method == NGX_HTTP_PUT  ? "PUT"
           : r->method == NGX_HTTP_GET  ? "GET"
                                        : "UNKNOWN!!",
           r->internal);
  if (!ctx) {
    ngx_pool_cleanup_t* cln;
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_request_ctx_t));
    if (ctx == NULL)
      return NGX_ERROR;
    cln = ngx_pool_cleanup_add(r->pool, 0);
    if (cln == NULL) {
      return NGX_ERROR;
    }

    cln->handler = ngx_http_module_cleanup_handler;
    cln->data    = ctx;

    naxsi_generate_request_id(ctx->request_id);
    ngx_http_set_ctx(r, ctx, ngx_http_naxsi_module);
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : orig learning : %d",
             cf->learning ? 1 : 0);
    /* it seems that nginx will - in some cases -
     have a variable with empty content but with lookup->not_found set to 0,
    so check len as well */
    ctx->learning = cf->learning;

    lookup = ngx_http_get_variable(r, &learning_flag, cf->flag_learning_h);
    if (lookup && !lookup->not_found && lookup->len > 0) {

      ctx->learning = lookup->data[0] - '0';
      NX_DEBUG(_debug_modifier,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : override learning : %d (raw=%d)",
               ctx->learning ? 1 : 0,
               lookup->len);
    }

    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : [final] learning : %d",
             ctx->learning ? 1 : 0);

    ctx->enabled = cf->enabled;
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : orig enabled : %d",
             ctx->enabled ? 1 : 0);

    lookup = ngx_http_get_variable(r, &enable_flag, cf->flag_enable_h);
    if (lookup && !lookup->not_found && lookup->len > 0) {
      ctx->enabled = lookup->data[0] - '0';
      NX_DEBUG(_debug_modifier,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : override enable : %d",
               ctx->enabled ? 1 : 0);
    }
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : [final] enabled : %d",
             ctx->enabled ? 1 : 0);

    /*
    ** LIBINJECTION_SQL
    */
    ctx->libinjection_sql = cf->libinjection_sql_enabled;

    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : orig libinjection_sql : %d",
             ctx->libinjection_sql ? 1 : 0);

    lookup = ngx_http_get_variable(r, &libinjection_sql_flag, cf->flag_libinjection_sql_h);

    if (lookup && !lookup->not_found && lookup->len > 0) {
      ctx->libinjection_sql = lookup->data[0] - '0';
      NX_DEBUG(_debug_modifier,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : override libinjection_sql : %d",
               ctx->libinjection_sql ? 1 : 0);
    }
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : [final] libinjection_sql : %d",
             ctx->libinjection_sql ? 1 : 0);

    /*
    ** LIBINJECTION_XSS
    */
    ctx->libinjection_xss = cf->libinjection_xss_enabled;

    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : orig libinjection_xss : %d",
             ctx->libinjection_xss ? 1 : 0);

    lookup = ngx_http_get_variable(r, &libinjection_xss_flag, cf->flag_libinjection_xss_h);
    if (lookup && !lookup->not_found && lookup->len > 0) {
      ctx->libinjection_xss = lookup->data[0] - '0';
      NX_DEBUG(_debug_modifier,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : override libinjection_xss : %d",
               ctx->libinjection_xss ? 1 : 0);
    }
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : [final] libinjection_xss : %d",
             ctx->libinjection_xss ? 1 : 0);

    /* post_action is off by default. */
    ctx->post_action = 0;
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : orig post_action : %d",
             ctx->post_action ? 1 : 0);

    lookup = ngx_http_get_variable(r, &post_action_flag, cf->flag_post_action_h);
    if (lookup && !lookup->not_found && lookup->len > 0) {
      ctx->post_action = lookup->data[0] - '0';
      NX_DEBUG(_debug_modifier,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : override post_action : %d",
               ctx->post_action ? 1 : 0);
    }
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : [final] post_action : %d",
             ctx->post_action ? 1 : 0);

    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : orig extensive_log : %d",
             ctx->extensive_log ? 1 : 0);

    lookup = ngx_http_get_variable(r, &extensive_log_flag, cf->flag_extensive_log_h);
    if (lookup && !lookup->not_found && lookup->len > 0) {
      ctx->extensive_log = lookup->data[0] - '0';
      NX_DEBUG(_debug_modifier,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : override extensive_log : %d",
               ctx->extensive_log ? 1 : 0);
    }
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : [final] extensive_log : %d",
             ctx->extensive_log ? 1 : 0);

    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : orig json_log : %d",
             ctx->json_log ? 1 : 0);

    lookup = ngx_http_get_variable(r, &json_log_flag, cf->flag_json_log_h);
    if (lookup && !lookup->not_found && lookup->len > 0) {
      ctx->json_log = lookup->data[0] - '0';
      NX_DEBUG(_debug_modifier,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : override json_log : %d",
               ctx->json_log ? 1 : 0);
    }
    NX_DEBUG(_debug_modifier,
             NGX_LOG_DEBUG_HTTP,
             r->connection->log,
             0,
             "XX-dummy : [final] json_log : %d",
             ctx->json_log ? 1 : 0);

    /* the module is not enabled here */
    if (!ctx->enabled)
      return (NGX_DECLINED);

    if ((r->method == NGX_HTTP_PATCH || r->method == NGX_HTTP_POST || r->method == NGX_HTTP_PUT) &&
        !ctx->ready) {
      NX_DEBUG(_debug_mechanics,
               NGX_LOG_DEBUG_HTTP,
               r->connection->log,
               0,
               "XX-dummy : body_request : before !");

      rc = ngx_http_read_client_request_body(r, ngx_http_naxsi_payload_handler);
      /* this might happen quite often, especially with big files /
      ** low network speed. our handler is called when headers are read,
      ** but, often, the full body request hasn't yet, so
      ** read client request body will return ngx_again. Then we need
      ** to return ngx_done, wait for our handler to be called once
      ** body request arrived, and let him call core_run_phases
      ** to be able to process the request.
      */
      if (rc == NGX_AGAIN) {
        ctx->wait_for_body = 1;
        NX_DEBUG(_debug_mechanics,
                 NGX_LOG_DEBUG_HTTP,
                 r->connection->log,
                 0,
                 "XX-dummy : body_request : NGX_AGAIN !");

        return (NGX_DONE);
      } else if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        /*
        ** might happen but never saw it, let the debug print.
        */
        ngx_log_debug(
          NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "XX-dummy : SPECIAL RESPONSE !!!!");
        return rc;
      }
    } else
      ctx->ready = 1;
  }
  if (ctx && ctx->ready && !ctx->over) {

    if ((start = times(&tmsstart)) == (clock_t)-1) {
      ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "XX-dummy : Failed to get time");
    }

    ngx_http_naxsi_data_parse(ctx, r);
    cf->request_processed++;
    if ((end = times(&tmsend)) == (clock_t)-1) {
      ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "XX-dummy : Failed to get time");
    }

    if (end - start > 10) {
      ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                    r->connection->log,
                    0,
                    "[MORE THAN 10MS] times : start:%l end:%l diff:%l",
                    start,
                    end,
                    (end - start));
    }

    ctx->over = 1;
    if (ctx->block || ctx->drop) {
      cf->request_blocked++;
      rc = ngx_http_output_forbidden_page(ctx, r);
      // nothing:  return (NGX_OK);
      // redirect: return (NGX_HTTP_OK);
      return rc;
    } else if (ctx->log) {
      rc = ngx_http_output_forbidden_page(ctx, r);
    }
  }
  NX_DEBUG(_debug_mechanics, NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "NGX_FINISHED !");

  return NGX_DECLINED;
}

static ngx_http_request_ctx_t*
recover_request_ctx(ngx_http_request_t* r)
{
  ngx_http_request_ctx_t* ctx;
  ngx_pool_cleanup_t*     cln;
  ctx = ngx_http_get_module_ctx(r, ngx_http_naxsi_module);
  if (ctx == NULL && (r->internal || r->filter_finalize)) {
    for (cln = r->pool->cleanup; cln; cln = cln->next) {
      if (cln->handler == ngx_http_module_cleanup_handler) {
        ctx = cln->data;
        break;
      }
    }
  }
  return ctx;
}

static void
ngx_http_module_cleanup_handler(void* data)
{
  return;
}

static ngx_int_t
ngx_http_naxsi_add_variables(ngx_conf_t* cf)
{
  ngx_http_variable_t *var, *v;

  for (v = ngx_http_naxsi_variables; v->name.len; v++) {
    var = ngx_http_add_variable(cf, &v->name, v->flags);
    if (var == NULL) {
      return NGX_ERROR;
    }
    var->get_handler = v->get_handler;
    var->data        = v->data;
  }
  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_server_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data)
{
  v->data         = r->headers_in.server.data;
  v->len          = r->headers_in.server.len;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;
  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_uri_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data)
{
  ngx_http_request_ctx_t* ctx = recover_request_ctx(r);
  if (!ctx) {
    v->not_found = 1;
    return NGX_OK;
  }

  ngx_str_t* tmp_uri = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
  if (!tmp_uri) {
    return (NGX_ERROR);
  }
  tmp_uri->len  = r->uri.len + (2 * ngx_escape_uri(NULL, r->uri.data, r->uri.len, NGX_ESCAPE_ARGS));
  tmp_uri->data = ngx_pcalloc(r->pool, tmp_uri->len + 1);
  if (!tmp_uri->data) {
    return (NGX_ERROR);
  }
  ngx_escape_uri(tmp_uri->data, r->uri.data, r->uri.len, NGX_ESCAPE_ARGS);

  v->data         = tmp_uri->data;
  v->len          = tmp_uri->len;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;
  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_learning_variable(ngx_http_request_t*        r,
                                 ngx_http_variable_value_t* v,
                                 uintptr_t                  data)
{
  ngx_http_request_ctx_t* ctx = recover_request_ctx(r);
  if (!ctx) {
    v->not_found = 1;
    return NGX_OK;
  }

  v->data = ngx_palloc(r->pool, 1);
  if (v->data == NULL) {
    return NGX_ERROR;
  }
  v->data[0]      = ctx->learning ? '1' : '0';
  v->len          = 1;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;

  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_block_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data)
{
  ngx_http_request_ctx_t* ctx = recover_request_ctx(r);
  if (!ctx) {
    v->not_found = 1;
    return NGX_OK;
  }

  v->data = ngx_palloc(r->pool, 1);
  if (v->data == NULL) {
    return NGX_ERROR;
  }
  v->data[0]      = ctx->block ? '1' : '0';
  v->len          = 1;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;

  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_total_processed_variable(ngx_http_request_t*        r,
                                        ngx_http_variable_value_t* v,
                                        uintptr_t                  data)
{
  ngx_http_naxsi_loc_conf_t* cf = ngx_http_get_module_loc_conf(r, ngx_http_naxsi_module);

  v->data = ngx_palloc(r->pool, NGX_INT32_LEN);
  if (v->data == NULL) {
    return NGX_ERROR;
  }
  v->len          = ngx_sprintf(v->data, "%z", cf->request_processed) - v->data;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;

  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_total_blocked_variable(ngx_http_request_t*        r,
                                      ngx_http_variable_value_t* v,
                                      uintptr_t                  data)
{
  ngx_http_naxsi_loc_conf_t* cf = ngx_http_get_module_loc_conf(r, ngx_http_naxsi_module);

  v->data = ngx_palloc(r->pool, NGX_INT32_LEN);
  if (v->data == NULL) {
    return NGX_ERROR;
  }
  v->len          = ngx_sprintf(v->data, "%z", cf->request_blocked) - v->data;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;

  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_score_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data)
{
  ngx_http_request_ctx_t* ctx = recover_request_ctx(r);
  if (!ctx) {
    v->not_found = 1;
    return NGX_OK;
  }

  const char*               fmt    = "%s:%d,"; /* cscore:score */
  ngx_http_matched_rule_t*  mr     = NULL;
  ngx_http_special_score_t* sc     = NULL;
  ngx_uint_t                others = 0, i = 0;
  size_t                    size = 0, written = 0;
  char*                     p = NULL;

  if (ctx->matched) {
    mr = ctx->matched->elts;
    for (i = 0; i < ctx->matched->nelts; i++) {
      if (mr[i].rule->rule_id < 1000) {
        others = 1;
        size += strlen("$INTERNAL,");
        break;
      }
    }
  }

  if (ctx->special_scores) {
    sc = ctx->special_scores->elts;
    for (i = 0; i < ctx->special_scores->nelts; i++) {
      if (sc[i].sc_score == 0) {
        continue;
      }
      size += snprintf(NULL, 0, fmt, sc[i].sc_tag->data, sc[i].sc_score);
    }
  }

  if (size < 1) {
    v->not_found = 1;
    return NGX_OK;
  }

  v->len  = size - 1; /* - last ',' */
  v->data = ngx_palloc(r->pool, size);
  if (v->data == NULL) {
    return NGX_ERROR;
  }
  p = (char*)v->data;

  if (others) {
    written = strlen("$INTERNAL,");
    memcpy(p, "$INTERNAL,", written + 1);
  }

  if (ctx->special_scores) {
    sc = ctx->special_scores->elts;
    for (i = 0; i < ctx->special_scores->nelts; i++) {
      if (sc[i].sc_score == 0) {
        continue;
      }

      int sub = snprintf(p + written, size - written, fmt, sc[i].sc_tag->data, sc[i].sc_score);
      if (sub < 0) {
        break;
      }
      written += sub;
    }
  }

  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;
  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_match_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data)
{
  ngx_http_request_ctx_t* ctx = recover_request_ctx(r);
  if (!ctx) {
    v->not_found = 1;
    return NGX_OK;
  }

  const char*              fmt     = "%d:%s%s:%s,"; /* rule_id:zone:var_name */
  ngx_http_matched_rule_t* mr      = NULL;
  ngx_uint_t               i       = 0;
  size_t                   size    = 0;
  size_t                   written = 0;
  char*                    p       = NULL;

  if (ctx->matched) {
    mr = ctx->matched->elts;
    for (i = 0; i < ctx->matched->nelts; i++) {
      const char* var_name = mr[i].name->len ? (const char*)mr[i].name->data : "-";
      ngx_uint_t  rule_id  = mr[i].rule->rule_id;
      /* FILE_EXT|NAME is the longest zone we may have */
      size += snprintf(NULL, 0, fmt, rule_id, "FILE_EXT", "|NAME", var_name);
    }
  }

  if (size < 1) {
    v->not_found = 1;
    return NGX_OK;
  }

  v->data = ngx_palloc(r->pool, size);
  if (v->data == NULL) {
    return NGX_ERROR;
  }

  p  = (char*)v->data;
  mr = ctx->matched->elts;
  for (i = 0; i < ctx->matched->nelts; i++) {
    const char* var_name = mr[i].name->len ? (const char*)mr[i].name->data : "-";
    const char* zone     = NULL;
    const char* name     = mr[i].target_name ? "|NAME" : "";
    ngx_uint_t  rule_id  = mr[i].rule->rule_id;

    if (mr[i].body_var) {
      zone = "BODY";
    } else if (mr[i].args_var) {
      zone = "ARGS";
    } else if (mr[i].headers_var) {
      zone = "HEADERS";
    } else if (mr[i].url) {
      zone = "URL";
    } else if (mr[i].file_ext) {
      zone = "FILE_EXT";
    } else {
      /* should never happen.. */
      continue;
    }

    int sub = snprintf(p + written, size - written, fmt, rule_id, zone, name, var_name);
    if (sub < 0) {
      break;
    }
    written += sub;
  }

  v->len          = written > 0 ? written - 1 : 0; /* - last ',' */
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;
  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_attack_family_variable(ngx_http_request_t*        r,
                                      ngx_http_variable_value_t* v,
                                      uintptr_t                  data)
{
  ngx_http_request_ctx_t*   ctx;
  ngx_pool_cleanup_t*       cln;
  ngx_http_special_score_t* sc;
  ngx_http_matched_rule_t*  mr;
  ngx_uint_t                i;
  size_t                    sz = 0;
  u_char*                   str;
  u_char*                   p;

  ctx = ngx_http_get_module_ctx(r, ngx_http_naxsi_module);

  if (ctx == NULL && (r->internal || r->filter_finalize)) {
    for (cln = r->pool->cleanup; cln; cln = cln->next) {
      if (cln->handler == ngx_http_module_cleanup_handler) {
        ctx = cln->data;
        break;
      }
    }
  }

  if (!ctx) {
    v->not_found = 1;
    return NGX_OK;
  }

  ngx_uint_t others = 0;
  if (ctx->matched) {
    mr = ctx->matched->elts;
    for (i = 0; i < ctx->matched->nelts; i++) {
      if (mr[i].rule->rule_id < 1000) {
        others = 1;
        sz     = strlen("$INTERNAL,");
        break;
      }
    }
  }

  if (ctx->special_scores) {
    sc = ctx->special_scores->elts;
    for (i = 0; i < ctx->special_scores->nelts; i++) {
      if (sc[i].sc_score != 0) {
        sz += sc[i].sc_tag->len + 1;
      }
    }
  }

  if (sz < 1) {
    v->not_found = 1;
    return NGX_OK;
  }

  str = (u_char*)ngx_pcalloc(r->pool, sz);
  if (str == NULL) {
    return NGX_ERROR;
  }
  p = str;

  if (others) {
    memcpy(p, "$INTERNAL,", sizeof("$INTERNAL,"));
    p += strlen("$INTERNAL,");
  }

  if (ctx->special_scores) {
    sc = ctx->special_scores->elts;
    for (i = 0; i < ctx->special_scores->nelts; i++) {
      if (sc[i].sc_score != 0) {
        memcpy(p, sc[i].sc_tag->data, sc[i].sc_tag->len);
        p += sc[i].sc_tag->len;
        *p = ',';
        p++;
      }
    }
  }

  v->data         = str;
  v->len          = sz - 1;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;

  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_attack_action_variable(ngx_http_request_t*        r,
                                      ngx_http_variable_value_t* v,
                                      uintptr_t                  data)
{
  ngx_http_request_ctx_t* ctx      = NULL;
  ngx_pool_cleanup_t*     cln      = NULL;
  size_t                  sz       = 0;
  u_char*                 str      = NULL;
  const char*             variable = NULL;
  // least significant bit represents if action is pass or block
  // second least significant bit represents if naxsi is in learning mode
  u_int learning_block_bits = 0;

  ctx = ngx_http_get_module_ctx(r, ngx_http_naxsi_module);

  if (ctx == NULL && (r->internal || r->filter_finalize)) {
    for (cln = r->pool->cleanup; cln; cln = cln->next) {
      if (cln->handler == ngx_http_module_cleanup_handler) {
        ctx = cln->data;
        break;
      }
    }
  }

  if (!ctx) {
    v->not_found = 1;
    return NGX_OK;
  }

  learning_block_bits = ((ctx->learning ? 1 : 0) << 1) | (ctx->block ? 1 : 0);

  switch (learning_block_bits) {
    case 0: // pass
      variable = "$PASS";
      break;
    case 1: // block
      variable = "$BLOCK";
      break;
    case 2: // learning pass
      variable = "$LEARNING-PASS";
      break;
    case 3: // learning block
      variable = "$LEARNING-BLOCK";
      break;
    default:
      break;
  }

  if (!variable) {
    v->not_found = 1;
    return NGX_OK;
  }

  sz  = strlen(variable);
  str = (u_char*)ngx_pcalloc(r->pool, sz);
  if (str == NULL) {
    return NGX_ERROR;
  }
  memcpy(str, variable, sz);

  v->data         = str;
  v->len          = sz;
  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;
  return NGX_OK;
}

static ngx_int_t
ngx_http_naxsi_request_id(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data)
{
  u_char*                 id  = NULL;
  const size_t            len = NAXSI_REQUEST_ID_SIZE << 1;
  ngx_http_request_ctx_t* ctx = ngx_http_get_module_ctx(r, ngx_http_naxsi_module);

  if (ctx == NULL) {
    return NGX_ERROR;
  }

  id = ngx_pnalloc(r->pool, len);
  if (id == NULL) {
    return NGX_ERROR;
  }

  v->valid        = 1;
  v->no_cacheable = 0;
  v->not_found    = 0;
  v->len          = len;
  v->data         = id;

  ngx_hex_dump(id, ctx->request_id, NAXSI_REQUEST_ID_SIZE);
  return NGX_OK;
}
