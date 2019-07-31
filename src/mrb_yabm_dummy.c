/*
** mrb_yabm_dummy.c - Yet Another Bare Metal class
**
** Copyright (c) Hiroki Mori 2019
**
** See Copyright Notice in LICENSE
*/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/array.h"
#include "mruby/string.h"

#include "mrb_yabm.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

#if defined(YABM_DUMMY)

typedef struct {
  int arch;
  int start;
} mrb_yabm_data;

static const struct mrb_data_type mrb_yabm_data_type = {
  "mrb_yabm_data", mrb_free,
};

static uint32_t mrb_yabm_strtoip(mrb_state *mrb, mrb_value str)
{
  char *cstr;
  uint32_t ip;
  int pos, val;

  ip = 0;
  pos = 0;
  val = 0;

  cstr = mrb_str_to_cstr(mrb, str);
  while (*cstr) {
    if (*cstr == '.') {
      ip |= val << (8 * (3 - pos));
      val = 0;
      ++pos;
    } else {
      val *= 10;
      val += *cstr - '0';
    }
    ++cstr;
  }
  ip |= val;
  return ip;
}

static mrb_value mrb_yabm_iptostr(mrb_state *mrb, uint32_t ip)
{
  char addr[16];
  int i , val, c, div;

  c = 0;
  for(i = 0; i < 4; ++i ) {
    val = (ip >> (8 * (3 - i))) & 0xff;
    div = val / 100;
    if (div != 0) {
      addr[c] = '0' + div;
      val -= 100 * div;
      ++c;
    }
    div = val / 10;
    if (div != 0) {
      addr[c] = '0' + div;
      val -= 10 * div;
      ++c;
    }
    div = val % 10;
    addr[c] = '0' + div;
    ++c;
    if(i != 3) {
      addr[c] = '.';
      ++c;
    }
   }
   addr[c] = '\0';
   return mrb_str_new_cstr(mrb, addr);
}

static mrb_value mrb_yabm_init(mrb_state *mrb, mrb_value self)
{
  mrb_yabm_data *data;
  struct timeval time_now;

  data = (mrb_yabm_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_yabm_data_type;
  DATA_PTR(self) = NULL;

  data = (mrb_yabm_data *)mrb_malloc(mrb, sizeof(mrb_yabm_data));
  data->arch = MODULE_DUMMY;
  gettimeofday(&time_now,NULL);
  data->start = time_now.tv_sec;
  DATA_PTR(self) = data;

  return self;
}

static mrb_value mrb_yabm_getarch(mrb_state *mrb, mrb_value self)
{
  mrb_yabm_data *data = DATA_PTR(self);

  return mrb_fixnum_value(data->arch);
}

static mrb_value mrb_yabm_print(mrb_state *mrb, mrb_value self)
{
  mrb_value val;
  mrb_get_args(mrb, "S", &val);
  printf(RSTRING_PTR(val));
  fflush(stdout);
  return mrb_nil_value();
}

static mrb_value mrb_yabm_count(mrb_state *mrb, mrb_value self)
{
  mrb_yabm_data *data = DATA_PTR(self);
  struct timeval time_now;

  gettimeofday(&time_now,NULL);
  return mrb_fixnum_value((time_now.tv_sec - data->start) * 1000 +
    time_now.tv_usec / 1000);
}

static mrb_value mrb_yabm_dummy(mrb_state *mrb, mrb_value self)
{
  return mrb_nil_value();
}

void mrb_mruby_yabm_gem_init(mrb_state *mrb)
{
  struct RClass *yabm;
  yabm = mrb_define_class(mrb, "YABM", mrb->object_class);

  mrb_define_const(mrb, yabm, "MODULE_UNKNOWN", mrb_fixnum_value(MODULE_UNKNOWN));
  mrb_define_const(mrb, yabm, "MODULE_RTL8196C", mrb_fixnum_value(MODULE_RTL8196C));
  mrb_define_const(mrb, yabm, "MODULE_BCM4712", mrb_fixnum_value(MODULE_BCM4712));
  mrb_define_const(mrb, yabm, "MODULE_RTL8196E", mrb_fixnum_value(MODULE_RTL8196E));
  mrb_define_const(mrb, yabm, "MODULE_BCM5350", mrb_fixnum_value(MODULE_BCM5350));
  mrb_define_const(mrb, yabm, "MODULE_BCM5352", mrb_fixnum_value(MODULE_BCM5352));
  mrb_define_const(mrb, yabm, "MODULE_BCM5354", mrb_fixnum_value(MODULE_BCM5354));
  mrb_define_const(mrb, yabm, "MODULE_ADM5120", mrb_fixnum_value(MODULE_ADM5120));
  mrb_define_const(mrb, yabm, "MODULE_ADM5120P", mrb_fixnum_value(MODULE_ADM5120P));
  mrb_define_const(mrb, yabm, "MODULE_KS8695", mrb_fixnum_value(MODULE_KS8695));

  mrb_define_method(mrb, yabm, "initialize", mrb_yabm_init, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "getarch", mrb_yabm_getarch, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "print", mrb_yabm_print, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "count", mrb_yabm_count, MRB_ARGS_NONE());

  mrb_define_method(mrb, yabm, "netstart", mrb_yabm_dummy, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, yabm, "netstartdhcp", mrb_yabm_dummy, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "getaddress", mrb_yabm_dummy, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "sntp", mrb_yabm_dummy, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, yabm, "gpiosetsel", mrb_yabm_dummy, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, yabm, "gpiosetled", mrb_yabm_dummy, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, yabm, "gpiogetctl", mrb_yabm_dummy, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetctl", mrb_yabm_dummy, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "gpiogetdir", mrb_yabm_dummy, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetdir", mrb_yabm_dummy, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "gpiogetdat", mrb_yabm_dummy, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetdat", mrb_yabm_dummy, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, yabm, "watchdogstart", mrb_yabm_dummy, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "watchdogreset", mrb_yabm_dummy, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "watchdogstop", mrb_yabm_dummy, MRB_ARGS_NONE());
  DONE;
}

void mrb_mruby_yabm_gem_final(mrb_state *mrb)
{
}

#endif /* YABM_DUMMY */
