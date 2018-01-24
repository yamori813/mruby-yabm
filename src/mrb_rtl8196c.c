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
  char *str;
  int len;
} mrb_rtl8196c_data;

static const struct mrb_data_type mrb_rtl8196c_data_type = {
  "mrb_rtl8196c_data", mrb_free,
};

static mrb_value mrb_rtl8196c_init(mrb_state *mrb, mrb_value self)
{
  mrb_rtl8196c_data *data;
  char *str;
  int len;

  data = (mrb_rtl8196c_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_rtl8196c_data_type;
  DATA_PTR(self) = NULL;

  mrb_get_args(mrb, "s", &str, &len);
  data = (mrb_rtl8196c_data *)mrb_malloc(mrb, sizeof(mrb_rtl8196c_data));
  data->str = str;
  data->len = len;
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

extern char udpbuff[];

static mrb_value mrb_rtl8196c_udprecv(mrb_state *mrb, mrb_value self)
{
mrb_value str;
  
  str = mrb_str_new_cstr(mrb, udpbuff);
  udpbuff[0] = '\0';
  return str;
}

void mrb_mruby_rtlbm_rtl8196c_gem_init(mrb_state *mrb)
{
  struct RClass *rtl8196c;
  rtl8196c = mrb_define_class(mrb, "RTL8196C", mrb->object_class);
  mrb_define_method(mrb, rtl8196c, "initialize", mrb_rtl8196c_init, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "print", mrb_rtl8196c_print, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "count", mrb_rtl8196c_count, MRB_ARGS_NONE());
  mrb_define_method(mrb, rtl8196c, "udprecv", mrb_rtl8196c_udprecv, MRB_ARGS_NONE());
  DONE;
}

void mrb_mruby_rtlbm_rtl8196c_gem_final(mrb_state *mrb)
{
}

