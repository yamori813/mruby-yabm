/*
** mrb_yabm.h - Yet Another Bare Metal class
**
** See Copyright Notice in LICENSE
*/

#ifndef MRB_RTL8196C_H
#define MRB_RTL8196C_H

void mrb_mruby_yabm_gem_init(mrb_state *mrb);

#define	MODULE_UNKNOWN				0
#define	MODULE_RTL8196C				1
#define	MODULE_BCM4712				2
#define	MODULE_RTL8196E				3
#define	MODULE_BCM5350				4
#define	MODULE_BCM5352				5
#define	MODULE_BCM5354				6
#define	MODULE_ADM5120				7
#define	MODULE_ADM5120P				8
#define	MODULE_KS8695				9
#define	MODULE_RTL8198				10
#define	MODULE_RTL8197D				11

#define	MODULE_DUMMY				100

#define	MIB_IN					0x100
#define	MIB_OUT					0x800

#define	MIB_SIZE				0x080

#define	MIB_IFINOCTETS				0x00
#define	MIB_IFINUCASTPKTS			0x08
#define	MIB_ETHERSTATSOCTETS			0x0c
#define	MIB_ETHERSTATSUNDERSIZEPKTS		0x14
#define	MIB_ETHERSTATSFRAGMEMTS			0x18
#define	MIB_ETHERSTATSPKTS64OCTETS		0x1c
#define	MIB_ETHERSTATSPKTS65TO127OCTETS		0x20
#define	MIB_ETHERSTATSPKTS128TO255OCTETS	0x24
#define	MIB_ETHERSTATSPKTS256TO511OCTETS	0x28
#define	MIB_ETHERSTATSPKTS512TO1023OCTETS	0x2c
#define	MIB_ETHERSTATSPKTS1024TO1518OCTETS	0x30
#define	MIB_ETHERSTATSOVERSIZEPKTS		0x34
#define	MIB_ETHERSTATSJABBERS			0x38
#define	MIB_ETHERSTATSMULTICASTPKTS		0x38
#define	MIB_ETHERSTATSBROADCASTPKTS		0x40
#define	MIB_DOT1DTPPORTINDISCARDS		0x44
#define	MIB_ETHERSTATSDROPEVENTS		0x48
#define	MIB_DOT3STATSFCSERRORS			0x4c
#define	MIB_DOT3STATSSYMBOLERRORS		0x50
#define	MIB_DOT3CONTROLINUNKNOWNOPCODES		0x54
#define	MIB_DOT3INPAUSEFRAMES			0x58

#define	MIB_IFOUTOCTETS				0x00
#define	MIB_IFOUTUCASTPKTS			0x08
#define	MIB_IFOUTMULTICASTPKTS			0x0c
#define	MIB_IFOUTBROADCASTPKTS			0x10
#define	MIB_IFOUTDISCARDS			0x14
#define	MIB_DOT3STATSSINGLECOLLISIONFRAMES	0x18
#define	MIB_DOT3STATSMULTIPLECOLLISIONFRAMES	0x1c
#define	MIB_DOT3STATSDEFERREDTRANSMISSIONS	0x20
#define	MIB_DOT3STATSLATECOLLISIONS		0x24
#define	MIB_DOT3STATSEXCESSIVECOLLISIONS	0x28
#define	MIB_DOT3OUTPAUSEFRAMES			0x2c
#define	MIB_DOT1DBASEPORTDELAYEXCEEDEDDISCARDS	0x30
#define	MIB_ETHERSTATSCOLLISIONS		0x34

#endif
