/*
  dll.h

  Constants relating to the PPP link
*/

#ifndef __OS_DLL_H
#define __OS_DLL_H

/* Data contains the config options */
#define LCP_CONFIG_REQUEST	1

/* All options acceptable, copy of options returned */
#define LCP_CONFIG_ACK		2

/* Not all options acceptable, copy of unacceptable options returned */
#define LCP_CONFIG_NAK		3

/* Some of the options wernt recognised, copy of unrecognised returned */
#define LCP_CONFIG_REJECT	4

/* Close down link - continue sending until ACK'ed */
#define LCP_TERMINATE_REQUEST	5
#define LCP_TERMINATE_ACK	6

/* Unrecognised LCP code, copy of packet returned */
#define LCP_CODE_REJECT		7

/* Unrecognised protocol, return protocol number and data */
#define LCP_PROTOCOL_REJECT	8

/* Data is 'magic number' of link */
#define LCP_ECHO_REQUEST	9
#define LCP_ECHO_REPLY		10

/* Silently discard packet */
#define LCP_DISCARD_REQUEST	11

#define LCP_CONFIG_MRU		1
#define LCP_CONFIG_ASYNCMAP	2
#define LCP_CONFIG_AUTH		3
#define LCP_CONFIG_QUALITY	4
#define LCP_CONFIG_MAGIC	5
#define LCP_CONFIG_PROTOCOL_COMPRESSION	7
#define LCP_CONFIG_ADDRESS_COMPRESSION	8
#define LCP_CONFIG_LAST		9

#define PPP_DLL_LCP			0xC021
#define PPP_DLL_AUTH_PAP		0xC023
#define PPP_DLL_AUTH_CHAP		0xC223
#define PPP_DLL_QUALITY_LQR		0xC025

#define PPP_DLL_IP			0x0021
#define PPP_DLL_IPCP			0x8021

#endif /* __OS_DLL_H */
