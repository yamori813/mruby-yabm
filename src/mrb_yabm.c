/*
** mrb_yabm.c - Yet Another Bare Metal class
**
** Copyright (c) Hiroki Mori 2018
**
** See Copyright Notice in LICENSE
*/

#include <string.h>
#include <sys/time.h>

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/class.h"

#include "mrb_yabm.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

#define	MIBBASE		0xbb801000

void xprintf (const char* fmt, ...);

#if !defined(YABM_DUMMY)

typedef struct {
  int arch;
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

int addr16(char *ptr)
{
  int i, r, n;

  r = 0;
  for(i = 0; i < 4; ++i) {
    if(ptr[i] <= '9') {
      n = ptr[i] - '0';
    } else {
      n = ptr[i] - 'a' + 10;
    }
    r = (r << 4) | n;
  }
  return r;

}


static void mrb_yabm_strtoip6(mrb_state *mrb, mrb_value str, int *addr)
{
  char *cstr;
  int i;

  cstr = mrb_str_to_cstr(mrb, str);
  for (i = 0; i < 8; ++i) {
    addr[i] = addr16(cstr + i * 5);
  }
}

static mrb_value mrb_yabm_iptostr(mrb_state *mrb, uint32_t ip)
{
  char addr[16];
  int i , val, c, div;
  int han;

  c = 0;
  for(i = 0; i < 4; ++i ) {
    han = 0;
    val = (ip >> (8 * (3 - i))) & 0xff;
    div = val / 100;
    if (div != 0) {
      addr[c] = '0' + div;
      val -= 100 * div;
      ++c;
      han = 1;
    }
    div = val / 10;
    if (han == 1 || div != 0) {
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

static void addhexstr(int addr, char *buf)
{
int num;
int i;

  for(i = 12; i >= 0; i -= 4) {
    num = (addr >> i) & 0xf;
    if(num < 10) {
      *buf = '0' + num;
    } else {
      *buf = 'a' + num - 10;
    }
    ++buf;
  }
}

static mrb_value mrb_yabm_ip6tostr(mrb_state *mrb, uint32_t *ip)
{
  int i, c;
  char addr[48];

  c = 0;
  for(i = 0; i < 8; ++i) {
    addhexstr(ip[i], &addr[c]);
    c += 4;
    if (i != 7) {
      addr[c] = ':';
      ++c;
    }
  }
  addr[c] = '\0';

  return mrb_str_new_cstr(mrb, addr);
}

int getarch();

static mrb_value mrb_yabm_init(mrb_state *mrb, mrb_value self)
{
  mrb_yabm_data *data;

  data = (mrb_yabm_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_yabm_data_type;
  DATA_PTR(self) = NULL;

  data = (mrb_yabm_data *)mrb_malloc(mrb, sizeof(mrb_yabm_data));
  data->arch = getarch();
  DATA_PTR(self) = data;

  return self;
}

static mrb_value mrb_yabm_getarch(mrb_state *mrb, mrb_value self)
{
  mrb_yabm_data *data = DATA_PTR(self);

  return mrb_fixnum_value(data->arch);
}

void print(char *);

static mrb_value mrb_yabm_print(mrb_state *mrb, mrb_value self)
{
  mrb_value val;
  mrb_get_args(mrb, "S", &val);
  print(RSTRING_PTR(val));
  return mrb_nil_value();
}

#if defined(YABM_BROADCOM)
void print2(char *);

static mrb_value mrb_yabm_print2(mrb_state *mrb, mrb_value self)
{
  mrb_value val;
  mrb_get_args(mrb, "S", &val);
  print2(RSTRING_PTR(val));
  return mrb_nil_value();
}

void setbaud(int baud, int port);

static mrb_value mrb_yabm_setbaud(mrb_state *mrb, mrb_value self)
{
  mrb_int baud, port;
  mrb_get_args(mrb, "ii", &baud, &port);
  setbaud(baud, port);
  return mrb_nil_value();
}
#endif

int sys_now();

static mrb_value mrb_yabm_count(mrb_state *mrb, mrb_value self)
{

  return mrb_fixnum_value(sys_now());
}

static mrb_value mrb_yabm_now(mrb_state *mrb, mrb_value self)
{

  return mrb_fixnum_value(time(NULL));
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
  mrb_int port;
  mrb_get_args(mrb, "i", &port);
  rtl_udp_bind(port);
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
mrb_value addr, header;
mrb_int port;
char tmp[512];
int len;

  mrb_get_args(mrb, "SiS", &addr, &port, &header);
  str = mrb_str_new_cstr(mrb, "");
  if (http_connect(mrb_yabm_strtoip(mrb, addr), port,
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
mrb_value host, addr, header;
mrb_int port;
char tmp[512];
int len;

  mrb_get_args(mrb, "SSiS", &host, &addr, &port, &header);
  str = mrb_str_new_cstr(mrb, "");
  if (https_connect(RSTRING_PTR(host), mrb_yabm_strtoip(mrb, addr), port, RSTRING_PTR(header))) {
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

int lookup(char *host, uint32_t *addr, int type);

static mrb_value mrb_yabm_lookup(mrb_state *mrb, mrb_value self)
{
mrb_value host;
int addr;

  mrb_get_args(mrb, "S", &host);
  if (lookup(RSTRING_PTR(host), &addr, 0))
    return mrb_yabm_iptostr(mrb, addr);
  else
    return mrb_str_new_cstr(mrb, "");
}

static mrb_value mrb_yabm_lookup6(mrb_state *mrb, mrb_value self)
{
mrb_value host;
int addr[8];

  mrb_get_args(mrb, "S", &host);
  if (lookup(RSTRING_PTR(host), addr, 1))
    return mrb_yabm_ip6tostr(mrb, addr);
  else
    return mrb_str_new_cstr(mrb, "");
}

void sntp(uint32_t *addr, int type);

static mrb_value mrb_yabm_sntp(mrb_state *mrb, mrb_value self)
{
mrb_value addr;
int ip[8];
char *cstr;

  mrb_get_args(mrb, "S", &addr);
  cstr = mrb_str_to_cstr(mrb, addr);
  if (strlen(cstr) == 39) {
    mrb_yabm_strtoip6(mrb, addr, ip);
    sntp(ip, 1);
  } else {
    ip[0] = mrb_yabm_strtoip(mrb, addr);
    sntp(ip, 0);
  }

  return mrb_nil_value();
}

#if defined(YABM_REALTEK)
static mrb_value mrb_yabm_getmib(mrb_state *mrb, mrb_value self)
{
  unsigned long *lptr;
  mrb_int port, dir, type;
  mrb_get_args(mrb, "iii", &port, &dir, &type);

  lptr = (unsigned long *)(MIBBASE + dir + 
    port * MIB_SIZE + type);
/* 64 bit not support
  if ((mrb_fixnum(dir) == MIB_IN && (mrb_fixnum(type) == MIB_IFINOCTETS ||
    mrb_fixnum(type) == MIB_ETHERSTATSOCTETS)) ||
    (mrb_fixnum(dir) == MIB_OUT && mrb_fixnum(type) == MIB_IFOUTOCTETS)) {
  }
*/

  return mrb_fixnum_value(*lptr);
}
#endif /* YABM_REALTEK */

#if defined(YABM_ADMTEK)
unsigned long physt();

static mrb_value mrb_yabm_getphyst(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(physt());
}
#endif /* YABM_ADMTEK */

#if defined(YABM_REALTEK) || defined(YABM_ADMTEK)
int readmdio(unsigned int addr, unsigned int reg, unsigned int *dat);

static mrb_value mrb_yabm_readmdio(mrb_state *mrb, mrb_value self)
{
  int data;
  mrb_int port, reg;
  mrb_get_args(mrb, "ii", &port, &reg);
  readmdio(port, reg, &data);

  return mrb_fixnum_value(data);
}
#endif /* YABM_REALTEK || YABM_ADMTEK */

#if defined(YABM_REALTEK)
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
#endif /* YABM_REALTEK */

void i2c_init(int scl, int sda, int u);
int i2c_write(unsigned char ch, int start, int stop);
unsigned char i2c_read(int stop);
void delay_ms(int);

static mrb_value mrb_yabm_i2cinit(mrb_state *mrb, mrb_value self)
{
  int res, i;
  mrb_int scl, sda, u;
  mrb_get_args(mrb, "iii", &scl, &sda, &u);

  res = 0;
  i2c_init(scl, sda, u);
  for(i = 0; i < 0x80; ++i) {
    if(i2c_write(i << 1, 1, 1)) {
      res = 1;
      xprintf("I2C find %x\r\n", i);
     }
     delay_ms(10);
   }

  return mrb_fixnum_value(res);
}

static mrb_value mrb_yabm_i2cchk(mrb_state *mrb, mrb_value self)
{
  int res;
  mrb_int addr;
  mrb_get_args(mrb, "i", &addr);

 if(i2c_write(addr << 1, 1, 1))
   res = 1;
  else
   res = 0;

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
  mrb_int addr, rep;
  mrb_value arr;
  int len;
  int i;
  int res;

  res = 0;
  mrb_get_args(mrb, "iAi", &addr, &arr, &rep);
  len = RARRAY_LEN( arr );
  if (i2c_write((addr << 1) | 0, 1, 0)) {
    for (i = 0;i < len - 1; ++i) {
      if (!i2c_write(mrb_fixnum( mrb_ary_ref( mrb, arr, i ) ), 0, 0)) {
        break;
      }
    }
    if (i2c_write(mrb_fixnum( mrb_ary_ref( mrb, arr, i ) ), rep ? 0 : 1, 1))
      res = 1;
  }

  return mrb_fixnum_value(res);
}

static mrb_value mrb_yabm_i2creads(mrb_state *mrb, mrb_value self)
{
  mrb_int addr, len;
  mrb_value arr;
  int i;
  int val;

  mrb_get_args(mrb, "ii", &addr, &len);
  if(i2c_write((addr << 1) | 1, 1, 0)) {
    arr = mrb_ary_new(mrb);
    for (i = 0;i < len; ++i) {
      val = i2c_read(i == len - 1 ? 1 : 0);
      mrb_ary_push(mrb, arr, mrb_fixnum_value(val));
    }
  } else {
    return mrb_nil_value();
  }

  return arr;
}

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

#if defined(YABM_ADMTEK)
void gpio_setled(int port, unsigned long val);

static mrb_value mrb_yabm_gpiosetled(mrb_state *mrb, mrb_value self)
{
  mrb_int port, val;
  mrb_get_args(mrb, "ii", &port, &val);

  gpio_setled(port, val);

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

void watchdog_start(int);
void watchdog_reset();
void watchdog_stop();

static mrb_value mrb_yabm_watchdogstart(mrb_state *mrb, mrb_value self)
{
  mrb_int val;
  mrb_get_args(mrb, "i", &val);
  watchdog_start(val);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_yabm_watchdogreset(mrb_state *mrb, mrb_value self)
{
  watchdog_reset();

  return mrb_fixnum_value(0);
}

static mrb_value mrb_yabm_watchdogstop(mrb_state *mrb, mrb_value self)
{
  watchdog_stop();

  return mrb_fixnum_value(0);
}

void mrb_mruby_yabm_gem_init(mrb_state *mrb)
{
  struct RClass *yabm;
  yabm = mrb_define_class(mrb, "YABM", mrb->object_class);
  MRB_SET_INSTANCE_TT(yabm, MRB_TT_DATA);

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
  mrb_define_const(mrb, yabm, "MODULE_RTL8198", mrb_fixnum_value(MODULE_RTL8198));
  mrb_define_const(mrb, yabm, "MODULE_RTL8197D", mrb_fixnum_value(MODULE_RTL8197D));
  mrb_define_const(mrb, yabm, "MODULE_DUMMY", mrb_fixnum_value(MODULE_DUMMY));

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

  mrb_define_method(mrb, yabm, "initialize", mrb_yabm_init, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "getarch", mrb_yabm_getarch, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "print", mrb_yabm_print, MRB_ARGS_REQ(1));
#if defined(YABM_BROADCOM)
  mrb_define_method(mrb, yabm, "print2", mrb_yabm_print2, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "setbaud", mrb_yabm_setbaud, MRB_ARGS_REQ(2));
#endif
  mrb_define_method(mrb, yabm, "count", mrb_yabm_count, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "now", mrb_yabm_now, MRB_ARGS_NONE());
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
  mrb_define_method(mrb, yabm, "lookup6", mrb_yabm_lookup6, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "sntp", mrb_yabm_sntp, MRB_ARGS_REQ(1));
#if defined(YABM_REALTEK)
  mrb_define_method(mrb, yabm, "getmib", mrb_yabm_getmib, MRB_ARGS_REQ(3));
#endif
#if defined(YABM_ADMTEK)
  mrb_define_method(mrb, yabm, "getphyst", mrb_yabm_getphyst, MRB_ARGS_NONE());
#endif
#if defined(YABM_REALTEK) || defined(YABM_ADMTEK)
  mrb_define_method(mrb, yabm, "readmdio", mrb_yabm_readmdio, MRB_ARGS_REQ(2));
#endif
#if defined(YABM_REALTEK)
  mrb_define_method(mrb, yabm, "readuart", mrb_yabm_readuart, MRB_ARGS_NONE());
#endif
  mrb_define_method(mrb, yabm, "i2cinit", mrb_yabm_i2cinit, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, yabm, "i2cchk", mrb_yabm_i2cchk, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "i2cread", mrb_yabm_i2cread, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, yabm, "i2cwrite", mrb_yabm_i2cwrite, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, yabm, "i2cwrites", mrb_yabm_i2cwrites, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, yabm, "i2creads", mrb_yabm_i2creads, MRB_ARGS_REQ(2));

#if defined(YABM_REALTEK)
  mrb_define_method(mrb, yabm, "gpiosetsel", mrb_yabm_gpiosetsel, MRB_ARGS_REQ(4));
#endif
#if defined(YABM_ADMTEK)
  mrb_define_method(mrb, yabm, "gpiosetled", mrb_yabm_gpiosetled, MRB_ARGS_REQ(2));
#endif
  mrb_define_method(mrb, yabm, "gpiogetctl", mrb_yabm_gpiogetctl, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetctl", mrb_yabm_gpiosetctl, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "gpiogetdir", mrb_yabm_gpiogetdir, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetdir", mrb_yabm_gpiosetdir, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "gpiogetdat", mrb_yabm_gpiogetdat, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "gpiosetdat", mrb_yabm_gpiosetdat, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "watchdogstart", mrb_yabm_watchdogstart, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, yabm, "watchdogreset", mrb_yabm_watchdogreset, MRB_ARGS_NONE());
  mrb_define_method(mrb, yabm, "watchdogstop", mrb_yabm_watchdogstop, MRB_ARGS_NONE());
  DONE;
}

void mrb_mruby_yabm_gem_final(mrb_state *mrb)
{
}

#endif /* YABM_DUMMY */
