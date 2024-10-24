#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/def.h> /* provide ntohs, htons */

#define WITH_LWIP 1
//#define HAVE_IPV6 1

#define PACKAGE_NAME "libcoap-lwip"
#define PACKAGE_VERSION "?"
#define PACKAGE_STRING PACKAGE_NAME PACKAGE_VERSION

#define assert(x) LWIP_ASSERT("CoAP assert failed", x)

/* it's just provided by libc. i hope we don't get too many of those, as
 * actually we'd need autotools again to find out what environment we're
 * building in */
#define HAVE_STRNLEN 1

#define HAVE_LIMITS_H

#define COAP_RESOURCES_NOHASH

#define HAVE_MALLOC

#endif /* _CONFIG_H_ */
