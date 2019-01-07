/*
** mrb_yabm.c - RTL8196C class
**
** Copyright (c) Hiroki Mori 2018
**
** See Copyright Notice in LICENSE
*/

#include <string.h>

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/array.h"
#include "mruby/string.h"

#include "mrb_yabm.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

#define	MIBBASE		0xbb801000

typedef struct {
  int model;
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
  mrb_value model;

  data = (mrb_yabm_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_yabm_data_type;
  DATA_PTR(self) = NULL;

  mrb_get_args(mrb, "i", &model);
  data = (mrb_yabm_data *)mrb_malloc(mrb, sizeof(mrb_yabm_data));
  data->model = mrb_fixnum(model);
  DATA_PTR(self) = data;

  return self;
}

void print(char *);

static mrb_value mrb_yabm_print(mrb_state *mrb, mrb_value self)
{
  mrb_value val;
  mrb_get_args(mrb, "S", &val);
  print(RSTRING_PTR(val));
  return mrb_nil_value();
}

int sys_now();

static mrb_value mrb_yabm_count(mrb_state *mrb, mrb_value self)
{

  return mrb_fixnum_value(sys_now());
}

void net_start(uint32_t, uint32_t, uint32_t, uint32_t);
void net_startdhcp();

static mrb_value mrb_yabm_netstart(mrb_state *mrb, mrb_value self)
{
  mrb_value addr, mask, gw, dns;
  mrb_get_args(mrb, "SSSS", &addr, &mask, &gw, &dns);
  net_start(mrb_yabm_strtoip(mrb, addr), mrb_yabm_strtoip(mrb, mask),
    mrb_yabm_strtoip(mrb, gw), mrb_yabm_strtoip(mrb, dns));

  return mrb_nil_value();
}

static mrb_value mrb_yabm_netstartdhcp(mrb_state *mrb, mrb_value self)
{

  net_startdhcp();
  return mrb_nil_value();
}

extern int netstat;
uint32_t getmyaddress();

static mrb_value mrb_yabm_netstat(mrb_state *mrb, mrb_value self)
{

  return mrb_fixnum_value(netstat);
}

static mrb_value mrb_yabm_getaddress(mrb_state *mrb, mrb_value self)
{

  return mrb_yabm_iptostr(mrb, getmyaddress());
}

void rtl_udp_init();
void rtl_udp_bind(int port);
int rtl_udp_recv(char *buf, int len);
void rtl_udp_send(int addr, int port, char *buf, int len);

static mrb_value mrb_yabm_udpinit(mrb_state *mrb, mrb_value self)
{
  rtl_udp_init();
  return mrb_nil_value();
}

static mrb_value mrb_yabm_udpbind(mrb_state *mrb, mrb_value self)
{
  mrb_value port;
  mrb_get_args(mrb, "i", &port);
  rtl_udp_bind(mrb_fixnum(port));
  return mrb_nil_value();
}

static mrb_value mrb_yabm_udprecv(mrb_state *mrb, mrb_value self)
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

static mrb_value mrb_yabm_udpsend(mrb_state *mrb, mrb_value self)
{
  mrb_value buff, addr;
  mrb_int port, len;
  mrb_get_args(mrb, "SiSi", &addr, &port, &buff, &len);
  rtl_udp_send(mrb_yabm_strtoip(mrb, addr), port, RSTRING_PTR(buff), len);
  return mrb_nil_value();
}

int http_connect(uint32_t addr, int port, char *header);
int http_read(char *buf, int len);
void http_close();

int https_connect(char *host, int addr, int port, char *header);
int https_read(char *buf, int len);
void https_close();

#if NOTUSE
int comphttp(char *ptr, int len)
{
char *sep;
char *clen;
char *h = "Content-Length: ";
int l;

  sep = strnstr(ptr, "\r\n\r\n", len);
  if (sep != NULL) {
    clen = strnstr(ptr, h, len);
    if(clen != NULL) {
      clen += strlen(h);
      l = 0;
      while(*clen >= '0' && *clen <= '9') {
        l = l * 10 + (*clen - '0');
        ++clen;
      }
      if(len == sep - ptr + l + 4)
        return 1;
    }
  }
  return 0;
}
#endif

static mrb_value mrb_yabm_http(mrb_state *mrb, mrb_value self)
{
mrb_value str;
mrb_value addr, port, header;
char tmp[512];
int len;

  mrb_get_args(mrb, "SiS", &addr, &port, &header);
  str = mrb_str_new_cstr(mrb, "");
  if (http_connect(mrb_yabm_strtoip(mrb, addr), mrb_fixnum(port),
    RSTRING_PTR(header))) {
    while(1) {
      len = http_read(tmp, sizeof(tmp) - 1);
      if (len < 0)
        break;
      if (len != 0) {
        tmp[len] = '\0';
        mrb_str_cat2(mrb, str, tmp);
      }
#if NOTUSR
      if(comphttp(RSTRING_PTR(str), RSTRING_LEN(str)))
        break;
#endif
    }
    http_close();
  }
  return str;
}

static mrb_value mrb_yabm_https(mrb_state *mrb, mrb_value self)
{
mrb_value str;
mrb_value host, addr, port, header;
char tmp[512];
int len;

  mrb_get_args(mrb, "SSiS", &host, &addr, &port, &header);
  str = mrb_str_new_cstr(mrb, "");
  if (https_connect(RSTRING_PTR(host), mrb_yabm_strtoip(mrb, addr), mrb_fixnum(port), RSTRING_PTR(header))) {
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

int lookup(char *host, uint32_t *addr);

static mrb_value mrb_yabm_lookup(mrb_state *mrb, mrb_value self)
{
mrb_value host;
int addr;

  mrb_get_args(mrb, "S", &host);
  lookup(RSTRING_PTR(host), &addr);

  return mrb_yabm_iptostr(mrb, addr);
}

void sntp(uint32_t addr);

static mrb_value mrb_yabm_sntp(mrb_state *mrb, mrb_value self)
{
mrb_value addr;

  mrb_get_args(mrb, "S", &addr);
  sntp(mrb_yabm_strtoip(mrb, addr));

  return mrb_nil_value();
}

#if defined(YABM_REALTEK)
static mrb_value mrb_yabm_getmib(mrb_state *mrb, mrb_value self)
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

static mrb_value mrb_yabm_readmdio(mrb_state *mrb, mrb_value self)
{
  int data;
  mrb_value port, reg;
  mrb_get_args(mrb, "ii", &port, &reg);
  rtl8651_getAsicEthernetPHYReg(mrb_fixnum(port), mrb_fixnum(reg), &data);

  return mrb_fixnum_value(data);
}

int getrxdata(char *buff, int len);
static mrb_value mrb_yabm_readuart(mrb_state *mrb, mrb_value self)
{
mrb_value str;
char buff[1024];
int len;
  
  len = getrxdata(buff, sizeof(buff) - 1);
  if (len != 0) {
    buff[len] = 0;
    str = mrb_str_new_cstr(mrb, buff);
  } else {
    str = mrb_str_new_cstr(mrb, "");
  }
    
  return str;
}

void i2c_init(int scl, int sda);
int i2c_write(unsigned char ch, int start, int stop);
unsigned char i2c_read(int stop);
void delay_ms(int);
void xprintf (const char* fmt, ...);

static mrb_value mrb_yabm_i2cinit(mrb_state *mrb, mrb_value self)
{
  int res, i;
  mrb_int scl, sda;
  mrb_get_args(mrb, "ii", &scl, &sda);

  res = 0;
  i2c_init(scl, sda);
  for(i = 0; i < 0x80; ++i) {
    if(i2c_write(i << 1, 1, 1)) {
      res = 1;
      xprintf("I2C find %x\r\n", i);
     }
     delay_ms(10);
   }

  return mrb_fixnum_value(res);
}

static mrb_value mrb_yabm_i2cread(mrb_state *mrb, mrb_value self)
{
  int res;
  mrb_int addr, reg;

  res = 0;
  mrb_get_args(mrb, "ii", &addr, &reg);
  if(i2c_write((addr << 1) | 0, 1, 0)) {
    if(i2c_write(reg, 0, 1)) {
      delay_ms(10);
      if(i2c_write((addr << 1) | 1, 1, 0)) 
           res = i2c_read(1);
     }
  }
  return mrb_fixnum_value(res);
}

static mrb_value mrb_yabm_i2cwrite(mrb_state *mrb, mrb_value self)
{
  int res;
  mrb_int addr, reg, val;

  res = 0;
  mrb_get_args(mrb, "iii", &addr, &reg, &val);
  if(i2c_write((addr << 1) | 0, 1, 0)) {
    if(i2c_write(reg, 0, 0)) {
      if(i2c_write(val, 0, 1)) {
        res = 1;
      }
    }
  }
  return mrb_fixnum_value(res);
}

static mrb_value mrb_yabm_i2cwrites(mrb_state *mrb, mrb_value self)
{
  mrb_int addr;
  mrb_value arr;
  int len;
  int i;

  mrb_get_args(mrb, "iA", &addr, &arr);
  len = RARRAY_LEN( arr );
  i2c_write((addr << 1) | 0, 1, 0);
  for (i = 0;i < len - 1; ++i)
    i2c_write(mrb_fixnum( mrb_ary_ref( mrb, arr, i ) ), 0, 0);
  i2c_write(mrb_fixnum( mrb_ary_ref( mrb, arr, i ) ), 0, 1);

  return mrb_fixnum_value(0);
}
#endif

#if defined(YABM_REALTEK)
void gpio_setsel(unsigned long sel, unsigned long selmask,
unsigned long sel2, unsigned long selmask2);
#endif
unsigned long gpio_getctl();
void gpio_setctl(unsigned long val);
unsigned long gpio_getdir();
void gpio_setdir(unsigned long val);
unsigned long gpio_getdat();
void gpio_setdat(unsigned long val);

#if defined(YABM_REALTEK)
static mrb_value mrb_yabm_gpiosetsel(mrb_state *mrb, mrb_value self)
{
  mrb_int sel, selmask, sel2, selmask2;
  mrb_get_args(mrb, "iiii", &sel, &selmask, &sel2, &selmask2);

  gpio_setsel(sel, selmask, sel2, selmask2);

  return mrb_fixnum_value(0);
}
#endif

static mrb_value mrb_yabm_gpiogetctl(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(gpio_getctl());
}

static mrb_value mrb_yabm_gpiosetctl(mrb_state *mrb, mrb_value self)
{
  mrb_int val;
  mrb_get_args(mrb, "i", &val);
  gpio_setctl(val);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_yabm_gpiogetdir(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(gpio_getdir());
}

static mrb_value mrb_yabm_gpiosetdir(mrb_state *mrb, mrb_value self)
{
  mrb_int val;
  mrb_get_args(mrb, "i", &val);
  gpio_setdir(val);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_yabm_gpiogetdat(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(gpio_getdat());
}

static mrb_value mrb_yabm_gpiosetdat(mrb_state *mrb, mrb_value self)
{
  mrb_int val;
  mrb_get_args(mrb, "i", &val);
  gpio_setdat(val);

  return mrb_fixnum_value(0);
}

void mrb_mruby_yabm_gem_init(mrb_state *mrb)
{
  struct RClass *yabm;
  yabm = mrb_define_class(mrb, "YABM", mrb->object_class);

  mrb_define_const(mrb, yabm, "MODULE_GENERIC", mrb_fixnum_value(MODULE_GENERIC));
  mrb_define_const(mrb, yabm, "MODULE_RTL8196C", mrb_fixnum_value(MODULE_RTL8196C));
  mrb_define_const(mrb, yabm, "MODULE_BCM4712", mrb_fixnum_value(MODULE_BCM4712));
  mrb_define_const(mrb, yabm, "MODULE_RTL8196E", mrb_fixnum_value(MODULE_RTL8196E));
  mrb_define_const(mrb, yabm, "MODULE_BCM5350", mrb_fixnum_value(MODULE_BCM5350));
  mrb_define_const(mrb, yabm, "MODULE_BCM5352", mrb_fixnum_value(MODULE_BCM5352));
  mrb_define_const(mrb, yabm, "MODULE_BCM5354", mrb_fixnum_value(MODULE_BCM5354));

#if defined(YABM_REALTEK)
  mrb_define_const(mrb, yabm, "MIB_IN", mrb_fixnum_value(MIB_IN));
  mrb_define_const(mrb, yabm, "MIB_OUT", mrb_fixnum_value(MIB_OUT));
  mrb_define_const(mrb, yabm, "MIB_IFINOCTETS", mrb_fixnum_value(MIB_IFINOCTETS));
  mrb_define_const(mrb, yabm, "MIB_IFINUCASTPKTS", mrb_fixnum_value(MIB_IFINUCASTPKTS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSOCTETS", mrb_fixnum_value(MIB_ETHERSTATSOCTETS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSUNDERSIZEPKTS", mrb_fixnum_value(MIB_ETHERSTATSUNDERSIZEPKTS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSFRAGMEMTS", mrb_fixnum_value(MIB_ETHERSTATSFRAGMEMTS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSPKTS64OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS64OCTETS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSPKTS65TO127OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS65TO127OCTETS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSPKTS128TO255OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS128TO255OCTETS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSPKTS256TO511OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS256TO511OCTETS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSPKTS512TO1023OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS512TO1023OCTETS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSPKTS1024TO1518OCTETS", mrb_fixnum_value(MIB_ETHERSTATSPKTS1024TO1518OCTETS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSOVERSIZEPKTS", mrb_fixnum_value(MIB_ETHERSTATSOVERSIZEPKTS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSJABBERS", mrb_fixnum_value(MIB_ETHERSTATSJABBERS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSMULTICASTPKTS", mrb_fixnum_value(MIB_ETHERSTATSMULTICASTPKTS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSBROADCASTPKTS", mrb_fixnum_value(MIB_ETHERSTATSBROADCASTPKTS));
  mrb_define_const(mrb, yabm, "MIB_DOT1DTPPORTINDISCARDS", mrb_fixnum_value(MIB_DOT1DTPPORTINDISCARDS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSDROPEVENTS", mrb_fixnum_value(MIB_ETHERSTATSDROPEVENTS));
  mrb_define_const(mrb, yabm, "MIB_DOT3STATSFCSERRORS", mrb_fixnum_value(MIB_DOT3STATSFCSERRORS));
  mrb_define_const(mrb, yabm, "MIB_DOT3STATSSYMBOLERRORS", mrb_fixnum_value(MIB_DOT3STATSSYMBOLERRORS));
  mrb_define_const(mrb, yabm, "MIB_DOT3CONTROLINUNKNOWNOPCODES", mrb_fixnum_value(MIB_DOT3CONTROLINUNKNOWNOPCODES));
  mrb_define_const(mrb, yabm, "MIB_DOT3INPAUSEFRAMES", mrb_fixnum_value(MIB_DOT3INPAUSEFRAMES));
  mrb_define_const(mrb, yabm, "MIB_IFOUTOCTETS", mrb_fixnum_value(MIB_IFOUTOCTETS));
  mrb_define_const(mrb, yabm, "MIB_IFOUTUCASTPKTS", mrb_fixnum_value(MIB_IFOUTUCASTPKTS));
  mrb_define_const(mrb, yabm, "MIB_IFOUTMULTICASTPKTS", mrb_fixnum_value(MIB_IFOUTMULTICASTPKTS));
  mrb_define_const(mrb, yabm, "MIB_IFOUTBROADCASTPKTS", mrb_fixnum_value(MIB_IFOUTBROADCASTPKTS));
  mrb_define_const(mrb, yabm, "MIB_IFOUTDISCARDS", mrb_fixnum_value(MIB_IFOUTDISCARDS));
  mrb_define_const(mrb, yabm, "MIB_DOT3STATSSINGLECOLLISIONFRAMES", mrb_fixnum_value(MIB_DOT3STATSSINGLECOLLISIONFRAMES));
  mrb_define_const(mrb, yabm, "MIB_DOT3STATSMULTIPLECOLLISIONFRAMES", mrb_fixnum_value(MIB_DOT3STATSMULTIPLECOLLISIONFRAMES));
  mrb_define_const(mrb, yabm, "MIB_DOT3STATSDEFERREDTRANSMISSIONS", mrb_fixnum_value(MIB_DOT3STATSDEFERREDTRANSMISSIONS));
  mrb_define_const(mrb, yabm, "MIB_DOT3STATSLATECOLLISIONS", mrb_fixnum_value(MIB_DOT3STATSLATECOLLISIONS));
  mrb_define_const(mrb, yabm, "MIB_DOT3STATSEXCESSIVECOLLISIONS", mrb_fixnum_value(MIB_DOT3STATSEXCESSIVECOLLISIONS));
  mrb_define_const(mrb, yabm, "MIB_DOT3OUTPAUSEFRAMES", mrb_fixnum_value(MIB_DOT3OUTPAUSEFRAMES));
  mrb_define_const(mrb, yabm, "MIB_DOT1DBASEPORTDELAYEXCEEDEDDISCARDS", mrb_fixnum_value(MIB_DOT1DBASEPORTDELAYEXCEEDEDDISCARDS));
  mrb_define_const(mrb, yabm, "MIB_ETHERSTATSCOLLISIONS", mrb_fixnum_value(MIB_ETHERSTATSCOLLISIONS));
#endif

  mrb_define_method(mrb, yabm, "initialize", mrb_yabm_init, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "print", mrb_yabm_print, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "count", mrb_yabm_count, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "netstart", mrb_yabm_netstart, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, yabm, "netstartdhcp", mrb_yabm_netstartdhcp, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "netstat", mrb_yabm_netstat, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "getaddress", mrb_yabm_getaddress, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "udpinit", mrb_yabm_udpinit, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "udpbind", mrb_yabm_udpbind, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "udprecv", mrb_yabm_udprecv, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "udpsend", mrb_yabm_udpsend, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, yabm, "http", mrb_yabm_http, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, yabm, "https", mrb_yabm_https, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, yabm, "lookup", mrb_yabm_lookup, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "sntp", mrb_yabm_sntp, MRB_ARGS_REQ(1));
#if defined(YABM_REALTEK)
  mrb_define_method(mrb, yabm, "getmib", mrb_yabm_getmib, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, yabm, "readmdio", mrb_yabm_readmdio, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, yabm, "readuart", mrb_yabm_readuart, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "i2cinit", mrb_yabm_i2cinit, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, yabm, "i2cread", mrb_yabm_i2cread, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, yabm, "i2cwrite", mrb_yabm_i2cwrite, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, yabm, "i2cwrites", mrb_yabm_i2cwrites, MRB_ARGS_REQ(2));

  mrb_define_method(mrb, yabm, "gpiosetsel", mrb_yabm_gpiosetsel, MRB_ARGS_REQ(4));
#endif
  mrb_define_method(mrb, yabm, "gpiogetctl", mrb_yabm_gpiogetctl, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetctl", mrb_yabm_gpiosetctl, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "gpiogetdir", mrb_yabm_gpiogetdir, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetdir", mrb_yabm_gpiosetdir, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "gpiogetdat", mrb_yabm_gpiogetdat, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetdat", mrb_yabm_gpiosetdat, MRB_ARGS_REQ(1));
  DONE;
}

void mrb_mruby_yabm_gem_final(mrb_state *mrb)
{
}

