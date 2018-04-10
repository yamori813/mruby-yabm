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

#define	MIBBASE		0xbb801000

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

void net_start(int, int, int, int);
void net_startdhcp();

static mrb_value mrb_rtl8196c_netstart(mrb_state *mrb, mrb_value self)
{
  mrb_value addr, mask, gw, dns;
  mrb_get_args(mrb, "iiii", &addr, &mask, &gw, &dns);
  net_start(mrb_fixnum(addr), mrb_fixnum(mask), mrb_fixnum(gw),
    mrb_fixnum(dns));

  return mrb_nil_value();
}

static mrb_value mrb_rtl8196c_netstartdhcp(mrb_state *mrb, mrb_value self)
{

  net_startdhcp();
  return mrb_nil_value();
}

extern int netstat;

static mrb_value mrb_rtl8196c_netstat(mrb_state *mrb, mrb_value self)
{

  return mrb_fixnum_value(netstat);
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

static mrb_value mrb_rtl8196c_getmib(mrb_state *mrb, mrb_value self)
{
  unsigned long *lptr;
  mrb_value port, dir, type;
  mrb_get_args(mrb, "iii", &port, &dir, &type);

  lptr = (unsigned long *)(MIBBASE + mrb_fixnum(dir) + 
    mrb_fixnum(port) * MIB_SIZE + mrb_fixnum(type));
/* 64 bit not support
  if ((mrb_fixnum(dir) == MIB_IN && (mrb_fixnum(type) == MIB_IFINOCTETS ||
    mrb_fixnum(type) == MIB_ETHERSTATSOCTETS)) ||
    (mrb_fixnum(dir) == MIB_OUT && mrb_fixnum(type) == MIB_IFOUTOCTETS)) {
  }
*/

  return mrb_fixnum_value(*lptr);
}

int rtl8651_getAsicEthernetPHYReg(unsigned int, unsigned int, unsigned int *);

static mrb_value mrb_rtl8196c_readmdio(mrb_state *mrb, mrb_value self)
{
  int data;
  mrb_value port, reg;
  mrb_get_args(mrb, "ii", &port, &reg);
  rtl8651_getAsicEthernetPHYReg(mrb_fixnum(port), mrb_fixnum(reg), &data);

  return mrb_fixnum_value(data);
}

void mrb_mruby_rtlbm_rtl8196c_gem_init(mrb_state *mrb)
{
  struct RClass *rtl8196c;
  rtl8196c = mrb_define_class(mrb, "RTL8196C", mrb->object_class);

  mrb_define_const(mrb, rtl8196c, "MODULE_GENERIC", mrb_fixnum_value(MODULE_GENERIC));
  mrb_define_const(mrb, rtl8196c, "MODULE_HOMESPOTCUBE", mrb_fixnum_value(MODULE_HOMESPOTCUBE));
  mrb_define_const(mrb, rtl8196c, "MODULE_BBR4HGV2", mrb_fixnum_value(MODULE_BBR4HGV2));
  mrb_define_const(mrb, rtl8196c, "MODULE_LANW300NR", mrb_fixnum_value(MODULE_LANW300NR));
  mrb_define_const(mrb, rtl8196c, "MODULE_MZKMF300N", mrb_fixnum_value(MODULE_MZKMF300N));
  mrb_define_const(mrb, rtl8196c, "MIB_IN", mrb_fixnum_value(MIB_IN));
  mrb_define_const(mrb, rtl8196c, "MIB_OUT", mrb_fixnum_value(MIB_OUT));
  mrb_define_const(mrb, rtl8196c, "MIB_IFINOCTETS", mrb_fixnum_value(MIB_IFINOCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_IFINUCASTPKTS", mrb_fixnum_value(MIB_IFINUCASTPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSOCTETS", mrb_fixnum_value(MIB_ETHERSTATSOCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSUNDERSIZEPKTS", mrb_fixnum_value(MIB_ETHERSTATSUNDERSIZEPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSFRAGMEMTS", mrb_fixnum_value(MIB_ETHERSTATSFRAGMEMTS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSPKTS64OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS64OCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSPKTS65TO127OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS65TO127OCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSPKTS128TO255OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS128TO255OCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSPKTS256TO511OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS256TO511OCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSPKTS512TO1023OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS512TO1023OCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSPKTS1024TO1518OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS1024TO1518OCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSOVERSIZEPKTS", mrb_fixnum_value(MIB_ETHERSTATSOVERSIZEPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSJABBERS", mrb_fixnum_value(MIB_ETHERSTATSJABBERS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSMULTICASTPKTS", mrb_fixnum_value(MIB_ETHERSTATSMULTICASTPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSBROADCASTPKTS", mrb_fixnum_value(MIB_ETHERSTATSBROADCASTPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT1DTPPORTINDISCARDS", mrb_fixnum_value(MIB_DOT1DTPPORTINDISCARDS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSDROPEVENTS", mrb_fixnum_value(MIB_ETHERSTATSDROPEVENTS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3STATSFCSERRORS", mrb_fixnum_value(MIB_DOT3STATSFCSERRORS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3STATSSYMBOLERRORS", mrb_fixnum_value(MIB_DOT3STATSSYMBOLERRORS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3CONTROLINUNKNOWNOPCODES", mrb_fixnum_value(MIB_DOT3CONTROLINUNKNOWNOPCODES));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3INPAUSEFRAMES", mrb_fixnum_value(MIB_DOT3INPAUSEFRAMES));
  mrb_define_const(mrb, rtl8196c, "MIB_IFOUTOCTETS", mrb_fixnum_value(MIB_IFOUTOCTETS));
  mrb_define_const(mrb, rtl8196c, "MIB_IFOUTUCASTPKTS", mrb_fixnum_value(MIB_IFOUTUCASTPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_IFOUTMULTICASTPKTS", mrb_fixnum_value(MIB_IFOUTMULTICASTPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_IFOUTBROADCASTPKTS", mrb_fixnum_value(MIB_IFOUTBROADCASTPKTS));
  mrb_define_const(mrb, rtl8196c, "MIB_IFOUTDISCARDS", mrb_fixnum_value(MIB_IFOUTDISCARDS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3STATSSINGLECOLLISIONFRAMES", mrb_fixnum_value(MIB_DOT3STATSSINGLECOLLISIONFRAMES));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3STATSMULTIPLECOLLISIONFRAMES", mrb_fixnum_value(MIB_DOT3STATSMULTIPLECOLLISIONFRAMES));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3STATSDEFERREDTRANSMISSIONS", mrb_fixnum_value(MIB_DOT3STATSDEFERREDTRANSMISSIONS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3STATSLATECOLLISIONS", mrb_fixnum_value(MIB_DOT3STATSLATECOLLISIONS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3STATSEXCESSIVECOLLISIONS", mrb_fixnum_value(MIB_DOT3STATSEXCESSIVECOLLISIONS));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT3OUTPAUSEFRAMES", mrb_fixnum_value(MIB_DOT3OUTPAUSEFRAMES));
  mrb_define_const(mrb, rtl8196c, "MIB_DOT1DBASEPORTDELAYEXCEEDEDDISCARDS", mrb_fixnum_value(MIB_DOT1DBASEPORTDELAYEXCEEDEDDISCARDS));
  mrb_define_const(mrb, rtl8196c, "MIB_ETHERSTATSCOLLISIONS", mrb_fixnum_value(MIB_ETHERSTATSCOLLISIONS));

  mrb_define_method(mrb, rtl8196c, "initialize", mrb_rtl8196c_init, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "print", mrb_rtl8196c_print, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "count", mrb_rtl8196c_count, MRB_ARGS_NONE());
  mrb_define_method(mrb, rtl8196c, "netstart", mrb_rtl8196c_netstart, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, rtl8196c, "netstartdhcp", mrb_rtl8196c_netstart, MRB_ARGS_NONE());
  mrb_define_method(mrb, rtl8196c, "netstat", mrb_rtl8196c_netstat, MRB_ARGS_NONE());
  mrb_define_method(mrb, rtl8196c, "udpbind", mrb_rtl8196c_udpbind, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "udprecv", mrb_rtl8196c_udprecv, MRB_ARGS_NONE());
  mrb_define_method(mrb, rtl8196c, "http", mrb_rtl8196c_http, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, rtl8196c, "https", mrb_rtl8196c_https, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, rtl8196c, "lookup", mrb_rtl8196c_lookup, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rtl8196c, "getmib", mrb_rtl8196c_getmib, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, rtl8196c, "readmdio", mrb_rtl8196c_readmdio, MRB_ARGS_REQ(2));
  DONE;
}

void mrb_mruby_rtlbm_rtl8196c_gem_final(mrb_state *mrb)
{
}

