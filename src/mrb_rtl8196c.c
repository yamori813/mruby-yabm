/*
** mrb_rtl8196c.c - RTL8196C class
**
** Copyright (c) Hiroki Mori 2018
**
** See Copyright Notice in LICENSE
*/

#include "mruby.h"
#include "mruby/data.h"
#include <mruby/string.h>
#include "mrb_rtl8196c.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

typedef struct {
  int model;
} mrb_rtl8196c_data;

static const struct mrb_data_type mrb_rtl8196c_data_type = {
  "mrb_rtl8196c_data", mrb_free,
};

static mrb_value mrb_rtl8196c_init(mrb_state *mrb, mrb_value self)
{
  mrb_rtl8196c_data *data;
  mrb_value model;

  data = (mrb_rtl8196c_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_rtl8196c_data_type;
  DATA_PTR(self) = NULL;

  mrb_get_args(mrb, "i", &model);
  data = (mrb_rtl8196c_data *)mrb_malloc(mrb, sizeof(mrb_rtl8196c_data));
  data->model = mrb_fixnum(model);
  DATA_PTR(self) = data;

  return self;
}

void print(char *);

static mrb_value mrb_rtl8196c_print(mrb_state *mrb, mrb_value self)
{
  mrb_value val;
  mrb_get_args(mrb, "S", &val);
  print(RSTRING_PTR(val));
  return mrb_nil_value();
}

int sys_now();

static mrb_value mrb_rtl8196c_count(mrb_state *mrb, mrb_value self)
{

  return mrb_fixnum_value(sys_now());
}

void rtl_udp_bind(int port);
int rtl_udp_recv(char *buf, int len);

static mrb_value mrb_rtl8196c_udpbind(mrb_state *mrb, mrb_value self)
{
  mrb_value port;
  mrb_get_args(mrb, "i", &port);
  rtl_udp_bind(mrb_fixnum(port));
  return mrb_nil_value();
}

static mrb_value mrb_rtl8196c_udprecv(mrb_state *mrb, mrb_value self)
{
mrb_value str;
char buff[1024];
int len;
  
  len = rtl_udp_recv(buff, sizeof(buff) - 1);
  if (len != 0) {
    buff[len] = 0;
    str = mrb_str_new_cstr(mrb, buff);
  } else {
    str = mrb_str_new_cstr(mrb, "");
  }
    
  return str;
}

int http_connect(int addr, int port, char *header);
int http_read(char *buf, int len);
void http_close();

int https_connect(char *host, int addr, int port, char *header);
int https_read(char *buf, int len);
void https_close();

static mrb_value mrb_rtl8196c_http(mrb_state *mrb, mrb_value self)
{
mrb_value str;
mrb_value addr, port, header;
char tmp[512];
int len;

  mrb_get_args(mrb, "iiS", &addr, &port, &header);
  str = mrb_str_new_cstr(mrb, "");
  if (http_connect(mrb_fixnum(addr), mrb_fixnum(port), RSTRING_PTR(header))) {
    while(1) {
      len = http_read(tmp, sizeof(tmp) - 1);
      if (len < 0)
        break;
      if (len != 0) {
        tmp[len] = '\0';
        mrb_str_cat2(mrb, str, tmp);
      }
    }
    http_close();
  }
  return str;
}

static mrb_value mrb_rtl8196c_https(mrb_state *mrb, mrb_value self)
{
mrb_value str;
mrb_value host, addr, port, header;
char tmp[512];
int len;

  mrb_get_args(mrb, "SiiS", &host, &addr, &port, &header);
  str = mrb_str_new_cstr(mrb, "");
  if (https_connect(RSTRING_PTR(host), mrb_fixnum(addr), mrb_fixnum(port), RSTRING_PTR(header))) {
    while(1) {
      len = https_read(tmp, sizeof(tmp) - 1);
      if (len < 0)
        break;
      if (len != 0) {
        tmp[len] = '\0';
        mrb_str_cat2(mrb, str, tmp);
      }
    }
    https_close();
  }
  return str;
}

int lookup(char *host, int *addr);

static mrb_value mrb_rtl8196c_lookup(mrb_state *mrb, mrb_value self)
{
mrb_value host;
int addr;

  mrb_get_args(mrb, "S", &host);
  lookup(RSTRING_PTR(host), &addr);

  return mrb_fixnum_value(addr);
}

void mrb_mruby_rtlbm_rtl8196c_gem_init(mrb_state *mrb)
{
  struct RClass *rtl8196c;
  rtl8196c = mrb_define_class(mrb, "RTL8196C", mrb->object_class);

  mrb_define_const(mrb, rtl8196c, "RTL8196C_GENERIC", mrb_fixnum_value(RTL8196C_GENERIC));
  mrb_define_const(mrb, rtl8196c, "RTL8196C_HOMESPOTCUBE", mrb_fixnum_value(RTL8196C_HOMESPOTCUBE));
  mrb_define_const(mrb, rtl8196c, "RTL8196C_BBR4HGV2", mrb_fixnum_value(RTL8196C_BBR4HGV2));
  mrb_define_const(mrb, rtl8196c, "RTL8196C_LANW300NR", mrb_fixnum_value(RTL8196C_LANW300NR));
  mrb_define_const(mrb, rtl8196c, "RTL8196C_MZKMF300N", mrb_fixnum_value(RTL8196C_MZKMF300N));

  mrb_define_method(mrb, rtl8196c, "initialize", mrb_rtl8196c_init, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "print", mrb_rtl8196c_print, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "count", mrb_rtl8196c_count, MRB_ARGS_NONE());
  mrb_define_method(mrb, rtl8196c, "udpbind", mrb_rtl8196c_udprecv, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "udprecv", mrb_rtl8196c_udprecv, MRB_ARGS_NONE());
  mrb_define_method(mrb, rtl8196c, "http", mrb_rtl8196c_http, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, rtl8196c, "https", mrb_rtl8196c_https, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, rtl8196c, "lookup", mrb_rtl8196c_lookup, MRB_ARGS_REQ(1));
  DONE;
}

void mrb_mruby_rtlbm_rtl8196c_gem_final(mrb_state *mrb)
{
}

