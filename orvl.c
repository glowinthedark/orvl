/*
 * ORVL: Orvibo (S20) Control program.          2017-03-27.  SMS.
 *
 *----------------------------------------------------------------------
 * Copyright 2017 Steven M. Schweda
 *
 * This file is part of ORVL.
 *
 * ORVL is subject to the terms of the Perl Foundation Artistic License
 * 2.0.  A copy of the License is included in the ORVL kit file
 * "artistic_license_2_0.txt", and on the Web at:
 * http://www.perlfoundation.org/artistic_license_2_0
 *
 * "Orvibo" is a trademark of ORVIBO Ltd.  http://www.orvibo.com
 *----------------------------------------------------------------------
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 *    C macros for program customization by the user:
 *
 * EARLY_RECVFROM       Defining EARLY_RECVFROM causes the program to
 * NO_EARLY_RECVFROM    attempt a recvfrom() before sending a message.
 *                      May not be useful.
 *                      Default: defined, unless NO_EARLY_RECVFROM is
 *                      defined.
 *
 * NEED_SYS_FILIO_H     Use <sys/filio.h> to get FIONBIO defined.
 *                      (FIONBIO is used with ioctl().)
 *
 * NO_AF_CHECK          Define NO_AF_CHECK to disable a check on results
 * GETADDRINFO_OK       from getaddrinfo() (address family == AF_INET).
 *                      On VMS VAX systems running TCPIP V5.3 - ECO 4,
 *                      an apparent bug causes the check to fail.  By
 *                      default, on all VMS VAX systems, a work-around
 *                      is applied, to avoid the spurious failure.
 *                      Define GETADDRINFO_OK to disable the
 *                      work-around, without disabling the check.
 *
 * NO_OPER_PRIVILEGE    On VMS, do not attempt to gain OPER privilege.
 *
 * NO_STATE_IN_EXIT_STATUS  Define to omit any device state data from
 *                          the program exit status.
 *
 * ORVL_DDF             Default name of the device data file (DDF).
 *                      Default: "ORVL_DDF".  This name is treated as
 *                      an environment variable (VMS: logical name)
 *                      which points to an actual file, but if that
 *                      translation fails, then the value itself is
 *                      used.  A simple "ddf" command-line option
 *                      enables use of the DDF; an explicit "ddf=name"
 *                      command-line option overrides this default file
 *                      name.
 *
 * RECVFROM_6           The data type to use for arg 6 of recvfrom().
 *                      Default: "unsigned int".  Popular alternatives
 *                      include "int" and "socklen_t".  (If the actual
 *                      type has the same size as "unsigned int", then
 *                      compiler warnings about mismatched pointer types
 *                      are probably harmless.)
 *
 * SOCKET_TIMEOUT       Time to wait for a device response.
 *                      Default: 0.5s (500000 microseconds).
 *
 * TASK_RETRY_MAX       Number of times to retry a task (send message to
 *                      device, receive response from device).
 *                      Default: 4.  (4 retries means 5 tries, total.)
 *
 * TASK_RETRY_WAIT      Time to wait before retrying a task.
 *                      Default:  0.5s (500 milliseconds).
 *                      Must be less than 1.0s (1000 milliseconds).
 *
 * USE_FCNTL            Use fcntl() to set socket to non-blocking.
 *                      Default is to use ioctl().
 *
 * Notes/hints:
 *
 *    On AIX, try "-DRECVFROM_6=socklen_t", and the appropriate compiler
 * option to make "char" signed by default (GCC: "-fsigned-char"?).
 *
 *    On Solaris, try "-DNEED_SYS_FILIO_H" or "-DUSE_FCNTL" to compile,
 * and "-lsocket" to link.
 *
 *    Some of this code depends on guesswork, based on some reverse
 * engineering.  In some cases, such as packing some (probable) two-byte
 * values (where one byte is always zero), it's not clear which two
 * bytes belong together.  Some adjustment may be needed when values
 * larger than 255 are encountered.
 * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

/*--------------------------------------------------------------------*/
/*    Program identification. */

#define PROGRAM_NAME        "ORVL"
#define PROGRAM_VERSION_MAJ     0
#define PROGRAM_VERSION_MIN     2

/*--------------------------------------------------------------------*/
/*    Header files, and related macros. */

#ifdef VMS
# include <prvdef.h>
# include <ssdef.h>
# include <starlet.h>
# include <stsdef.h>
#endif /* def VMS */

#ifdef _WIN32
# define _CRT_SECURE_NO_WARNINGS
# include <Winsock2.h>
# include <Ws2tcpip.h>
# define ssize_t SSIZE_T
# define BAD_SOCKET( s) ((s) == INVALID_SOCKET)
# define CLOSE_SOCKET closesocket
# define IOCTL_SOCKET ioctlsocket
# define LOCALTIME_R( t, tm) localtime_s( (tm), (t))    /* Note arg order. */
# define STRNCASECMP _strnicmp
#else /* def _WIN32 */
# include <unistd.h>
# include <netdb.h>
# include <sys/socket.h>
# ifdef USE_FCNTL
#  include <fcntl.h>
# else /* def USE_FCNTL */
#  include <sys/ioctl.h>
#  ifdef NEED_SYS_FILIO_H
#   include <sys/filio.h>                               /* For FIONBIO. */
#  endif /* def NEED_SYS_FILIO_H */
# endif /* def USE_FCNTL [else] */
# define BAD_SOCKET( s) ((s) < 0)
# define CLOSE_SOCKET close
# define INVALID_SOCKET (-1)
# define IOCTL_SOCKET ioctl
# define LOCALTIME_R( t, tm) localtime_r( (t), (tm))
# define SOCKET int
# define STRNCASECMP strncasecmp
#endif /* def _WIN32 [else] */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*--------------------------------------------------------------------*/
/*    User-adjustable macros. */

#ifndef NO_EARLY_RECVFROM
# define EARLY_RECVFROM                 /* Perform early recvfrom(). */
#endif /* ndef NO_EARLY_RECVFROM */

#define ORVL_DDF       "ORVL_DDF"       /* ORVL device data file name. */

# ifndef RECVFROM_6
#  define RECVFROM_6 unsigned int       /* Type for arg 6 of recvfrom(). */
# endif /* ndef RECVFROM_6 */           /* ("int", "socklen_t", ...?) */

#define SOCKET_TIMEOUT     500000       /* Microseconds. */

#define TASK_RETRY_MAX          4       /* Task retry count, */
#define TASK_RETRY_WAIT       500       /* delay.  Milliseconds (< 1000ms). */

/*--------------------------------------------------------------------*/
/*    Fixed macros. */

#define DEV_NAME_LEN           16
#define PASSWORD_LEN           12
#define MAC_ADDR_SIZE           6
#define PORT_ORV            10000       /* IP port used for device comm. */
#define TIME_OFS       0x83aa7e80       /* 70y (1970 - 1900). */

#define OMIN( a, b) (((a) <= (b)) ? (a) : (b))

/*--------------------------------------------------------------------*/
/*    Data structures. */

typedef struct orv_data_t                       /* Orvibo device data. */
{
  struct orv_data_t *next;                      /* Link to next. */
  struct orv_data_t *prev;                      /* Link to previous. */
  unsigned int cnt_flg;                         /* Count (O)/flag. */
  int sort_key;                                 /* Sort key. */
  struct in_addr ip_addr;                       /* IP address (net order). */
  unsigned short port;                          /* IP port (net order). */
  short type;                                   /* Device type (icon code). */
  unsigned char name[ DEV_NAME_LEN];            /* Device name. */
  unsigned char passwd[ PASSWORD_LEN];          /* Remote password. */
  unsigned char mac_addr[ MAC_ADDR_SIZE];       /* MAC address. */
  char state;                                   /* Device state. */
} orv_data_t;

/*--------------------------------------------------------------------*/
/*    Symbolic constants. */

/* Debug category flags. */
				
#define DBG_ACT        0x00000001       /* Action. */
#define DBG_DNS        0x00000002       /* DNS, name resolution. */
#define DBG_DEV        0x00000004       /* Device list. */
#define DBG_FIL        0x00000008       /* Device data file. */
#define DBG_MSI        0x00000010       /* Message in. */
#define DBG_MSO        0x00000020       /* Message out. */
#define DBG_OPT        0x00000040       /* Options. */
#define DBG_SEL        0x00000080       /* Name, address selection. */
#define DBG_SIO        0x00000100       /* Socket I/O. */
#define DBG_VMS        0x00000200       /* VMS-specific. */
#define DBG_WIN        0x00000400       /* Windows-specific. */

/* fprintf_device_list() flags. */

#define FDL_BRIEF      0x00000001       /* Brief. */
#define FDL_DDF        0x00000002       /* Device data file. */
#define FDL_QUIET      0x00000004       /* Quiet. */
#define FDL_SINGLE     0x00000008       /* Single device. */

/* Response types (bit mask). */

#define RSP_CL         0x00000001       /* Subscribe. */
#define RSP_DC         0x00000002       /* Device control (message). */
#define RSP_HB         0x00000004       /* Heartbeat. */
#define RSP_QA         0x00000008       /* Global discovery. */
#define RSP_QG         0x00000010       /* Unit discovery. */
#define RSP_RT         0x00000020       /* Read table. */
#define RSP_SF         0x00000040       /* Device control. */
#define RSP_TM         0x00000080       /* Write Table. */
#define RSP___         0x80000000       /* Unknown/unrecognized response. */

/* Task codes. */

#define TSK_MIN                 0       /* Smallest TSK_xxx value. */
#define TSK_GLOB_DISC_B         0       /* Global discovery, broadcast. */
#define TSK_GLOB_DISC           1       /* Global discovery, specific. */
#define TSK_UNIT_DISC           2       /* Unit discovery. */
#define TSK_SUBSCRIBE           3       /* Subscribe. */
#define TSK_HEARTBEAT           4       /* Heartbeat. */
#define TSK_SW_OFF              5       /* Switch off. */
#define TSK_SW_ON               6       /* Switch on. */
#define TSK_RT_SOCKET           7       /* Read table: socket. */
#define TSK_RT_TIMING           8       /* Read table: timing. */
#define TSK_WT_SOCKET           9       /* Write table: socket. */
#define TSK_WT_TIMING          10       /* Write table: timing. */
#define TSK_MAX                10       /* Largest TSK_xxx value. */

/*--------------------------------------------------------------------*/
/*    Global Storage.
 *    Device messages, device type names, command-line keyword arrays.
 */

static int debug;                               /* Debug flag(s). */

/* Basic output message content:
 *    [0],[1]: Prefix ("magic key") = "hd".
 *    [2],[3]: Message length.
 *    [4],[5]: Operation code.
 */

/* Subscribe ("cl" = Claim?). */

static unsigned char cmd_subs[] =
 { 'h', 'd', 0x00, 0x00, 'c', 'l',              /* Prefix, length, op. */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          /* MAC address (fwd). */
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20,          /* 0x20 fill. */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          /* MAC address (rev). */
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20           /* 0x20 fill. */
 };

/* Switch off/on ("dc" = Device Control?). */

static unsigned char cmd_switch[] =
 { 'h', 'd', 0x00, 0x00, 'd', 'c',              /* Prefix, length, op. */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          /* MAC address. */
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20,          /* 0x20 fill. */
   0x00, 0x00, 0x00, 0x00, 0x00
 };

/* Heartbeat ("hb" = HeartBeat?). */

static unsigned char cmd_heartbeat[] =
 { 'h', 'd', 0x00, 0x00, 'h', 'b',
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          /* MAC address. */
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20,          /* 0x20 fill. */
   0x00, 0x00, 0x00, 0x00                       /* Unk(4). */
 };

/* Global discovery ("qa" = Query All?). */

static unsigned char cmd_glob_disc[] =
 { 'h', 'd', 0x00, 0x00, 'q', 'a' };            /* Prefix, length, op. */

/* Unit discovery ("qg" = Query G?). */

static unsigned char cmd_unit_disc[] =
 { 'h', 'd', 0x00, 0x00, 'q', 'g',              /* Prefix, length, op. */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          /* MAC address. */
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20           /* 0x20 fill. */
 };

/* Read table ("rt" = Read Table?). */

static unsigned char cmd_read_table[] =
 { 'h', 'd', 0x00, 0x00, 'r', 't',              /* Prefix, length, op. */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          /* MAC address. */
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20,          /* 0x20 fill. */
   0x00, 0x00, 0x00, 0x00,                      /* Unk(4). */
   0x04, 0x00, 0x00, 0x00,                      /* Table 04, version 00. */
   0x00, 0x00, 0x00                             /* Unk(1), rec-len(2) = 0. */
 };

#define READ_TABLE_TABLE 22
#define READ_TABLE_VERSION 24

/* Write table ("tm" = Table Modify?). */

static unsigned char cmd_write_table[] =
 { 'h', 'd', 0x00, 0x00, 't', 'm'               /* Prefix, length, op. */
/* 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, */       /* MAC address. */
/* 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, */       /* 0x20 fill. */
/* 0x00, 0x00, 0x00, 0x00,             */       /* Unk(4). */
/* 0x04, 0x00, 0x01                    */       /* Table 04, version 01. */
/* 0x00, 0x00                          */       /* rec-len(2). */
 };


/* Device type names. */

#define ICON_MAX 5

static const char *icon_name[ ICON_MAX+ 1] =    /* Device type names. */
 { "Light", "Fan", "Thermostat", "Switch",
   "Socket-US", "Socket-AU"
 };

/* ORVL operation keywords. */

char *oprs[] =
{       "heartbeat",    "help",         "list",         "qlist",
        "off",          "on",           "set",          "usage",
        "version"
};

#define OPR_HEARTBEAT           0
#define OPR_HELP                1
#define OPR_LIST                2
#define OPR_QLIST               3
#define OPR_OFF                 4
#define OPR_ON                  5
#define OPR_SET                 6
#define OPR_USAGE               7
#define OPR_VERSION             8

/* ORVL option keywords. */

char *opts[] =
 {      "brief",        "quiet",        "ddf",          "ddf=",
        "debug",        "debug=",       "name=",        "password=",
        "sort="
 };

#define OPT_BRIEF               0
#define OPT_QUIET               1
#define OPT_DDF                 2
#define OPT_DDF_EQ              3
#define OPT_DEBUG               4
#define OPT_DEBUG_EQ            5
#define OPT_NAME_EQ             6
#define OPT_PASSWORD_EQ         7
#define OPT_SORT_EQ             8

/* "sort=" option value keywords. */

char *sort_keys[] =
 {      "ip",           "mac"
 };

#define SRT_IP          0
#define SRT_MAC         1

/*--------------------------------------------------------------------*/
/*    Functions. */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* usage(): Display usage information. */

int usage( void)
{
  int i;
  int sts = 0;

char *usage_text[] =
{
"   Usage: orvl [ options ] operation [ identifier ]",
"",
"Options:    debug[=value]       Set debug flags, all or selected.",
"            ddf[=file_spec]     Use device data file.  Default: ORVL_DDF",
"                                 (normally an env-var or logical name).",
"            name=device_name    New device name for \"set\" operation.",
"            password=passwd     New remote password for \"set\" operation.",
"            brief               Simplify [q]list and off/on reports.",
"            quiet               Suppress [q]list and off/on reports.",
"            sort={ip|mac}       Sort devs by IP or MAC addr.  Default: ip",
"",
"Operations: help, usage         Display this help/usage text.",
"            list                List devices.  (Minimal device queries.)",
"            qlist               Query and list devices.  (Query device(s)",
"                                 to get detailed device information.)",
"            off, on             Switch off/on.  Identifier required.",
"            set                 Set dev data (name, password).  Ident req'd.",
"            version             Show program version.",
"",
"Identifier: DNS name            DNS name, numeric IP address, or dev name.",
"            IP address          Used with operations \"off\", \"on\", or \"set\",",
"            Device name         or to limit a [q]list report to one device."
};

  fprintf( stderr, "\n");
  fprintf( stderr, "    %s %d.%d   %s\n",
   PROGRAM_NAME, PROGRAM_VERSION_MAJ, PROGRAM_VERSION_MIN,
   usage_text[ 0]);

  for (i = 1; i < (sizeof( usage_text)/ sizeof( *usage_text)); i++)
  {
    fprintf( stderr, "%s\n", usage_text[ i]);
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* dev_name(): Store a trimmed device name into user's buffer.
 * Return length in *name_len.
 */

char *dev_name( orv_data_t *orv_data_p, char *name_buf, int *name_len)
{
  int i;
  int last_non_blank = -1;

#define UNSET_NAME "(unset)"

  for (i = 0; i < DEV_NAME_LEN; i++)
  { /* Check first character for special values. */
    if ((orv_data_p->name)[ i] == 0xff)
    { /* Unset.  (Factory name = DEV_NAME_LEN* 0xff.) */
      if (name_buf != NULL)
      {
        memcpy( name_buf, UNSET_NAME, strlen( UNSET_NAME));
      }
      last_non_blank = strlen( UNSET_NAME)- 1;
      break;
    }
    else if ((orv_data_p->name)[ i] == '\0')
    { /* Unread.  (Our unread value is DEV_NAME_LEN* '\0'.) */
      break;
    }
    else
    { /* Normal name. */
      if ((orv_data_p->name)[ i] != ' ')
      {
        last_non_blank = i;
      }
      if (name_buf != NULL)
      {
        name_buf[ i] = (orv_data_p->name)[ i];
      }
    }
  }

  if (name_buf != NULL)
  {
    name_buf[ last_non_blank+ 1] = '\0';
  }
  if (name_len != NULL)
  {
    *name_len = last_non_blank+ 1;
  }
  return name_buf;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* orv_data_find_ip_addr(): Find an ip address (at LL orgn) in orv_data LL. */

orv_data_t *orv_data_find_ip_addr( orv_data_t *origin_p)
{
  orv_data_t *orv_data_p;
  orv_data_t *result = NULL;    /* Result = NULL, if not found. */

  orv_data_p = origin_p->next;  /* Start with the first (real?) LL member. */
  while (orv_data_p != origin_p)        /* Quit when back to origin. */
  {
    if (origin_p->ip_addr.s_addr == orv_data_p->ip_addr.s_addr)
    {
      if ((debug& DBG_SEL) != 0)
      {
        fprintf( stderr, " odfip().  Match: ip = %08x.\n",
         ntohl( orv_data_p->ip_addr.s_addr));
      }
      result = orv_data_p;      /* Found it.  Return this as result. */
      break;
    }
    orv_data_p = orv_data_p->next;      /* Advance to the next member. */
  }
  return result;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* orv_data_find_name(): Find device name in orv_data LL. */

orv_data_t *orv_data_find_name( orv_data_t *origin_p, char *name)
{
  int name_len;
  orv_data_t *orv_data_p;
  orv_data_t *result = NULL;    /* Result = NULL, if not found. */

  name_len = strlen( name);
  if ((debug& DBG_SEL) != 0)
  {
    fprintf( stderr, " odfn().  name_len = %d.\n", name_len);
  }

  orv_data_p = origin_p->next;  /* Start with the first (real?) LL member. */
  while (orv_data_p != origin_p)        /* Quit when back to origin. */
  {
    int orv_name_len;

    dev_name( orv_data_p, NULL, &orv_name_len); /* Get candidate name len. */

    if ((debug& DBG_SEL) != 0)
    {
      fprintf( stderr, " odfn().  orv_name_len = %d.\n", orv_name_len);
    }

    if (orv_name_len > 0)
    { /* orv_data_p->name is not NUL-terminated. */
      if ((name_len == orv_name_len) &&
       (strncmp( name, (char *)orv_data_p->name, name_len) == 0))
      {
        if ((debug& DBG_SEL) != 0)
        {
          fprintf( stderr, " odfn().  >>> Match: name = >%s<.\n", name);
        }
        result = orv_data_p;
        break;
      }
    }
    orv_data_p = orv_data_p->next;      /* Advance to the next member. */
  }
  return result;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* orv_data_find_dev(): Find device IP address or name in orv_data LL. */

int orv_data_find_dev( orv_data_t *origin_p, int use_ip, char *name, int loc)
{
  int sts = 0;                                  /* Assume success. */
  orv_data_t *orv_data_p;

  if (use_ip == 0)
  { /* Compare device names (name arg v. real LL data). */
    orv_data_p = orv_data_find_name( origin_p, name);
  }
  else
  { /* Compare IP addresses (LL origin v. real LL data). */
    orv_data_p = orv_data_find_ip_addr( origin_p);
  }

  if (orv_data_p == NULL)
  {
    fprintf( stderr,
     "%s: Device name not matched (loc=%d): >%s<.\n",
     PROGRAM_NAME, loc, name);
    errno = ENXIO;
    sts = EXIT_FAILURE;
  }
  else
  {
    orv_data_p->cnt_flg = 1;            /* Mark this member for reportng. */
  }

  if ((debug& DBG_SEL) != 0)
  {
    fprintf( stderr,
     " odfd().  sts = %d, use_ip = %d, loc = %d.  o_d_p = %sNULL.\n",
     sts, use_ip, loc, ((orv_data_p == NULL) ? "" : "non-"));
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* orv_data_find_mac(): Find MAC address in orv_data LL. */

orv_data_t *orv_data_find_mac( orv_data_t *origin_p, unsigned char *mac_addr)
{
  int sts;
  orv_data_t *orv_data_p;
  orv_data_t *result = NULL;    /* Result = NULL, if not found. */

  orv_data_p = origin_p->next;  /* Start with the first (real?) LL member. */
  while (orv_data_p != origin_p)        /* Quit when back to the origin. */
  {
    sts = memcmp( mac_addr, orv_data_p->mac_addr, MAC_ADDR_SIZE);
    if (sts == 0)
    {
      result = orv_data_p;      /* Found it.  Return this as result. */
      break;
    }
    orv_data_p = orv_data_p->next;      /* Advance to the next member. */
  }
  return result;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* mac_cmp(): Compare two MAC addresses. */

int mac_cmp( unsigned char *ma1, unsigned char *ma2)
{
  int i;
  int result = 0;

  for (i = 0; i < MAC_ADDR_SIZE; i++)
  {
    if (ma1[ i] < ma2[ i])
    {
      result = -1;
      break;
    }
    else if (ma1[ i] > ma2[ i])
    {
      result = 1;
      break;
    }
  }
  return result;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* orv_data_new(): Insert new member into orv_data LL. */

orv_data_t *orv_data_new( orv_data_t *origin_p, void *sort_p)
{
  orv_data_t *orv_data_p;
  orv_data_t *orv_data_new_p;

  /* Allocate storage for the new LL member. */
  orv_data_new_p = malloc( sizeof( orv_data_t));
  if (orv_data_new_p != NULL)
  {
    /* Find insertion point in LL, sorting by IP address (in host order). */
    orv_data_p = origin_p->next;        /* Start with first (real?) LL mmbr. */
    while (orv_data_p != origin_p)      /* Quit when back to the origin. */
    {
      if (origin_p->sort_key == SRT_IP)
      { /* Sort by IP address. */
        if (ntohl( ((struct in_addr *)sort_p)->s_addr) <
         ntohl( orv_data_p->ip_addr.s_addr))
        {
          break;                /* Insert new member before this member. */
        }
      }
      else
      { /* Sort by MAC address. */
        if (mac_cmp( (unsigned char *)sort_p, orv_data_p->mac_addr) < 0)
        {
          break;
        }
      }
      orv_data_p = orv_data_p->next;    /* Advance to the next member. */
    }

    origin_p->cnt_flg++;                        /* Count the new member. */

    /* Initialize new member data.
     * (Note: memset(0) sets sort_key to SRT_IP.)
     */
    memset( orv_data_new_p, 0, sizeof( orv_data_t));    /* Zero all data. */
    orv_data_new_p->type = -1;                  /* Set unknown device type. */
    orv_data_new_p->state = -1;                 /* Set unknown device state. */

    /* Insert the new member into the LL before the insertion-point member. */
    orv_data_new_p->next = orv_data_p;          /* New.next = Old. */
    orv_data_new_p->prev = orv_data_p->prev;    /* New.prev = Old.prev. */
    orv_data_p->prev->next = orv_data_new_p;    /* Old.prev.next = New. */
    orv_data_p->prev = orv_data_new_p;          /* Old.prev = New. */
  }
  return orv_data_new_p;                        /* Return pointer to New. */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* type_name(): Store a device type name into user's buffer. */

char *type_name( int type, char *type_buf)
{
  if ((type >= 0) && (type < ICON_MAX))
  {
    strcpy( type_buf, icon_name[ type]);
  }
  else if (type == -1)
  {
    strcpy( type_buf, "");
  }
  else
  {
    sprintf( type_buf, "?%04x?", type);
  }
  return type_buf;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* fprintf_device(): Display device data from LL member. */

int fprintf_device( FILE *fp, orv_data_t *orv_data_p)
{
  int bw = 0;                           /* Bytes written. */
  int bwt = 0;                          /* Bytes written, total. */
  int nam_len;                          /* Device name length. */
  char type_str[ 16];                   /* Device type string. */
  char ip_str[ 16];                     /* IP address string. */
  char nam_buf[ DEV_NAME_LEN+ 1];       /* Device name string. */
  unsigned int ia4;                     /* IP address (host order). */

  ia4 = ntohl( orv_data_p->ip_addr.s_addr);

  nam_buf[ 0] = '>';                            /* ">device name<". */
  dev_name( orv_data_p, &nam_buf[ 1], &nam_len);
  nam_buf[ nam_len+ 1] = '<';
  nam_buf[ nam_len+ 2] = '\0';

  sprintf( ip_str, "%u.%u.%u.%u",               /* IP address. */
   ((ia4/ 0x100/ 0x100/ 0x100)& 0xff),
   ((ia4/ 0x100/ 0x100)& 0xff),
   ((ia4/ 0x100)& 0xff),
   (ia4& 0xff));

  type_name( orv_data_p->type, type_str);
  bw = fprintf( fp,
   "%-15s  %02x:%02x:%02x:%02x:%02x:%02x  %-18s  # %s    %s\n",
   ip_str,                                              /* IP address. */
   orv_data_p->mac_addr[ 0], orv_data_p->mac_addr[ 1],  /* MAC address. */
   orv_data_p->mac_addr[ 2], orv_data_p->mac_addr[ 3],
   orv_data_p->mac_addr[ 4], orv_data_p->mac_addr[ 5],
   nam_buf,                                             /* ">dev name<". */
   ((orv_data_p->state == -1) ? "???" :         /* State string (Unk). */
   ((orv_data_p->state == 0) ? "Off" : "On ")), /* State string (0, 1). */
   type_str);                                           /* Device type. */

  if (bw >= 0)
  {
    bwt += bw;
  }
  else
  {
    bwt = -1;
  }
  return bwt;   /* Bytes written, total.  If error, then -1. */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* fprintf_device_header(): Display device list header. */

int fprintf_device_header( FILE *fp, int flags, orv_data_t *origin_p)
{
  int bw = 0;                           /* Bytes written. */
  int bwt = 0;                          /* Bytes written, total. */
  char dev_cnt_str[ 20];
  time_t t1;
  struct tm stm;

  time( &t1);
  LOCALTIME_R( &t1, &stm);
  sprintf( dev_cnt_str, "(%s: %d)",
   (((flags& FDL_DDF) != 0) ? "ddf" : "probe"),
   origin_p->cnt_flg);

  bw = fprintf( fp,
"#      %s %2d.%d  --  Devices %-18s  %04d-%02d-%02d:%02d:%02d:%02d\n",
   PROGRAM_NAME, PROGRAM_VERSION_MAJ, PROGRAM_VERSION_MIN,
   dev_cnt_str, (stm.tm_year+ 1900), (stm.tm_mon+ 1), stm.tm_mday,
   stm.tm_hour, stm.tm_min, stm.tm_sec);
  if (bw >= 0)
  {
    bwt += bw;
    bw = fprintf( fp,
"# IP address        MAC address     >Device name<       # State  Type\n");
  }
  if (bw >= 0)
  {
    bwt += bw;
    bw = fprintf( fp,
"#-----------------------------------------------------------------------\n");
  }
  if (bw >= 0)
  {
    bwt += bw;
  }
  if (bw < 0)
  {
    bwt = -1;
  }
  return bwt;   /* Bytes written, total.  If error, then -1. */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* fprintf_device_list(): Display device list. */

int fprintf_device_list( FILE *fp, int flags, orv_data_t *origin_p)
{
  orv_data_t *orv_data_p;
  int bw = 0;                           /* Bytes written. */
  int bwt = 0;                          /* Bytes written, total. */
  int header_written = 0;

  orv_data_p = origin_p->next;  /* Start with the first (real?) LL member. */
  while (orv_data_p != origin_p)        /* Quit when back to the origin. */
  {
    bwt += bw;
    bw = 0;

    /* If reporting only this device, then save its state (at origin). */
    if (orv_data_p->cnt_flg != 0)
    {
      origin_p->state = orv_data_p->state;
    }

    if ((flags& FDL_QUIET) == 0)                /* Not quiet. */
    {
      if (((flags& FDL_SINGLE) == 0) ||         /* Not specific, or: */
       (orv_data_p->cnt_flg != 0))              /* this one device. */
      {
        if ((header_written == 0) && ((flags& FDL_BRIEF) == 0))
        { /* Write the header once, before any device data. */
          header_written = 1;
          bw = fprintf_device_header( fp, flags, origin_p);
          if (bw < 0)
          {
            break;
          }
          bwt += bw;
        }
        bw = fprintf_device( fp, orv_data_p);
        if (bw < 0)
        {
          break;
        }
      }
    }
    orv_data_p = orv_data_p->next;      /* Advance to the next member. */
  } /* while */
  if (bw >= 0)
  {
    bwt += bw;
  }
  if (bw < 0)
  {
    bwt = -1;
  }
  return bwt;   /* Bytes written, total.  If error, then -1. */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* msg_dump(): Display message data. */

void msg_dump( unsigned char *buf, ssize_t len)
{
  ssize_t i;
  char dmp[ 17];

  for (i = 0; i < len; i++)
  {
    fprintf( stderr, " %02x", buf[ i]);
    if ((buf[ i] >= 32) && (buf[ i] <= 127))
    {
      dmp[ i% 16] = buf[ i];
    }
    else
    {
      dmp[ i% 16] = '.';
    }
    if ((i% 16) == 15)
    {
      dmp[ (i% 16)+ 1] = '\0';
      fprintf( stderr, "    >%s<\n", dmp);
    }
  }

  if ((i% 16) != 0)
  {
    dmp[ (i% 16)] = '\0';
  }

  for (i = len; i < (ssize_t)((len+ 15)& 0xfffffff0); i++)
  {
    fprintf( stderr, "   ");
  }
  fprintf( stderr, "    >%s<\n", dmp);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* hexchr2int(): Convert hex character to int value. */

int hexchr2int( char c)
{
  int val = -1;

  if ((c >= '0') && (c <= '9'))
  {
    val = c- '0';
  }
  else if ((c >= 'A') && (c <= 'F'))
  {
    val = c- 'A'+ 10;
  }
  else if ((c >= 'a') && (c <= 'f'))
  {
    val = c- 'a'+ 10;
  }
  return val;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* parse_mac(): Translate "xx:xx:xx:xx:xx:xx" string to byte array. */

int parse_mac( char *str, unsigned char *bytes)
{
  int i;
  int sts = 0;                                  /* Assume success. */

  if (strlen( str) != (MAC_ADDR_SIZE* 3- 1))
  {
    sts = -1;                           /* Wrong string length. */
  }
  else
  { /* Allow ":" or "-" as MAC octet separators. */
    for (i = 0; i < (MAC_ADDR_SIZE- 1); i++)
    {
      if ((str[ i* 3+ 2] != ':') && (str[ i* 3+ 2] != '-'))
      {
        sts = -2;                       /* Bad separator. */
        break;
      }
    }

    if (sts == 0)
    {
      for (i = 0; i < MAC_ADDR_SIZE; i++)
      {
        int c0;
        int c1;

        c1 = hexchr2int( str[ 3* i]);
        c0 = hexchr2int( str[ 3* i+ 1]);

        if ((c0 < 0) || (c1 < 0))
        {
          sts = -3;                     /* Invalid hex digit. */
          break;
        }
        else
        {
          bytes[ i] = 16* c1+ c0;
        }
      }
    }
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* revcpy(): Reverse memcpy(). */

void *revcpy( void *dst, const void *src, size_t siz)
{
  size_t i;

  for (i = 0; i < siz; i++)
  {
    ((unsigned char *)dst)[ i] = ((unsigned char *)src)[ siz- 1- i];
  }
  return dst;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* show_errno(): Display error code(s), message. */

int show_errno( char *prog_name)
{
  int sts;

  if (prog_name == NULL)
  {
    prog_name = "";
  }

#ifdef _WIN32
  sts = fprintf( stderr, "%s: errno = %d, WSAGLA() = %d.\n",
   prog_name, errno, WSAGetLastError());
#else /* def _WIN32 */
# ifdef VMS
  sts = fprintf( stderr, "%s: errno = %d, vaxc$errno = %08x.\n",
   prog_name, errno, vaxc$errno);
# else /* def VMS */
  sts = fprintf( stderr, "%s: errno = %d.\n", prog_name, errno);
# endif /* def VMS [else] */
#endif /* def _WIN32 [else]*/

  if (sts >= 0)
  {
    fprintf( stderr, "%s: %s\n", prog_name, strerror( errno));
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* dns_resolve(): Resolve DNS name to IPv4 address using getaddrinfo(). */

int dns_resolve( char *name, struct in_addr *ip_addr)
{
  int sts;
  struct addrinfo *ai_pp;       /* addrinfo() result (linked list). */
  struct addrinfo ai_h;         /* addrinfo() hints. */

  /* Fill getaddrinfo() hints structure. */
  memset( &ai_h, 0, sizeof( ai_h));
  ai_h.ai_family = AF_INET;
  ai_h.ai_protocol = IPPROTO_UDP;

  sts = getaddrinfo( name,              /* Node name. */
                     NULL,              /* Service name. */
                     &ai_h,             /* Hints. */
                     &ai_pp);           /* Results (linked list). */

  if (sts != 0)
  {
    if ((debug& DBG_DNS) != 0)
    {
      fprintf( stderr, " getaddrinfo() failed.\n");
    }
  }
  else
  { /* Use the first result in the ai_pp linked list. */
    if ((debug& DBG_DNS) != 0)
    {
      fprintf( stderr, " getaddrinfo():\n");
      fprintf( stderr,
       " ai_family = %d, ai_socktype = %d, ai_protocol = %d\n",
       ai_pp->ai_family, ai_pp->ai_socktype, ai_pp->ai_protocol);

      fprintf( stderr,
       " ai_addrlen = %d, ai_addr->sa_family = %d.\n",
       ai_pp->ai_addrlen, ai_pp->ai_addr->sa_family);

      fprintf( stderr,
       " ai_flags = 0x%08x.\n", ai_pp->ai_flags);

      if (ai_pp->ai_addr->sa_family == AF_INET)
      {
        fprintf( stderr, " ai_addr->sa_data[ 2: 5] = %u.%u.%u.%u\n",
         (unsigned char)ai_pp->ai_addr->sa_data[ 2],
         (unsigned char)ai_pp->ai_addr->sa_data[ 3],
         (unsigned char)ai_pp->ai_addr->sa_data[ 4],
         (unsigned char)ai_pp->ai_addr->sa_data[ 5]);
      }
    }

#ifndef NO_AF_CHECK     /* Disable the address-family check everywhere. */
    {
# if defined( VMS) && defined( VAX) && !defined( GETADDRINFO_OK)
      /* Work around apparent bug in getaddrinfo() in TCPIP V5.3 - ECO 4
       * on VMS V7.3 (typical hobbyist kit?).  sa_family seems to be
       * misplaced, misaligned, corrupted.  Spurious sa_len?
       */
      int sa_fam;
#  define SA_FAMILY sa_fam

      if (ai_pp->ai_addr->sa_family != AF_INET)
      {
        if ((debug& DBG_DNS) != 0)
        {
          fprintf( stderr,
           " Unexpected address family (not %d, V-V work-around): %d.\n",
           AF_INET, ai_pp->ai_addr->sa_family);
        }
        SA_FAMILY = ai_pp->ai_addr->sa_family/ 256;     /* Try offset. */
      }
# else /* defined( VMS) && defined( VAX) && !defined( GETADDRINFO_OK) */
#  define SA_FAMILY ai_pp->ai_addr->sa_family
# endif /* defined( VMS) && defined( VAX) && !defined( GETADDRINFO_OK) [else] */

      if (SA_FAMILY != AF_INET)
      { /* Complain about (original) bad value. */
        fprintf( stderr, "%s: Unexpected address family (not %d): %d.\n",
         PROGRAM_NAME, AF_INET, ai_pp->ai_addr->sa_family);
        sts = -1;
      }
    }
#endif /* ndef NO_AF_CHECK */

    if ((sts == 0) && (ip_addr != NULL))
    { /* IPv4 address (net order). */
      memcpy( ip_addr, &ai_pp->ai_addr->sa_data[ 2], sizeof( *ip_addr));
    }
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* set_socket_noblock(): Set socket to non-blocking. */

int set_socket_noblock( int sock)
{
#ifdef USE_FCNTL                        /* Use fcntl(). */

  int fc_flgs;
  int sts = 0;                                  /* Assume success. */

  fc_flgs = fcntl( sock, F_GETFL, 0);           /* Get current flags. */

  if ((debug& DBG_SIO) != 0)
  {
    fprintf( stderr, " fcntl(0).  Sock = %d, flgs = %08x .\n",
     sock, fc_flgs);
  }

  if (fc_flgs != -1)
  {
    fc_flgs |= O_NONBLOCK;                      /* OR-in O_NONBLOCK. */

    if ((debug& DBG_SIO) != 0)
    {
      fprintf( stderr, " fcntl(1). flgs = %08x .\n", fc_flgs);
    }

    fc_flgs = fcntl( sock, F_SETFL, fc_flgs);   /* Set revised flags.*/

    if ((debug& DBG_SIO) != 0)
    {
      fprintf( stderr, " fcntl(2). flgs = %08x .\n", fc_flgs);
    }
  }

  if (fc_flgs == -1)
  {
    fprintf( stderr, "%s: fcntl(noblock) failed.  sock = %d.\n",
     PROGRAM_NAME, sock);
    show_errno( PROGRAM_NAME);
    sts = -1;
  }

#else /* def USE_FCNTL */               /* Use ioctl(). */

  int ioctl_tmp;
  int sts;

  ioctl_tmp = 1;
  sts = IOCTL_SOCKET( sock, FIONBIO, &ioctl_tmp);       /* Set FIONBIO. */

  if ((debug& DBG_SIO) != 0)
  {
    fprintf( stderr, " ioctl() sts = %d, tmp = %08x .\n",
     sts, ioctl_tmp);
  }

  if (sts < 0)
  {
    fprintf( stderr, "%s: ioctl() failed.  sock = %d, sts = %d.\n",
     PROGRAM_NAME, sock, sts);
    show_errno( PROGRAM_NAME);
  }

#endif /* def USE_FCNTL [else] */

  return sts;                                   /* -1, if error. */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef VMS
# ifndef NO_OPER_PRIVILEGE

/* set_priv_oper(): Try to set OPER privilege according to oper_new.
 *                  Set *oper_old_p to old value.
 */

int set_priv_oper( int oper_new, int *oper_old_p)
{
  int sts;
  union prvdef priv_mask_old = { 0 };   /*  Privilege masks. */
  union prvdef priv_mask_new = { 0 };

  oper_new = (oper_new == 0 ? 0 : 1);   /* Ensure 0/1 flag value. */
  priv_mask_new.prv$v_oper = 1;         /* OPER privilege mask bit. */
  sts = sys$setprv( oper_new,           /* Enable/disable privileges. */
                    &priv_mask_new,     /* New privilege mask. */
                    0,                  /* Not permanent. */
                    &priv_mask_old);    /* Previous privilege mask. */

  if (sts == SS$_NORMAL)
  {
    sts = 0;
    if (oper_old_p != NULL)
    {
      *oper_old_p = priv_mask_old.prv$v_oper;
    }
  }
  else
  {
    errno = EVMSERR;
    vaxc$errno = sts;
  }
  return sts;
}

# endif /* ndef NO_OPER_PRIVILEGE */
#endif /* def VMS */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* form_msg_out(): Form an output message. */

size_t form_msg_out( int task_nr,
                     unsigned char **msg_out,
                     unsigned char *mac_addr)
{
  size_t msg_out_len = 0;
  char *str;

  /* TSK_WT_SOCKET uses modified "rt" data from device, not a message
   * created here.  TSK_WT_TIMING still not determined.
   */

  if ((task_nr >= TSK_MIN) && (task_nr <= TSK_MAX))
  {
    if ((task_nr == TSK_GLOB_DISC_B) || (task_nr == TSK_GLOB_DISC))
    { /* Global discovery. */
      str = "GLOB DISC";
      *msg_out = malloc( sizeof( cmd_glob_disc));
      if (*msg_out != NULL)
      {
        msg_out_len = sizeof( cmd_glob_disc);
        memcpy( *msg_out, cmd_glob_disc, msg_out_len);
      }
    }
    else if (task_nr == TSK_UNIT_DISC)
    { /* Unit discovery. */
      str = "UNIT DISC";
      *msg_out = malloc( sizeof( cmd_unit_disc));
      if (*msg_out != NULL)
      {
        msg_out_len = sizeof( cmd_unit_disc);
        memcpy( *msg_out, cmd_unit_disc, msg_out_len);
        if (mac_addr != NULL)
        {
          memcpy( (*msg_out+ 6), mac_addr, MAC_ADDR_SIZE);      /* MAC addr. */
        }
      }
    }
    else if (task_nr == TSK_HEARTBEAT)
    { /* Heartbeat. */
      str = "HEARTBEAT";
      *msg_out = malloc( sizeof( cmd_heartbeat));
      if (*msg_out != NULL)
      {
        msg_out_len = sizeof( cmd_heartbeat);
        memcpy( *msg_out, cmd_heartbeat, msg_out_len);
        if (mac_addr != NULL)
        {
          memcpy( (*msg_out+ 6), mac_addr, MAC_ADDR_SIZE);      /* MAC addr. */
        }
      }
    }
    else if (task_nr == TSK_SUBSCRIBE)
    { /* Subscribe. */
      str = "SUBSCRIBE";
      *msg_out = malloc( sizeof( cmd_subs));
      if (*msg_out != NULL)
      {
        msg_out_len = sizeof( cmd_subs);
        memcpy( *msg_out, cmd_subs, msg_out_len);
        if (mac_addr != NULL)
        {
          memcpy( (*msg_out+ 6), mac_addr, MAC_ADDR_SIZE);      /* MAC addr. */
          revcpy( (*msg_out+ 18), mac_addr, MAC_ADDR_SIZE);     /* MAC (rv). */
        }
      }
    }
    else if ((task_nr == TSK_SW_OFF) || (task_nr == TSK_SW_ON))
    { /* Switch off/on. */
      int val;

      if (task_nr == TSK_SW_OFF)
      {
        str = "SWITCH OFF";
        val = 0;
      }
      else /* (task_nr == TSK_SW_ON) */
      {
        str = "SWITCH ON";
        val = 1;
      }

      *msg_out = malloc( sizeof( cmd_switch));
      if (*msg_out != NULL)
      {
        msg_out_len = sizeof( cmd_switch);
        memcpy( *msg_out, cmd_switch, msg_out_len);
        if (mac_addr != NULL)
        {
          memcpy( (*msg_out+ 6), mac_addr, MAC_ADDR_SIZE);      /* MAC addr. */
          (*msg_out)[ 22] = val;
        }
      }
    }
    else if ((task_nr == TSK_RT_SOCKET) || (task_nr == TSK_RT_TIMING))
    { /* Read socket/timing data. */
      unsigned short tbl;
      unsigned short vers;

      if (task_nr == TSK_RT_SOCKET)
      {
        str = "RT-SOCKET";
        /* tbl = 0x0004; */
        /* vers = 0x0003; */
        tbl = 0x0004;
        vers = 0x0000;
      }
      else /* (task_nr == TSK_RT_TIMING) */
      {
        str = "RT-TIMING";
        tbl = 0x0003;
        vers = 0x0003;
      }

      *msg_out = malloc( sizeof( cmd_read_table));
      if (*msg_out != NULL)
      {
        msg_out_len = sizeof( cmd_read_table);
        memcpy( *msg_out, cmd_read_table, msg_out_len);
        if (mac_addr != NULL)
        {
          memcpy( (*msg_out+ 6), mac_addr, MAC_ADDR_SIZE);      /* MAC addr. */
        }

        tbl = 0x0004;
        vers = 0x0000;

        (*msg_out)[ READ_TABLE_TABLE] =      tbl% 256;
        (*msg_out)[ READ_TABLE_VERSION] =    vers% 256;
      }
    }

    /* Insert message length into message. */
    if (msg_out_len > 0)
    {
      (*msg_out)[ 2] = (unsigned short)msg_out_len/ 256;
      (*msg_out)[ 3] = (unsigned short)msg_out_len% 256;
    }

    if ((debug& DBG_MSO) != 0)
    {
      fprintf( stderr, " >> %s\n", str);
      fprintf( stderr, " msg_len = %ld.\n", msg_out_len);
      fprintf( stderr, " [2], [3] = %02x %02x\n",
       (*msg_out)[ 2], (*msg_out)[ 3]);

      /* Display message to be sent. */
      if (msg_out_len >= 6)
      {
        fprintf( stderr, "   Snd (%3ld)  %c  %c\n",
         msg_out_len, (*msg_out)[ 4], (*msg_out)[ 5]);
      }
      msg_dump( *msg_out, msg_out_len);
    }
  }
  return msg_out_len;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* task(): Perform a task: Send message, receive and process results. */

int task( int task_nr,                  /* Task number. */
          int *rsp_p,                   /* Response type bit mask. */
          unsigned char **tbl_p,        /* Table data. */
          orv_data_t *origin_p,         /* orv_data origin. */
          orv_data_t *target_p)         /* orv_data target. */
{
  unsigned char mac_addr[ MAC_ADDR_SIZE];       /* Orvibo MAC address. */
  SOCKET sock_orv = INVALID_SOCKET;     /* Orvibo device socket. */

  ssize_t bc;                   /* Byte count (send or receive). */
  int sts;                      /* Status. */

  int bcast = 0;                /* Broadcast message flag. */

  int mac_addr_ndx;
  unsigned char msg_inp[ 1024]; /* Receive message buffer. */
  unsigned char *msg_out;       /* Send message pointer. */
  RECVFROM_6 sock_addr_len_rec;
  ssize_t msg_out_len = 0;
  struct sockaddr_in sock_addr_rec;
  struct sockaddr_in sock_addr_snd;

  fd_set fds_rec;
  struct timeval timeout_rec;

  unsigned short countdown;
  unsigned short countdown_sts;
  short icon_code;
  int record_len;
  int record_nr;
  int version_id;
  int hw_version;
  int fw_version;
  int cc_version;
  int server_port;
  time_t time_dev;
  unsigned int server_ip;
  unsigned int local_ip;
  unsigned int local_gw;
  unsigned int local_nm;
  int remote_port;
  char remote_name[ 84];
  unsigned short table_nr;
  unsigned short unk_nr;

  char mac_addr_str[ 16];
  char uid_str[ 16];
  char remote_password[ PASSWORD_LEN+ 1];
  char device_name[ DEV_NAME_LEN+ 1];
  char device_type[ 12];

  int save_orv_data;
  int state_new;
  int state_old;

  orv_data_t *orv_data_p;

#ifdef VMS
  int oper_priv_save = -1;
#endif /* def VMS */

  sts = 0;

  if ((debug& DBG_MSI) != 0)
  {
    fprintf( stderr,
     " task(beg).  task_nr = %d, tbl_p = %sNULL.\n",
     task_nr, ((tbl_p == NULL) ? "" : "non-"));
  }

  /* Form command message according to task_nr. */

  if (sts == 0)
  {
    /* Broadcast IP, specific IP, or specific MAC address? */
    if (task_nr == TSK_GLOB_DISC_B)
    { /* Broadcast IP. */
      bcast = 1;
    }
    else if (task_nr != TSK_GLOB_DISC)
    { /* Specific MAC address.  (Not TSK_GLOB_DISC[_B].) */
      memcpy( mac_addr, target_p->mac_addr, MAC_ADDR_SIZE);
    }

    /* Form the message for this task_nr, or point to an already
     * existing message.
     */
    if ((task_nr == TSK_WT_SOCKET) || (task_nr == TSK_WT_TIMING))
    {
      if (*tbl_p != NULL)
      { /* Use an existing (Write-Table) message. */
        msg_out = *tbl_p;
        msg_out_len = (unsigned short)msg_out[ 2]* 256+
                      (unsigned short)msg_out[ 3];

        if ((debug& DBG_MSO) != 0)
        {
          fprintf( stderr, " task(non-f_m_o).  m_o_l = %ld.\n",
           msg_out_len);
        }
      }
    }
    else
    { /* Form a new message. */
      msg_out_len = form_msg_out( task_nr, &msg_out, mac_addr);

      if ((debug& DBG_MSO) != 0)
      {
        fprintf( stderr, " task(f_m_o).  m_o_l = %ld.\n",
         msg_out_len);
      }

      if (msg_out_len <= 0)
      {
        errno = EINVAL;
        sts = -1;
      }
    }
  }

#ifdef VMS
# ifndef NO_OPER_PRIVILEGE
  /* On VMS, broadcast may require BYPASS, OPER, or SYSPRV privilege,
   * unless TCPIP SET PROTOCOL UDP /BROADCAST.  We try to enable only
   * OPER, which may be relatively safe.
   *
   * As of TCPIP V5.7 - ECO 5 on Alpha, privilege must be adequate
   * (elevated) when the socket is created.  If raised after socket(),
   * then setsockopt( SO_BROADCAST) fails (EACCES).
   */

  if (sts == 0)
  {
    if (bcast != 0)
    {
      sts = set_priv_oper( 1, &oper_priv_save);

      if (sts != 0)
      {
        fprintf( stderr,
         "%s: Set privilege (OPER) failed.  sts = %%x%08x .\n",
         PROGRAM_NAME, sts);
        show_errno( PROGRAM_NAME);
        sts = 0;                        /* Try to continue. */
      }
      else if ((debug& DBG_VMS) != 0)
      {
        fprintf( stderr,
         " set_priv_oper(set).  sts = %%x%08x , oper_old = %d.\n",
         sts, oper_priv_save);
      }
    }
  }
# endif /* ndef NO_OPER_PRIVILEGE */
#endif /* def VMS */

  state_new = -1;                       /* Clear device states. */
  state_old = -1;

  if (sts == 0)
  {
    /* Fill receive socket addr structure. */
    memset( &sock_addr_rec, 0, sizeof( sock_addr_rec));
    sock_addr_rec.sin_family      = AF_INET;
    sock_addr_rec.sin_port        = htons( PORT_ORV);
    sock_addr_rec.sin_addr.s_addr = htons( INADDR_ANY);

    sock_orv = socket( AF_INET,                 /* Address family. */
                       SOCK_DGRAM,              /* Type. */
                       IPPROTO_UDP);            /* Protocol. */

    if ((debug& DBG_SIO) != 0)
    {
      fprintf( stderr, " sock_orv = %d.\n", sock_orv);
    }

    if (BAD_SOCKET( sock_orv))
    {
      fprintf( stderr, "%s: socket() failed.\n", PROGRAM_NAME);
      show_errno( PROGRAM_NAME);
      sts = -1;
    }
    else
    {
#ifdef _WIN32
      char sock_opt_rec = 1;
#else /* def _WIN32 */
      unsigned int sock_opt_rec = 1;
#endif /* def _WIN32 [else] */

      sts = setsockopt( sock_orv,                       /* Socket. */
                        SOL_SOCKET,                     /* Level. */
                        SO_REUSEADDR,                   /* Option name. */
                        &sock_opt_rec,                  /* Option value. */
                        sizeof( sock_opt_rec));         /* Option length. */

      if (sts < 0)
      {
        fprintf( stderr, "%s: setsockopt(rec) failed.\n", PROGRAM_NAME);
        show_errno( PROGRAM_NAME);
      }
      else if ((debug& DBG_SIO) != 0)
      {
        fprintf( stderr, " setsockopt(rec) sts = %d.\n", sts);
      }
    }

    if (sts == 0)
    {
      sts = set_socket_noblock( sock_orv);
    }

    if (sts == 0)
    {
      sts = bind( sock_orv,
                  (struct sockaddr *)
                   &sock_addr_rec,              /* Socket address. */
                  sizeof( sock_addr_rec));      /* Socket address length. */

      if (sts < 0)
      {
        fprintf( stderr, "%s: bind(rec) failed.\n", PROGRAM_NAME);
        show_errno( PROGRAM_NAME);
      }
      else if ((debug& DBG_SIO) != 0)
      {
        fprintf( stderr, " bind(rec) sts = %d.\n", sts);
      }

      {
#ifdef EARLY_RECVFROM                           /* Useful or not? */
        if ((debug& DBG_SIO) != 0)
        {
          fprintf( stderr,
           " pre-recvfrom(e).  sock_orv = %d, siz = %ld.\n",
           sock_orv, sizeof( msg_inp));
        }

        /* Read a response.  (Should be too soon.) */
        sock_addr_len_rec = sizeof( sock_addr_rec);     /* Socket addr len. */
        bc = recvfrom( sock_orv,
                       msg_inp,
                       sizeof( msg_inp),
                       0,                       /* Flags (not OOB or PEEK). */
                       (struct sockaddr *)
                        &sock_addr_rec,         /* Socket address. */
                       &sock_addr_len_rec);     /* Socket address length. */

        if (bc < 0)
        {
# ifdef _WIN32
          if (WSAGetLastError() != WSAEWOULDBLOCK)
# else /* def _WIN32 */
          if (errno != EWOULDBLOCK)
# endif /* def _WIN32 [else] */
          {
            fprintf( stderr, "%s: recvfrom(e) failed.\n", PROGRAM_NAME);
            show_errno( PROGRAM_NAME);
          }
        }
        else if ((debug& DBG_SIO) != 0)
        {
          fprintf( stderr, " recvfrom(e) bc = %ld.\n", bc);
        }
#endif /* def EARLY_RECVFROM */
      }
    }
  }

  if (sts == 0)
  {
    bc = -1;
    /* Fill send socket addr structure. */
    memset( &sock_addr_snd, 0, sizeof( sock_addr_snd));
    sock_addr_snd.sin_family = AF_INET;
    sock_addr_snd.sin_port = htons( PORT_ORV);
    sock_addr_snd.sin_addr.s_addr = target_p->ip_addr.s_addr;

    if (bcast != 0)
    {
      /* Set socket broadcast flag. */

#ifdef _WIN32
      char sock_opt_snd = 1;
#else /* def _WIN32 */
      unsigned int sock_opt_snd = 1;
#endif /* def _WIN32 [else] */

      sts = setsockopt( sock_orv,                       /* Socket. */
                        SOL_SOCKET,                     /* Level. */
                        SO_BROADCAST,                   /* Option name. */
                        &sock_opt_snd,                  /* Option value. */
                        sizeof( sock_opt_snd));         /* Option length. */

      if ((debug& DBG_SIO) != 0)
      {
        fprintf( stderr, " setsockopt( snd-bc0) = %d .\n", sts);
      }

      if (sts < 0)
      {
        fprintf( stderr, "%s: setsockopt( snd-bc1) failed.\n",
         PROGRAM_NAME);
        show_errno( PROGRAM_NAME);
      }
      else
      {
        /* Disable multicast loopback.  (Ineffective?) */
        unsigned char sock_opt_snd = 0;

        sts = setsockopt( sock_orv,                     /* Socket. */
                          IPPROTO_IP,                   /* Level. */
                          IP_MULTICAST_LOOP,            /* Option name. */
                          &sock_opt_snd,                /* Option value. */
                          sizeof( sock_opt_snd));       /* Option length. */

        if (sts < 0)
        {
          fprintf( stderr, "%s: setsockopt( snd-lb) failed.\n",
           PROGRAM_NAME);
          show_errno( PROGRAM_NAME);
        }
      }
    }

    if (sts == 0)
    { /* Send the command. */
      bc = sendto( sock_orv,                    /* Socket. */
                   msg_out,                     /* Message. */
                   msg_out_len,                 /* Message length. */
                   0,                           /* Flags (not MSG_OOB). */
                   (struct sockaddr *)
                    &sock_addr_snd,             /* Socket address. */
                   sizeof( sock_addr_snd));     /* Socket address length. */

      if (bc < 0)
      {
        fprintf( stderr, "%s: sendto() failed.\n", PROGRAM_NAME);
        show_errno( PROGRAM_NAME);
        sts = -1;
      }
      else if ((debug& DBG_SIO) != 0)
      {
       fprintf( stderr, " sendto() = %ld.\n", bc);
      }
    }
  }

  if ((sts == 0) && (bc >= 0))
  {
    /* Fill file-descriptor flags and time-out value for select(). */
    memset( &fds_rec, 0, sizeof( fds_rec));
    FD_SET( sock_orv, &fds_rec);
    timeout_rec.tv_sec  = SOCKET_TIMEOUT/ 1000000;      /* Seconds. */
    timeout_rec.tv_usec = SOCKET_TIMEOUT% 1000000;      /* Microseconds. */

    if ((debug& DBG_SIO) != 0)
    {
      fprintf( stderr, " sock_orv = %d, FD_SETSIZE = %d,\n",
       sock_orv, FD_SETSIZE);

      fprintf( stderr, " FD_ISSET( sock_orv, &fds_rec) = %d.\n",
       FD_ISSET( sock_orv, &fds_rec));
    }
  }

  /* Read responses until recvfrom()/select() times out. */
  while (bc >= 0)
  {
    save_orv_data = 0;                  /* Clear good-data flag. */

    if ((debug& DBG_SIO) != 0)
    {
      fprintf( stderr, " pre-select(1).  sock_orv = %d.\n", sock_orv);
    }

    sts = select( FD_SETSIZE, &fds_rec, NULL, NULL, &timeout_rec);

    if (sts <= 0)
    {
      if (sts < 0)
      {
        fprintf( stderr, "%s: select(1) failed.\n", PROGRAM_NAME);
        show_errno( PROGRAM_NAME);
      }
      else if ((debug& DBG_SIO) != 0)
      {
        fprintf( stderr, " select(1) sts = %d.\n", sts);
      }
      break;
    }
    else
    {
      if ((debug& DBG_SIO) != 0)
      {
        fprintf( stderr, " pre-recvfrom(n).  sock_orv = %d.\n",
         sock_orv);
      }

      /* Receive a response. */
      sock_addr_len_rec = sizeof( sock_addr_rec);       /* Socket addr len. */
      bc = recvfrom( sock_orv,
                     msg_inp,
                     sizeof( msg_inp),
                     0,                         /* Flags (not OOB or PEEK). */
                     (struct sockaddr *)
                      &sock_addr_rec,           /* Socket address. */
                     &sock_addr_len_rec);       /* Socket address length. */

      if (bc < 0)
      {
        fprintf( stderr, "%s: recvfrom(n) failed.\n", PROGRAM_NAME);
        show_errno( PROGRAM_NAME);
      }
      else
      {
        mac_addr_ndx = -1;

        if ((debug& DBG_SIO) != 0)
        {
          if (sock_addr_rec.sin_family == AF_INET)
          {
            unsigned int ia4;

            fprintf( stderr, " s_addr: %08x .\n",
             sock_addr_rec.sin_addr.s_addr);

            ia4 = ntohl( sock_addr_rec.sin_addr.s_addr);
            fprintf( stderr, " recvfrom(n) addr: %u.%u.%u.%u\n",
             ((ia4/ 0x100/ 0x100/ 0x100)& 0xff),
             ((ia4/ 0x100/ 0x100)& 0xff),
             ((ia4/ 0x100)& 0xff),
             (ia4& 0xff));
          }
        }

        if ((debug& DBG_MSI) != 0)
        {
          /* Display the response (hex, ASCII). */

          if (bc >= 6)
          {
            fprintf( stderr, "   Rec (%3ld)  %c  %c\n",
             bc, msg_inp[ 4], msg_inp[ 5]);
          }

          msg_dump( msg_inp, bc);
        }

        /* Find MAC.  Determine Off/On state, if known. */

        if (bc >= 6)
        {
          if ((msg_inp[ 4] == 'c') && (msg_inp[ 5] == 'l'))
          { /* Subscribe ("cl"). */
            *rsp_p |= RSP_CL;
            if (bc >= 12)
            {
              mac_addr_ndx = 6;
            }
            if (bc >= 24)
            {
              state_old = msg_inp[ 23];
              save_orv_data = RSP_CL;           /* Found good data (cl). */
            }
          }
          else if ((msg_inp[ 4] == 'd') && (msg_inp[ 5] == 'c'))
          { /* Switch Off/On (message) ("dc").  (No useful data?) */
            *rsp_p |= RSP_DC;
          }
          else if ((msg_inp[ 4] == 'h') && (msg_inp[ 5] == 'b'))
          { /* Heartbeat Off/On ("hb").  (No useful data?) */
            *rsp_p |= RSP_HB;
          }
          else if ((msg_inp[ 4] == 'q') && (msg_inp[ 5] == 'a'))
          { /* Global Discovery ("qa"). */
            /* We may see the original (short) request, if broadcast,
             * so set the "qa" response bit only if the message is a
             * real response, that is, if it's long enough to include
             * a MAC address (which a "qa" request does not).
             */
            if (bc >= 13)
            {
              *rsp_p |= RSP_QA;
              mac_addr_ndx = 7;
            }
            if (bc >= 41)
            {
              time_dev =                        /* Casts ensure that */
               (((time_t)msg_inp[ 40]* 256+     /* arithmetic is done as */
                 (time_t)msg_inp[ 39])* 256+    /* time_t, which may be */
                 (time_t)msg_inp[ 38])* 256+    /* 32/64-bit, [un]signed */
                 (time_t)msg_inp[ 37];          /* on different systems. */

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " time_dev: %lu (0x%08lx)\n", time_dev, time_dev);

                time_dev -= TIME_OFS;                   /* 1970 - 1900. */
                fprintf( stderr, " ctime(adj) = %s",    /* No '\n'. */
                 ctime( (time_t *)&time_dev));
              }
            }
            if (bc >= 42)
            {
              state_old = msg_inp[ 41];
              save_orv_data = RSP_QA;           /* Found good data (qa). */
            }
          }
          else if ((msg_inp[ 4] == 'q') && (msg_inp[ 5] == 'g'))
          { /* Unit Discovery ("qg"). */
            *rsp_p |= RSP_QG;
            if (bc >= 13)
            {
              mac_addr_ndx = 7;
            }
            if (bc >= 41)
            {
              time_dev =                        /* Casts ensure that */
               (((time_t)msg_inp[ 40]* 256+     /* arithmetic is done as */
                 (time_t)msg_inp[ 39])* 256+    /* time_t, which may be */
                 (time_t)msg_inp[ 38])* 256+    /* 32/64-bit, [un]signed */
                 (time_t)msg_inp[ 37];          /* on different systems. */

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " time_dev: %lu (0x%08lx)\n", time_dev, time_dev);

                time_dev -= TIME_OFS;                   /* 1970 - 1900. */
                fprintf( stderr, " ctime(adj) = %s\n",
                 ctime( (time_t *)&time_dev));
              }
            }
            if (bc >= 42)
            {
              state_old = msg_inp[ 41];
              save_orv_data = RSP_QG;           /* Found good data (qg). */
            }
          }
          else if ((msg_inp[ 4] == 'r') && (msg_inp[ 5] == 't'))
          { /* Read Table ("rt"). */
            *rsp_p |= RSP_RT;

            /* If caller wants them, then copy msg_inp data into user's
             * new buffer, and tell caller where to find them.  (Size is
             * stored in the data.)
             */
            if (tbl_p != NULL)
            {
              if (*tbl_p == NULL)              /* First time. */
              {
                unsigned short msg_len;        /* Embedded msg len. */

                msg_len = (unsigned short)msg_inp[ 2]* 256+
                          (unsigned short)msg_inp[ 3];

                if (bc != msg_len)
                {
                  fprintf( stderr,
                   "%s: Unexpected message length.  bc = %ld, m_l = %d.\n",
                   PROGRAM_NAME, bc, msg_len);
                }
                else
                {
                  *tbl_p = malloc( bc);
                  if (*tbl_p == NULL)
                  {
                    fprintf( stderr, "%s: malloc() failed [x].\n",
                     PROGRAM_NAME);
                  }
                  else
                  {
                    memcpy( *tbl_p, msg_inp, bc);
                  }
                }
              }
            }

            if (bc >= 12)
            {
              mac_addr_ndx = 6;
            }
#if 0
            if (bc >= 20)
            {
              int record_id;

              record_id =                       /* Record ID. */
               (unsigned int)msg_inp[ 19]* 256+
               (unsigned int)msg_inp[ 18];

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " Record ID: %u (0x%04x)\n", record_id, record_id);
              }
            }
#endif /* 0 */
            if (bc >= 25)                       /* Unk: 18, 19, 20, 21, 22. */
            {
              table_nr =                        /* Table Nr. */
               (unsigned int)msg_inp[ 24]* 256+
               (unsigned int)msg_inp[ 23];

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " Table Nr: %u (0x%04x)\n", table_nr, table_nr);
              }
            }
            if (bc >= 27)
            {
              unk_nr =                          /* Unk Nr. */
               (unsigned int)msg_inp[ 26]* 256+
               (unsigned int)msg_inp[ 25];

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " Unk Nr:   %u (0x%04x)\n", unk_nr, unk_nr);
              }
            }                                   /* Unk: 27. */
            if (bc >= 30)
            { /* (Record length is bytes to follow: total - 30.) */
              record_len =                      /* Record length. */
               (unsigned int)msg_inp[ 29]* 256+
               (unsigned int)msg_inp[ 28];

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " Record Len: %u (0x%04x)\n", record_len, record_len);
              }
            }
            if (bc >= 32)
            {
              record_nr =                       /* Record number. */
               (unsigned int)msg_inp[ 31]* 256+
               (unsigned int)msg_inp[ 30];

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " Record Nr: %u (0x%04x)\n", record_nr, record_nr);
              }
            }
            if (bc >= 34)
            {
              version_id =                      /* Version ID. */
               (unsigned int)msg_inp[ 33]* 256+
               (unsigned int)msg_inp[ 32];

              if ((debug& DBG_MSI) != 0)
              {
                fprintf( stderr,
                 " Version ID: %u (0x%04x)\n", version_id, version_id);
              }
            }
            if (table_nr == 4)
            {
              if (bc >= 46)
              { /* 6-char UID (MAC address)+ 6* 0x20. */
                memcpy( uid_str, &msg_inp[ 34], 12);
                uid_str[ 12] = '\0';

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " MAC address (fwd): %02x:%02x:%02x:%02x:%02x:%02x\n",
                   msg_inp[ 34], msg_inp[ 35], msg_inp[ 36],
                   msg_inp[ 37], msg_inp[ 38], msg_inp[ 39]);
                }
              }
              if (bc >= 58)
              { /* 6-char MAC addr+ 6* 0x20. */
                memcpy( mac_addr_str, &msg_inp[ 46], 12);
                mac_addr_str[ 12] = '\0';

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " MAC address (rev): %02x:%02x:%02x:%02x:%02x:%02x\n",
                   msg_inp[ 46], msg_inp[ 47], msg_inp[ 48],
                   msg_inp[ 49], msg_inp[ 50], msg_inp[ 51]);
                }
              }
              if (bc >= 70)
              { /* 12-char remote password (0x20-padded). */
                memcpy( remote_password, &msg_inp[ 58], PASSWORD_LEN);
                remote_password[ PASSWORD_LEN] = '\0';

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " Remote password: >%s<\n", remote_password);
                }
              }
              if (bc >= 86)
              { /* 16-char device name (0x20 padded). */
                memcpy( device_name, &msg_inp[ 70], DEV_NAME_LEN);
                device_name[ DEV_NAME_LEN] = '\0';

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Device name: >%s<\n", device_name);
                }
              }
              if (bc >= 88)
              { /* Device type ("icon_code"). */
                icon_code =                     /* Icon code. */
                 (unsigned int)msg_inp[ 87]* 256+
                 (unsigned int)msg_inp[ 86];

                if ((debug& DBG_MSI) != 0)
                {
                  type_name( icon_code, device_type);
                  fprintf( stderr, " Device type: %d  (%s)\n",
                   icon_code, device_type);
                 }
              }
              if (bc >= 92)
              {
                hw_version =                    /* Hardware Version. */
                 (unsigned int)msg_inp[ 91]* 256* 256* 256+
                 (unsigned int)msg_inp[ 90]* 256* 256+
                 (unsigned int)msg_inp[ 89]* 256+
                 (unsigned int)msg_inp[ 88];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " Hardware version:        0x%08x\n", hw_version);
                }
              }
              if (bc >= 96)
              {
                fw_version =                    /* Firmware Version. */
                 (unsigned int)msg_inp[ 95]* 256* 256* 256+
                 (unsigned int)msg_inp[ 94]* 256* 256+
                 (unsigned int)msg_inp[ 93]* 256+
                 (unsigned int)msg_inp[ 92];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " Firmware version:        0x%08x\n", fw_version);
                }
              }
              if (bc >= 100)
              {
                cc_version =                    /* CC3300 Firmware Version. */
                 (unsigned int)msg_inp[ 99]* 256* 256* 256+
                 (unsigned int)msg_inp[ 98]* 256* 256+
                 (unsigned int)msg_inp[ 97]* 256+
                 (unsigned int)msg_inp[ 96];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " CC3300 Firmware version: 0x%08x\n", cc_version);
                }
              }
              if (bc >= 102)
              {
                server_port =                   /* Server port. */
                 (unsigned int)msg_inp[ 101]* 256+
                 (unsigned int)msg_inp[ 100];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Server port: code: %d (0x%04x)\n",
                   server_port, server_port);
                }
              }
              if (bc >= 106)
              {
                server_ip =                     /* Server IP address. */
                 (unsigned int)msg_inp[ 102]* 256* 256* 256+
                 (unsigned int)msg_inp[ 103]* 256* 256+
                 (unsigned int)msg_inp[ 104]* 256+
                 (unsigned int)msg_inp[ 105];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " Server IP address: 0x%08x %u.%u.%u.%u\n",
                   server_ip, msg_inp[ 102], msg_inp[ 103],
                   msg_inp[ 104], msg_inp[ 105]);
                }
              }
              if (bc >= 108)
              {
                remote_port =                   /* Remote port. */
                 (unsigned int)msg_inp[ 107]* 256+
                 (unsigned int)msg_inp[ 106];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Remote port: %d (0x%04x)\n",
                   remote_port, remote_port);
                }
              }
              if (bc >= 148)
              {
                memcpy( remote_name, &msg_inp[ 108], 40);
                remote_name[ 40] = '\0';

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Remote name: >%s<\n", remote_name);
                }
              }
              if (bc >= 152)
              {
                local_ip =                      /* Local IP address. */
                 (unsigned int)msg_inp[ 148]* 256* 256* 256+
                 (unsigned int)msg_inp[ 149]* 256* 256+
                 (unsigned int)msg_inp[ 150]* 256+
                 (unsigned int)msg_inp[ 151];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " Local IP address: 0x%08x %u.%u.%u.%u\n",
                   local_ip, msg_inp[ 148], msg_inp[ 149],
                   msg_inp[ 150], msg_inp[ 151]);
                }
              }
              if (bc >= 156)
              {
                local_gw =                      /* Local gateway address. */
                 (unsigned int)msg_inp[ 152]* 256* 256* 256+
                 (unsigned int)msg_inp[ 153]* 256* 256+
                 (unsigned int)msg_inp[ 154]* 256+
                 (unsigned int)msg_inp[ 155];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " Local GW address: 0x%08x %u.%u.%u.%u\n",
                   local_gw, msg_inp[ 152], msg_inp[ 153],
                   msg_inp[ 154], msg_inp[ 155]);
                }
              }
              if (bc >= 160)
              {
                local_nm =                      /* Local netmask. */
                 (unsigned int)msg_inp[ 156]* 256* 256* 256+
                 (unsigned int)msg_inp[ 157]* 256* 256+
                 (unsigned int)msg_inp[ 158]* 256+
                 (unsigned int)msg_inp[ 159];

                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr,
                   " Local Netmask:    0x%08x %u.%u.%u.%u\n",
                   local_nm, msg_inp[ 156], msg_inp[ 157],
                   msg_inp[ 158], msg_inp[ 159]);
                }
              }
              if (bc >= 161)
              {
                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " DHCP: %d (0x%02x)\n",
                    msg_inp[ 160], msg_inp[ 160]);
                }
              }
              if (bc >= 162)
              {
                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Discoverable: %d (0x%02x)\n",
                   msg_inp[ 161], msg_inp[ 161]);
                }
              }
              if (bc >= 163)
              {
                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Time zone set: %d (0x%02x)\n",
                   msg_inp[ 162], msg_inp[ 162]);
                }
              }
              if (bc >= 164)
              {
                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Time zone: %d (0x%02x)\n",
                   (char)msg_inp[ 163], msg_inp[ 163]);
                }
              }
              if (bc >= 166)
              {
                countdown_sts =                 /* Countdown status. */
                 (unsigned short)msg_inp[ 165]* 256+
                 (unsigned short)msg_inp[ 164];
                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Countdown status: %d (0x%04x)\n",
                   countdown_sts, countdown_sts);
                }
              }
              if (bc >= 168)
              {
                countdown =                     /* Countdown (s). */
                 (unsigned short)msg_inp[ 167]* 256+
                 (unsigned short)msg_inp[ 166];
                if ((debug& DBG_MSI) != 0)
                {
                  fprintf( stderr, " Countdown: %d (0x%04x)\n",
                   countdown, countdown);
                }
                save_orv_data = RSP_RT;         /* Found good data (rt). */
              }
            } /* table_nr == 4 */
          }
          else if ((msg_inp[ 4] == 's') && (msg_inp[ 5] == 'f'))
          { /* Device control (Switch Off/On) ("sf"). */
            *rsp_p |= RSP_SF;
            if (bc >= 12)
            {
              mac_addr_ndx = 6;
            }
            if (bc >= 23)
            {
              state_new = msg_inp[ 22];
              save_orv_data = RSP_SF;           /* Found good data (sf). */
            }
          }
          else if ((msg_inp[ 4] == 't') && (msg_inp[ 5] == 'm'))
          { /* Write Table ("tm"). */
            *rsp_p |= RSP_TM;
            if (bc >= 12)
            {
              mac_addr_ndx = 6;
            }
            if (bc >= 23)
            { /* We expect 23 bytes.  Can't really tell good from bad. */
#if 0
              save_orv_data = RSP_TM;           /* Found good data (tm). */
#endif /* 0 */
            }
          }
          else
          { /* Unknown. */
            *rsp_p |= RSP___;
            if ((debug& DBG_MSI) != 0)
            {
              fprintf( stderr,
               " Unexpected response: \"%c%c\" (%.1x%.1x).\n",
               msg_inp[ 4], msg_inp[ 5], msg_inp[ 4], msg_inp[ 5]);
            }
          }

          if ((debug& DBG_MSI) != 0)
          {
             if ((state_old >= 0) || (state_new >= 0))
             {
              char state_new_str[ 16];
              char state_old_str[ 16];

              sprintf( state_new_str, "%2.2x", state_new);
              sprintf( state_old_str, "%2.2x", state_old);

              fprintf( stderr, "   States: old = %s, new = %s.\n",
               ((state_old < 0) ? "??" : state_old_str),
               ((state_new < 0) ? "??" : state_new_str));
            }

            if (mac_addr_ndx >= 0)
            {
              int i;

              fprintf( stderr, "   MAC addr: ");
              for (i = mac_addr_ndx; i < mac_addr_ndx+ MAC_ADDR_SIZE; i++)
              {
                fprintf( stderr, "%02x", msg_inp[ i]);
                if (i < mac_addr_ndx+ MAC_ADDR_SIZE- 1)
                {
                  fprintf( stderr, ":");
                }
                if (i == mac_addr_ndx+ MAC_ADDR_SIZE- 1)
                {
                  fprintf( stderr, "\n");
                }
              }
            }
          }
        }
      }
    }
    if ((save_orv_data > 0) && (mac_addr_ndx >= 0))
    {
      orv_data_p = orv_data_find_mac( origin_p,
                                      &msg_inp[ mac_addr_ndx]);

      if ((debug& DBG_MSI) != 0)
      {
        fprintf( stderr, " s_o_d = %d, o_f_d_m() = %sNULL.\n",
         save_orv_data, ((orv_data_p == NULL)  ? "" : "non-"));
      }

      if (orv_data_p == NULL)
      {
        orv_data_p = orv_data_new( origin_p,
         ((origin_p->sort_key == SRT_IP) ?
         (void *)&sock_addr_rec.sin_addr :              /* IP address. */
         (void *)&msg_inp[ mac_addr_ndx]));             /* MAC address. */

        if (orv_data_p == NULL)
        {
          fprintf( stderr, "%s: malloc() failed [1].\n", PROGRAM_NAME);
        }
        else
        {
          memcpy( orv_data_p->mac_addr, &msg_inp[ mac_addr_ndx],
           MAC_ADDR_SIZE);
          orv_data_p->ip_addr.s_addr = sock_addr_rec.sin_addr.s_addr;
        }
      }
      else if (save_orv_data == RSP_RT)
      { /* Have Read Table (detailed) data. */
        memcpy( orv_data_p->passwd, remote_password, PASSWORD_LEN);
        memcpy( orv_data_p->name, device_name, DEV_NAME_LEN);
        orv_data_p->type = icon_code;
        orv_data_p->port = server_port;
      }
      if (state_new >= 0)
      {
        orv_data_p->state = state_new;
      }
      else if (state_old >= 0)
      {
        orv_data_p->state = state_old;
      }
    }
  }

#ifdef VMS
# ifndef NO_OPER_PRIVILEGE
  /* Restore original OPER privilege state, if elevated. */
  if (oper_priv_save == 0)      /* Was set (not -1), and initial was zero. */
  {
    sts = set_priv_oper( 0, &oper_priv_save);

    if (sts != 0)
    {
      fprintf( stderr,
       "%s: Remove privilege (OPER) failed.  sts = %%x%08x .\n",
       PROGRAM_NAME, sts);
      show_errno( PROGRAM_NAME);
    }
    else if ((debug& DBG_VMS) != 0)
    {
      fprintf( stderr,
       " set_priv_oper(restore).  sts = %%x%08x , oper_old = %d.\n",
       sts, oper_priv_save);
    }
  }
# endif /* ndef NO_OPER_PRIVILEGE */
#endif /* def VMS */

  if (!BAD_SOCKET( sock_orv))
  {
    CLOSE_SOCKET( sock_orv);
  }

  if ((debug& DBG_MSI) != 0)
  {
    fprintf( stderr, " task(end).  sts = %d.\n", sts);
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* msleep(): Suspend process for <arg> milliseconds.
 * <arg> < 1000 if using usleep().
 */

#ifdef _WIN32                   /* Windows: Use msleep(). */

int msleep( int msec)
{
  Sleep( msec);
  return 0;
}

#else /* def _WIN32 */          /* Non-Windows: Use usleep(). */

int msleep( int msec)
{
  int sts;

  if (msec >= 1000)
  {
    errno = EINVAL;
    sts = -1;
  }
  else
  {
    sts = usleep( msec* 1000);
  }
  return sts;
}

#endif /* def _WIN32 [else] */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* task_retry(): Execute task() with retries.
 *               Retry until rsp_req condition is met, or retry limit is
 *               reached.
 */

int task_retry( int rsp_req,            /* Response requirement bit mask. */
                int task_nr,            /* Task number. */
                int *rsp_p,             /* Response type bit mask. */
                unsigned char **tbl_p,  /* Table data. */
                orv_data_t *origin_p,   /* orv_data origin. */
                orv_data_t *target_p)   /* orv_data target. */
{
  int retry_count = 0;
  int sts = 0;

  while ((sts == 0) &&
   ((*rsp_p& rsp_req) == 0) &&
   (retry_count < TASK_RETRY_MAX))
  {
    if (retry_count > 0)
    {
      if (((debug& DBG_MSI) != 0) || ((debug& DBG_MSO) != 0))
      {
        fprintf( stderr,
         " TASK RETRY (%d).  N = %d, rsp = %08x , rsp_req = %08x .\n",
         task_nr, retry_count, *rsp_p, rsp_req);
      }
      msleep( TASK_RETRY_WAIT);         /* Delay (ms) between retries. */
    }
    sts = task( task_nr, rsp_p, tbl_p, origin_p, target_p);
    retry_count++;
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/* catalog_devices_ddf(): Use file data to populate the orv_data LL. */

#define CLG_LINE_MAX 256

int catalog_devices_ddf( FILE *fp,              /* File pointer. */
                         int *clg_line_nr,      /* File line number. */
                         char *err_tkn,         /* Bad token. */
                         orv_data_t *origin_p)  /* LL origin. */
{
  int sts = 0;
  char clg_line[ CLG_LINE_MAX+ 1];              /* fgets() buffer. */
  char *cp;                                     /* fgets() result, ... */
  char *ipa;                                    /* IP address string. */
  char *mac;                                    /* MAC address string. */
  char *nam;                                    /* Device name string. */
  int fl;                                       /* file line length, ... */
  struct in_addr ip_addr;                       /* IP address. */
  unsigned char mac_b[ 6];                      /* MAC address. */
  orv_data_t *orv_data_p;

  *clg_line_nr = 0;                             /* File line number. */
  while ((cp = fgets( clg_line, CLG_LINE_MAX, fp)) != NULL)
  {
    (*clg_line_nr)++;                   /* Count this line. */
    ipa = NULL;                         /* Clear token pointers. */
    mac = NULL;
    nam = NULL;

    clg_line[ CLG_LINE_MAX] = '\0';     /* Make buffer-end tidy. */
    fl = strlen( clg_line);
    if ((fl > 0) && (clg_line[ fl- 1] == '\n'))
    {
      clg_line[ fl- 1] = '\0';          /* Strip off a new-line. */
    }

    if ((cp = strchr( clg_line, '#')) != NULL)  /* Trim comment. */
    {
      *cp = '\0';
    }

    if (strlen( clg_line) > 0)          /* Trim leading white space. */
    {
      cp = clg_line;
      while (isspace( *cp))
      {
        cp++;
      }
    }

    if (strlen( cp) == 0)               /* No data on line. */
    {
      continue;
    }

    if (strlen( cp) > 0)
    {
      ipa = cp;                         /* IP address. */
      while (!isspace( *cp))
      {
        cp++;
      }
      *cp++ = '\0';                     /* NUL-terminate at first space. */
    }

    if (strlen( cp) > 0)                /* Skip white space. */
    {
      while (isspace( *cp))
      {
        cp++;
      }

      mac = cp;                         /* MAC address. */
      while (!isspace( *cp))
      {
        cp++;
      }
      *cp++ = '\0';                     /* NUL-terminate at first space. */
    }

    if (strlen( cp) > 0)
    {
      while (isspace( *cp))             /* Skip white space. */
      {
        cp++;
      }

      nam = cp;                         /* Device name. */
      if (*cp == '>')                   /* Apparently, ">name<". */
      {
        nam++;
        while ((*cp != '\0') && (*cp != '<'))
        {
          cp++;
        }
        *cp++ = '\0';                   /* NUL-terminate at "<". */
      }
      else
      {
        while (!isspace( *cp))
        {
          cp++;
        }
        *cp++ = '\0';                   /* NUL-terminate at first space. */
      }
    }

    if (ipa == NULL)
    {
      sts = -2;
      break;
    }

    if (mac == NULL)
    {
      sts = -3;
      break;
    }
    if (nam == NULL)
    {
      sts = -4;
      break;
    }

    if ((debug& DBG_FIL) != 0)
    {
      fprintf( stderr, " ipa: >%s<, mac: >%s<, nam: >%s<\n", ipa, mac, nam);
    }

    sts = dns_resolve( ipa, &ip_addr);
    if (sts != 0)
    {
      sts = -5;
      fprintf( stderr, "%s: Bad IP address on line %d: >%s<.\n",
       PROGRAM_NAME, *clg_line_nr, ipa);
      break;
    }

    sts = parse_mac( mac, mac_b);
    if (sts != 0)
    {
      sts = -6;
      fprintf( stderr, "%s: Bad MAC address on line %d: >%s<.\n",
       PROGRAM_NAME, *clg_line_nr, mac);
      break;
    }

    fl = strlen( nam);
    if (fl > DEV_NAME_LEN)
    {
      sts = -7;
      fprintf( stderr,
       "%s: Name too long (len = %d > %d) on line %d: >%s<.\n",
       PROGRAM_NAME, fl, DEV_NAME_LEN, *clg_line_nr, nam);
      break;
    }
    /* (Check name for invalid characters?  What's valid?) */

    orv_data_p = orv_data_new( origin_p,
     ((origin_p->sort_key == SRT_IP) ? (void *)&ip_addr : (void *)mac_b));

    if (orv_data_p == NULL)
    {
      fprintf( stderr, "%s: malloc() failed [2].\n", PROGRAM_NAME);
      break;
    }
    else
    {
      memcpy( orv_data_p->mac_addr, mac_b, MAC_ADDR_SIZE);      /* MAC addr. */
      orv_data_p->ip_addr.s_addr = ip_addr.s_addr;              /* IP addr. */
      if ((fl == strlen( UNSET_NAME)) &&
       (memcmp( nam, UNSET_NAME, strlen( UNSET_NAME)) == 0))
      {
        memset( orv_data_p->name, 0xff, DEV_NAME_LEN);  /* Unset name. */
      }
      else
      {
        memcpy( orv_data_p->name, nam, fl);             /* Device name. */
        memset( orv_data_p->name+ fl, 0x20,             /* Blank fill. */
         (DEV_NAME_LEN- fl));
      }
    }
  } /* while */

  if ((debug& DBG_FIL) != 0)
  {
    fprintf( stderr, " catalog_devices_ddf(end).  sts = %d.\n", sts);
    fprintf_device_list( stdout, FDL_DDF, origin_p);
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* discover_devices(): Use Unit Discovery to populate the orv_data LL. */

int discover_devices( int single, orv_data_t *origin_p)
{
  int rsp;
  int sts;
  orv_data_t *orv_data_p;

  if ((debug& DBG_DEV) != 0)
  {
    fprintf( stderr, " disc_devs(0).  single = %08x .\n", single);
  }

  orv_data_p = origin_p->next;      /* Start with first (real?) LL mmbr. */
  while (orv_data_p != origin_p)    /* Quit when back to the origin. */
  {
    if ((debug& DBG_DEV) != 0)
    {
      fprintf( stderr, " disc_devs(1).  cnt_flg = %d.\n", orv_data_p->cnt_flg);
      fprintf_device( stderr, orv_data_p);
    }

    if ((single == 0) || (orv_data_p->cnt_flg != 0))
    {
      /* Send Unit Discovery message.  Expect some "qg" response. */
      rsp = 0;
      sts = task_retry( RSP_QG, TSK_UNIT_DISC, &rsp, NULL,
       origin_p, orv_data_p);
      if (sts != 0)
      {
        fprintf( stderr, "%s: Unit discovery.  sts = %d.\n",
         PROGRAM_NAME, sts);
        break;
      }
    }
    orv_data_p = orv_data_p->next;  /* Advance to the next member. */
  } /* while */

  if ((debug& DBG_DEV) != 0)
  {
    fprintf( stderr, " disc_devs(end).  sts = %d.\n", sts);
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* query_devices(): Query devices to populate the orv_data LL. */

int query_devices( int single, orv_data_t *origin_p)
{
  int rsp;
  int sts;
  orv_data_t *orv_data_p;

  if ((debug& DBG_DEV) != 0)
  {
    fprintf( stderr, " query_devs(0).  single = %08x .\n", single);
  }

  orv_data_p = origin_p->next;      /* Start with first (real?) LL mmbr. */
  while (orv_data_p != origin_p)    /* Quit when back to the origin. */
  {
    if ((debug& DBG_DEV) != 0)
    {
      fprintf( stderr, " query_devs(1).  cnt_flg = %d.\n", orv_data_p->cnt_flg);
      fprintf_device( stderr, orv_data_p);
    }

    if ((single == 0) || (orv_data_p->cnt_flg != 0))
    {
      /* Send Subscribe message.  Expect some "cl" response. */
      rsp = 0;
      sts = task_retry( RSP_CL, TSK_SUBSCRIBE, &rsp, NULL,
       origin_p, orv_data_p);
      if (sts != 0)
      {
        fprintf( stderr, "%s: Read table: socket.  sts = %d.\n",
         PROGRAM_NAME, sts);
        break;
      }

      if (sts == 0)
      {
        /* Send Read table: socket message.  Expect some "rt" response. */
        rsp = 0;
        sts = task_retry( RSP_RT, TSK_RT_SOCKET, &rsp, NULL,
         origin_p, orv_data_p);
        if (sts != 0)
        {
          fprintf( stderr, "%s: Read table: socket.  sts = %d.\n",
           PROGRAM_NAME, sts);
          break;
        }
      }
    }
    orv_data_p = orv_data_p->next;  /* Advance to the next member. */
  } /* while */

  if ((debug& DBG_DEV) != 0)
  {
    fprintf( stderr, " query_devs(end).  sts = %d.\n", sts);
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* catalog_devices_live(): Use Global Discovery to populate the orv_data LL. */

int catalog_devices_live( orv_data_t *origin_p)
{
  int rsp;
  int sts;

  /* Broadcast query (Global discovery).  Expect some "qa" response. */
  rsp = 0;
  sts = task_retry( RSP_QA, TSK_GLOB_DISC_B, &rsp, NULL,
   origin_p, origin_p);
  if (sts == 0)
  {
    if (origin_p->cnt_flg == 0)
    {
      fprintf( stderr, "%s: No devices found (loc=cdl).\n", PROGRAM_NAME);
      errno = ENXIO;
      sts = -1;
    }
    else
    {
      if ((debug& DBG_DEV) != 0)
      {
        fprintf( stderr, " Devices found: %d.\n", origin_p->cnt_flg);
      }

      sts = query_devices( 0, origin_p);        /* Query all devs in the LL. */
    }
  }

  if ((debug& DBG_DEV) != 0)
  {
    fprintf( stderr, " catalog_devices_live(end).  sts = %d.\n", sts);
  }
  return sts;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* keyword_match(): Match abbreviated keyword against keyword array. */

int keyword_match( char *arg, int kw_cnt, char **kw_array)
{
  int cmp_len;
  int kw_ndx;
  int match = -1;

  cmp_len = strlen( arg);
  for (kw_ndx = 0; kw_ndx < kw_cnt; kw_ndx++)
  {
    if (STRNCASECMP( arg, kw_array[ kw_ndx], cmp_len) == 0)
    {
      if (match < 0)
      {
        match = kw_ndx;         /* Record first match. */
      }
      else
      {
        match = -2;             /* Record multiple match, */
        break;                  /* and quit early. */
      }
    }
  }
  return match;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* main(): Main program. */

int main( int argc, char **argv)
{
  int brief;
  int expect_set = 0;
  int match_opr;
  int opts_ndx;
  int quiet;
  int rsp;
  int single;
  int specific_ip;
  int sts;
  int task_nr;
  size_t cmp_len;
  char *orv_data_file_name = NULL;
  FILE *fp;
  char *new_dev_name = NULL;
  char *new_password = NULL;
  size_t new_dev_name_len;
  size_t new_password_len;
  unsigned short msg_len;

  unsigned char *msg_tmp1_p;
  unsigned char *msg_tmp2_p;

  orv_data_t *orv_data_p;

  orv_data_t orv_data =                         /* orv_data LL origin. */
   { &orv_data,                                 /* next ptr. */
     &orv_data,                                 /* prev ptr. */
     0,                                         /* cnt_flg. */
     SRT_IP,                                    /* sort_key. */
     { 0 },                                     /* ip_addr. */
     htons( PORT_ORV),                          /* port. */
     -1,                                        /* type. */
     { 0, 0, 0, 0, 0, 0, 0, 0,                  /* (device) name. */
       0, 0, 0, 0, 0, 0, 0, 0 },
     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },    /* passwd. */
     { 0, 0, 0, 0, 0, 0 },                      /* mac_addr */
     -1,                                        /* state. */
   };

  brief = 0;
  debug = 0;
  quiet = 0;
  single = 0;
  sts = 0;

  /* Check command-line arguments. */

  if (sts == 0)
  {
    char *eqa_p;
    char *eqo_p;
    int opts_cnt = sizeof( opts)/ sizeof( *opts);
    int match_opt;

    while (argc > 1)
    { /* Parse/skip option arguments. */
      match_opt = -1;
      eqa_p = strchr( argv[ 1], '=');           /* Find "=" in arg. */
      for (opts_ndx = 0; opts_ndx < opts_cnt; opts_ndx++)   /* Try all opts. */
      {
        eqo_p = strchr( opts[ opts_ndx], '=');  /* Find "=" in opt. */
        if (! ((eqa_p == NULL) ^ (eqo_p == NULL)))
        { /* "=" in both arg and opt, or neither. */
          if (eqa_p == NULL)
          { /* No "=" in argument/option.  Compare whole strings. */
            cmp_len = OMIN( strlen( argv[ 1]), strlen( opts[ opts_ndx]));
          }
          else
          { /* "=".  Compare strings before "=". */
            cmp_len = (eqa_p- argv[ 1]);
          }
          if (STRNCASECMP( argv[ 1], opts[ opts_ndx], cmp_len) == 0)
          {
            if (match_opt < 0)
            {
              match_opt = opts_ndx;     /* Record first match. */
            }
            else
            {
              match_opt = -2;           /* Record multiple match, */
              break;                    /* and quit early. */
            }
          } /* if arg-opt match */
        } /* if "=" match */
      } /* for opts_ndx */

      /* All options checked.  Act accordingly. */
      if (match_opt < -1)
      { /* Multiple match. */
        fprintf( stderr, "%s: Ambiguous option: %s\n",
         PROGRAM_NAME, argv[ 1]);
        errno = EINVAL;
        sts = EXIT_FAILURE;
        break; /* while */
      }
      else if (match_opt < 0)           /* Not a known option.  Arg? */
      {
        break; /* while */
      }
      else                              /* Recognized option. */
      {
        if (match_opt == OPT_BRIEF)             /* "brief". */
        {
          match_opt = -1;                       /* Consumed. */
          brief = 1;
        }
        else if (match_opt == OPT_QUIET)        /* "quiet". */
        {
          match_opt = -1;                       /* Consumed. */
          quiet = 1;
        }
        else if (match_opt == OPT_DDF)          /* "ddf". */
        { /* No "=file_spec".  Use environment variable. */
          match_opt = -1;                       /* Consumed. */
          orv_data_file_name = getenv( ORVL_DDF);
          if (orv_data_file_name == NULL)
          {
            orv_data_file_name = ORVL_DDF;
          }
        }
        else if (match_opt == OPT_DDF_EQ)       /* "ddf=file_spec". */
        {
          match_opt = -1;                       /* Consumed. */
          orv_data_file_name = argv[ 1]+ cmp_len+ 1;
        }
        else if (match_opt == OPT_DEBUG)        /* "debug". */
        {
          match_opt = -1;                       /* Consumed. */
          debug = 0xffffffff;
          fprintf( stderr, " debug:  0x%08x .\n", debug);
        }
        else if (match_opt == OPT_DEBUG_EQ)     /* "debug=". */
        {
          match_opt = -1;                       /* Consumed. */
          debug = strtol( (argv[ 1]+ cmp_len+ 1), NULL, 0);
          fprintf( stderr, " debug = 0x%08x .\n", debug);
        }
        else if (match_opt == OPT_NAME_EQ)      /* "name=". */
        {
          match_opt = -1;                       /* Consumed. */
          expect_set = 1;                       /* Expect "set" op. */
          new_dev_name = argv[ 1]+ cmp_len+ 1;
          new_dev_name_len = strlen( new_dev_name);
          if (new_dev_name_len > DEV_NAME_LEN)
          {
            fprintf( stderr,
             "%s: Device name too long (len = %ld > %d): %s\n",
             PROGRAM_NAME, new_dev_name_len, DEV_NAME_LEN, argv[ 1]);
            errno = EINVAL;
            sts = EXIT_FAILURE;
            break; /* while */
          }
        }
        else if (match_opt == OPT_PASSWORD_EQ)  /* "password=". */
        {
          match_opt = -1;                       /* Consumed. */
          expect_set = 1;                       /* Expect "set" op. */
          new_password = argv[ 1]+ cmp_len+ 1;
          new_password_len = strlen( new_password);
          if (new_password_len > PASSWORD_LEN)
          {
            fprintf( stderr,
             "%s: Password too long (len = %ld > %d): %s\n",
             PROGRAM_NAME, new_password_len, PASSWORD_LEN, argv[ 1]);
            errno = EINVAL;
            sts = EXIT_FAILURE;
            break; /* while */
          }
        }
        else if (match_opt == OPT_SORT_EQ)      /* "sort=". */
        {
          match_opt = -1;                       /* Consumed. */

          /* Match sort-key keyword. */
          orv_data.sort_key = keyword_match( (argv[ 1]+ cmp_len+ 1),
           (sizeof( sort_keys)/ sizeof( *sort_keys)),
           sort_keys);

          if (orv_data.sort_key < -1)
          { /* Multiple match. */
            fprintf( stderr, "%s: Ambiguous sort key: >%s<\n",
             PROGRAM_NAME, (argv[ 1]+ cmp_len+ 1));
            usage();
            errno = EINVAL;
            sts = EXIT_FAILURE;
          }
          else if (orv_data.sort_key < 0)       /* Not a known key. */
          { /* No match. */
            fprintf( stderr, "%s: Invalid sort key: >%s<.\n",
             PROGRAM_NAME, (argv[ 1]+ cmp_len+ 1));
            usage();
            errno = EINVAL;
            sts = EXIT_FAILURE;
          }
        }

        if (match_opt >= 0)             /* Unexpected option. */
        { /* Match, but no handler. */
          fprintf( stderr,
           "%s: Unexpected option (bug), code = %d.\n",
           PROGRAM_NAME, match_opt);
          errno = EINVAL;
          sts = EXIT_FAILURE;
          break; /* while */
        }
        argc--;                 /* Shift past this argument. */
        argv++;
      }
    } /* while argc */
  }

  if ((debug& DBG_OPT) != 0)
  {
    fprintf( stderr, " SET opts: dev_name: >%s<, password: >%s<\n",
     ((new_dev_name == NULL) ? "" : new_dev_name),
     ((new_password == NULL) ? "" : new_password));
  }

  if (sts == 0)
  {
    if (argc < 2)
    {
     fprintf( stderr,
      "%s: Missing required operation.  (Post-options arg count = %d.)\n",
      PROGRAM_NAME, (argc- 1));

      usage();
      errno = E2BIG;
      sts = EXIT_FAILURE;
    }
    else if (argc > 3)
    {
      fprintf( stderr,
       "%s: Excess arg(s), or bad opts.  (Post-opts arg count = %d.)\n",
       PROGRAM_NAME, (argc- 1));

      usage();
      errno = E2BIG;
      sts = EXIT_FAILURE;
    }
    else
    { /* Match operation keyword. */
      match_opr = keyword_match( argv[ 1],  /* Opr keyword candidate. */
       (sizeof( oprs)/ sizeof( *oprs)),     /* Opr keyword array size. */
       oprs);                               /* Opr keyword array. */

      /* All operations checked.  Act accordingly. */
      if (match_opr < -1)
      { /* Multiple match. */
        fprintf( stderr, "%s: Ambiguous operation: >%s<\n",
         PROGRAM_NAME, argv[ 1]);
        usage();
        errno = EINVAL;
        sts = EXIT_FAILURE;
      }
      else if (match_opr < 0)           /* Not a known option.  Arg? */
      { /* No match. */
        fprintf( stderr, "%s: Invalid option/operation: >%s<.\n",
         PROGRAM_NAME, argv[ 1]);
        usage();
        errno = EINVAL;
        sts = EXIT_FAILURE;
      }
      else if ((expect_set != 0) && (match_opr != OPR_SET))
      {
        fprintf( stderr,
"%s: \"name=\" or \"password=\" option, but operation not \"set\": %s\n",
         PROGRAM_NAME, oprs[ match_opr]);
        usage();
        errno = EINVAL;
        sts = EXIT_FAILURE;
      }
      else if ((expect_set == 0) && (match_opr == OPR_SET))
      {
        fprintf( stderr,
"%s: Operation \"set\", but no \"name=\" or \"password=\" option.\n",
         PROGRAM_NAME);
        usage();
        errno = EINVAL;
        sts = EXIT_FAILURE;
      }
      else if (argc < 3)
      {
        if ((match_opr == OPR_HEARTBEAT) ||
         (match_opr == OPR_OFF) ||
         (match_opr == OPR_ON) ||
         (match_opr == OPR_SET))
        {
          fprintf( stderr,
           "%s: Missing required device identifier for operation: %s\n",
           PROGRAM_NAME, oprs[ match_opr]);
          usage();
          errno = EINVAL;
          sts = EXIT_FAILURE;
        }
      }
    }
  }

  /* Prepare for device communication. */

#ifdef _WIN32

  /* Windows socket library, start-up, ... */

# pragma comment( lib, "ws2_32.lib")    /* Arrange WinSock link library. */

  if (sts == 0)
  {
    WORD ws_ver_req = MAKEWORD( 2, 2);  /* Request WinSock version 2.2. */
    WSADATA wsa_data;

    sts = WSAStartup( ws_ver_req, &wsa_data);
    if (sts != 0)
    {
      fprintf( stderr, "%s: Windows socket start-up failed.  sts = %d.\n",
       PROGRAM_NAME, sts);
      show_errno( PROGRAM_NAME);
    }
    else if ((debug& DBG_WIN) != 0)
    {
      /* LOBYTE = MSB, HIBYTE = LSB. */
      fprintf( stderr, " wVersion = %d.%d, wHighVersion = %d.%d.\n",
       LOBYTE( wsa_data.wVersion), HIBYTE( wsa_data.wVersion),
       LOBYTE( wsa_data.wHighVersion), HIBYTE( wsa_data.wHighVersion));
    }
  }
#endif /* def _WIN32 */

  if (sts == 0)
  { /* Assume broadcast, but use argv[ 2] if it can be resolved. */
    specific_ip = 0;
    orv_data.ip_addr.s_addr = htonl( INADDR_BROADCAST);

    if (argc >= 3)
    {
      int sts2;
      struct in_addr ip_addr;

      single = 1;               /* User-specified device (DNS, IP, name). */
      sts2 = dns_resolve( argv[ 2], &ip_addr);  /* Try to resolve the name. */

      if ((debug& DBG_DNS) != 0)
      {
        fprintf( stderr, " dns_resolve() sts = %d.\n", sts2);
      }

      if (sts2 == 0)
      {
        specific_ip = 1;
        orv_data.ip_addr.s_addr = ip_addr.s_addr;
      }
    }
  }

  if (sts == 0)
  {
    /* Open/read the data file, if specified.  Otherwise, broadcast query. */
    if (orv_data_file_name != NULL)
    {
      fp = fopen( orv_data_file_name, "r");
      if (fp == NULL)
      {
        sts = -1;
        fprintf( stderr, "%s: Open (read) failed: %s\n",
         PROGRAM_NAME, orv_data_file_name);
        show_errno( PROGRAM_NAME);
      }
      else
      {
        int line_nr;
        char err_tkn[ CLG_LINE_MAX];

        sts = catalog_devices_ddf( fp, &line_nr, err_tkn, &orv_data);

        if (sts == -2)
        {
          fprintf( stderr, "%s: IP address not found on line %d.\n",
           PROGRAM_NAME, line_nr);
        }
        else if (sts == -3)
        {
          fprintf( stderr, "%s: MAC address not found on line %d.\n",
           PROGRAM_NAME, line_nr);
        }
        else if (sts == -4)
        {
          fprintf( stderr, "%s: Device name not found on line %d.\n",
           PROGRAM_NAME, line_nr);
        }
        else if (sts == -5)
        {
          fprintf( stderr, "%s: Bad IP address on line %d: >%s<.\n",
           PROGRAM_NAME, line_nr, err_tkn);
        }
        else if (sts == -6)
        {
          fprintf( stderr, "%s: Bad MAC address on line %d: >%s<.\n",
           PROGRAM_NAME, line_nr, err_tkn);
        }
        else if (sts == -7)
        {
          fprintf( stderr,
           "%s: Name too long (len > %d) on line %d: >%s<.\n",
           PROGRAM_NAME, line_nr, DEV_NAME_LEN, err_tkn);
        }
        else if (sts != 0)
        {
          fprintf( stderr,
           "%s: Unknown error (%d) reading file at line %d.\n",
           PROGRAM_NAME, sts, line_nr);
        }
        fclose( fp);
      }
    }
  }

  if (sts == 0)
  {
    if (match_opr == OPR_HEARTBEAT)
    { /* "Heartbeat". */
      if (specific_ip == 0)
      { /* Unknown IP address.  Use broadcast query. */
        if (orv_data_file_name == NULL)
        {
          sts = catalog_devices_live( &orv_data);
        }
      }
      else
      { /* Specific IP address.  Use specific query. */
        /* Send Global discovery message.  Expect some "qa" response. */
        rsp = 0;
        sts = task_retry( RSP_QA, TSK_GLOB_DISC, &rsp, NULL,
         &orv_data, &orv_data);
      }

      if (sts == 0)
      {
        if (specific_ip == 0)
        { /* Compare device names (name arg v. real LL data). */
          orv_data_p = orv_data_find_name( &orv_data, argv[ 2]);
        }
        else
        { /* Compare IP addresses (LL origin v. real LL data). */
          orv_data_p = orv_data_find_ip_addr( &orv_data);
        }
        if (orv_data_p == NULL)
        {
          fprintf( stderr,
           "%s: Device name not matched (loc=%d): >%s<.\n",
           PROGRAM_NAME, 2, argv[ 2]);
          errno = ENXIO;
          sts = EXIT_FAILURE;
        }
      }
      if (sts == 0)
      {
        /* Send Heartbeat message.  Expect some "hb" response. */
        rsp = 0;
        sts = task_retry( RSP_HB, TSK_HEARTBEAT, &rsp, NULL,
         &orv_data, orv_data_p);

        if ((sts == 0) && ((rsp& RSP_HB) == 0))
        {
            fprintf( stderr, "%s: No heartbeat response received.\n",
             PROGRAM_NAME);
            errno = ENOMSG;
            sts = -1;
        }
      }
    }
    else if (match_opr == OPR_LIST)
    { /* "list".  List device(s), without query, if possible. */
      if (orv_data_file_name == NULL)
      { /* No DDF data available. */
        if ((single != 0) && (specific_ip == 0))
        { /* Have a device name (if anything valid).  Need full inventory. */
          sts = catalog_devices_live( &orv_data);
        }
        else
        { /* General or specific IP address.  Discovery is enough. */
          task_nr = (specific_ip == 0) ? TSK_GLOB_DISC_B : TSK_GLOB_DISC;
          /* Send the appropriate (Global or Unit) discovery message.
           * Expect some "qa" response.
           */
          rsp = 0;
          sts = task_retry( RSP_QA, task_nr, &rsp, NULL,
           &orv_data, &orv_data);

          if ((specific_ip == 0) && (orv_data.cnt_flg == 0))
          {
            fprintf( stderr, "%s: No devices found (loc=list).\n",
             PROGRAM_NAME);
            errno = ENXIO;
            sts = -1;
          }
          else
          {
            if ((debug& DBG_DEV) != 0)
            {
              fprintf( stderr, " Devices found: %d.\n", orv_data.cnt_flg);
            }
          }
        }
      }

      if ((sts == 0) && (single != 0))
      { /* Locate the specific device in the orv_data LL. */
        sts = orv_data_find_dev( &orv_data, specific_ip, argv[ 2], 0);
      }

      if ((sts == 0) && (orv_data_file_name != NULL))
      { /* DDF data available.  Use Unit discovery to sense. */
        sts = discover_devices( single, &orv_data);
      }
    }
    else if (match_opr == OPR_QLIST)
    { /* "qlist".  List device(s), with query. */
      if (orv_data_file_name == NULL)
      { /* No DDF data available.  Must query devices.*/
        if (specific_ip == 0)
        { /* Do all, or could have a device name.  Need full inventory. */
          sts = catalog_devices_live( &orv_data);
          if ((sts == 0) && (single != 0))
          { /* Match device name. */
            sts = orv_data_find_dev( &orv_data, specific_ip, argv[ 2], 1);
          }
        }
        else
        { /* Have IP address.  Do specific Global Discovery and query. */
          rsp = 0;
          sts = task_retry( RSP_QA, TSK_GLOB_DISC, &rsp, NULL,
           &orv_data, &orv_data);
          if (sts == 0)
          { /* Match specific IP address. */
            sts = orv_data_find_dev( &orv_data, specific_ip, argv[ 2], 1);
            if (sts == 0)
            { /* Query the single device. */
              sts = query_devices( single, &orv_data);
            }
          }
        }
      }
      else
      { /* Using DDF.  Query the significant devices. */
        if (single != 0)
        { /* Match specific IP address or device name. */
          sts = orv_data_find_dev( &orv_data, specific_ip, argv[ 2], 1);
        }
        if (sts == 0)
        {
          sts = query_devices( single, &orv_data);
        }
      }
    }
    else if ((match_opr == OPR_OFF) || (match_opr == OPR_ON))
    { /* "Off", "On".  Device control: Switch Off/On. */
      if (specific_ip == 0)
      { /* Unknown IP address.  Use broadcast query. */
        if (orv_data_file_name == NULL)
        {
          sts = catalog_devices_live( &orv_data);
        }
      }
      else
      { /* Specific IP address.  Use specific query. */
        /* Send Global discovery message.  Expect some "qa" response. */
        rsp = 0;
        sts = task_retry( RSP_QA, TSK_GLOB_DISC, &rsp, NULL,
         &orv_data, &orv_data);
      }

      if (sts == 0)
      {
        if (specific_ip == 0)
        { /* Compare device names (name arg v. real LL data). */
          orv_data_p = orv_data_find_name( &orv_data, argv[ 2]);
        }
        else
        { /* Compare IP addresses (LL origin v. real LL data). */
          orv_data_p = orv_data_find_ip_addr( &orv_data);
        }
        if (orv_data_p == NULL)
        {
          fprintf( stderr,
           "%s: Device name not matched (loc=%d): >%s<.\n",
           PROGRAM_NAME, 2, argv[ 2]);
          errno = ENXIO;
          sts = EXIT_FAILURE;
        }
        else
        {
          orv_data_p->cnt_flg = 1;    /* Mark this member for reportng. */
        }
      }
      if (sts == 0)
      {
        /* Send Subscribe message.  Expect some "cl" response. */
        rsp = 0;
        single = 1;

        sts = task_retry( RSP_CL, TSK_SUBSCRIBE, &rsp, NULL,
         &orv_data, orv_data_p);
        if (sts == 0)
        {
          /* Send Device control message.  Expect some "dc" response. */
          /* (Use one loop for both Subscribe and Device control? */
          rsp = 0;
          task_nr = (match_opr == OPR_OFF) ? TSK_SW_OFF : TSK_SW_ON;
          sts = task_retry( RSP_DC, task_nr, &rsp, NULL,
           &orv_data, orv_data_p);
        }
      }
    }
    else if (match_opr == OPR_SET)
    { /* "set".  Set device parameter(s). */
      if (specific_ip == 0)
      { /* Unknown IP address.  Use broadcast query. */
        if (orv_data_file_name == NULL)
        {
          sts = catalog_devices_live( &orv_data);
        }
      }
      else
      { /* Specific IP address.  Use specific query. */
        /* Send Global discovery message.  Expect some "qa" response. */
        rsp = 0;
        sts = task_retry( RSP_QA, TSK_GLOB_DISC, &rsp, NULL,
         &orv_data, &orv_data);
      }

      if (sts == 0)
      {
        if (specific_ip == 0)
        { /* Compare device names (name arg v. real LL data). */
          orv_data_p = orv_data_find_name( &orv_data, argv[ 2]);
        }
        else
        { /* Compare IP addresses (LL origin v. real LL data). */
          orv_data_p = orv_data_find_ip_addr( &orv_data);
        }
        if (orv_data_p == NULL)
        {
          fprintf( stderr,
           "%s: Device name not matched (loc=%d): >%s<.\n",
           PROGRAM_NAME, 2, argv[ 2]);
          errno = ENXIO;
          sts = EXIT_FAILURE;
        }
        else
        {
          orv_data_p->cnt_flg = 1;    /* Mark this member for reportng. */
        }
      }
      if (sts == 0)
      {
        /* Send Subscribe message.  Expect some "cl" response. */
        rsp = 0;
        single = 1;

        sts = task_retry( RSP_CL, TSK_SUBSCRIBE, &rsp, NULL,
         &orv_data, orv_data_p);
        if (sts == 0)
        {
          /* Send Read table: socket message.  Expect some "rt" response.
           * Save the "rt" response for use (after modification) as the
           * "tm" message. 
           */
          rsp = 0;
          msg_tmp1_p = NULL;
          msg_tmp2_p = NULL;

          sts = task_retry( RSP_RT, TSK_RT_SOCKET, &rsp, &msg_tmp1_p,
           &orv_data, orv_data_p);
          if (sts != 0)
          {
            fprintf( stderr, "%s: Read table: socket.  sts = %d.\n",
             PROGRAM_NAME, sts);
          }
          else if (msg_tmp1_p == NULL)
          {
            fprintf( stderr, "%s: Read table: socket.  NULL ptr.\n",
             PROGRAM_NAME);
            sts = -1;
          }
          else
          { /* Extract response message length. */
            msg_len = (unsigned short)msg_tmp1_p[ 2]* 256+
                      (unsigned short)msg_tmp1_p[ 3];

            if ((debug& DBG_MSI) != 0)
            {
              /* Display the response (hex, ASCII). */
              fprintf( stderr, "   %c  %c  msg_len = %d\n",
               (msg_tmp1_p)[ 4], (msg_tmp1_p)[ 5], msg_len);

              msg_dump( msg_tmp1_p, msg_len);
            }
          }
        }

        if (sts == 0)
        {
          /* Form a new "tm" message from the saved "rt" response.
           * Overlay reduced message length and "tm" op-code onto the
           * saved "rt" response.  Leave the initial "hd" (2 bytes) and
           * the MAC address and 0x20 padding (12 bytes).
           */
          msg_tmp1_p[ 2] = (unsigned short)(msg_len- 3)/ 256;
          msg_tmp1_p[ 3] = (unsigned short)(msg_len- 3)% 256;
          memcpy( (msg_tmp1_p+ 4), (cmd_write_table+ 4), 2);

          /* Discard byte 18.  Shift bytes 19:25 (including the table
           * and version numbers) left one byte (18:24).
           */
          memmove( (msg_tmp1_p+ 18), (msg_tmp1_p+ 19), 7);

          /* Discard bytes 26, 27.  Shift the remaining bytes (28:end)
           * left three bytes (25:(end-3)).
           */
          memmove( (msg_tmp1_p+ 25), (msg_tmp1_p+ 28), (msg_len- 28));

          /* Overlay "name=" option value onto the message. */
          if (new_dev_name != NULL)
          { /* Option value plus blank fill. */
            if (new_dev_name_len == 0)
            { /* No name specified.  Reset to factory: 16* 0xff. */
              memset( (msg_tmp1_p+ 67), 0xff, DEV_NAME_LEN);
            }
            else
            { /* Normal name. */
              memcpy( (msg_tmp1_p+ 67), new_dev_name, new_dev_name_len);
              memset( (msg_tmp1_p+ 67+ new_dev_name_len), 0x20,
               (DEV_NAME_LEN- new_dev_name_len));
            }
          }

          /* Overlay "password=" option value onto the message. */
          if (new_password != NULL)
          { /* Option value plus blank fill. */
            if (new_password_len == 0)
            { /* No password specified.  Reset to factory: "888888". */
              new_password = "888888";
              new_password_len = strlen( new_password);
            }
            memcpy( (msg_tmp1_p+ 55), new_password, new_password_len);
            memset( (msg_tmp1_p+ 55+ new_password_len), 0x20,
             (PASSWORD_LEN- new_password_len));
          }

          if ((debug& DBG_MSI) != 0)
          {
            if (msg_tmp1_p != NULL)
            {
              fprintf( stderr, "   %c  %c  msg_len-3 = %d\n",
               (msg_tmp1_p)[ 4], (msg_tmp1_p)[ 5], (msg_len-3));

              msg_dump( msg_tmp1_p, (msg_len-3));
            }
          }

          /* Send Write table: socket message.  Expect some "tm" response. */
          rsp = 0;
          sts = task_retry( RSP_TM, TSK_WT_SOCKET, &rsp, &msg_tmp1_p,
           &orv_data, orv_data_p);
          if (sts != 0)
          {
            fprintf( stderr, "%s: Write table: socket.  sts = %d.\n",
             PROGRAM_NAME, sts);
          }
          else if ((rsp& RSP_TM) == 0)
          {
            fprintf( stderr,
             "%s: No Write table: socket response received.\n",
             PROGRAM_NAME);
            errno = ENOMSG;
            sts = -1;
          }
          else
          { /* Got "tm" response.  Re-query device to update data. */
            /* Send Read table: socket message.  Expect some "rt" response.
             * Save the "rt" response to check against requested data.
             */
            rsp = 0;
            sts = task_retry( RSP_RT, TSK_RT_SOCKET, &rsp, &msg_tmp2_p,
             &orv_data, orv_data_p);
            if (sts != 0)
            {
              fprintf( stderr, "%s: Read table: socket.  sts = %d.\n",
               PROGRAM_NAME, sts);
            }
            else
            {
              int cmp;

              /* If used, check requested device name against new "rt" data. */
              if (new_dev_name != NULL)
              {
                cmp = memcmp( (msg_tmp1_p+ 67), (msg_tmp2_p+ 70),
                 DEV_NAME_LEN);

                if (cmp != 0)
                {
                  fprintf( stderr, "%s: Device name NOT set.\n",
                   PROGRAM_NAME);
                  errno = EINVAL;
                  sts = -1;
                }
                else if ((debug& DBG_ACT) != 0)
                {
                  fprintf( stderr, " New device name: >%*s<\n",
                   DEV_NAME_LEN, (msg_tmp2_p+ 70));
                }
              }

              /* If used, check requested password against new "rt" data. */
              if (new_password != NULL)
              {
                cmp = memcmp( (msg_tmp1_p+ 55), (msg_tmp2_p+ 58),
                 DEV_NAME_LEN);

                if (cmp != 0)
                {
                  fprintf( stderr, "%s: Password NOT set.\n",
                   PROGRAM_NAME);
                  errno = EINVAL;
                  sts = -1;
                }
                else if ((debug& DBG_ACT) != 0)
                {
                  fprintf( stderr, " New password: >%*s<\n",
                   PASSWORD_LEN, (msg_tmp2_p+ 58));
                }
              }
            }
          }
        }
      }
    }
    else if ((match_opr == OPR_HELP) || (match_opr == OPR_USAGE))
    { /* "help", "usage". */
      usage();
    }
    else if (match_opr == OPR_VERSION)
    { /* "version". */
      fprintf( stdout, "%s %d.%d\n",
       PROGRAM_NAME, PROGRAM_VERSION_MAJ, PROGRAM_VERSION_MIN);
    }
    else
    {
      fprintf( stderr,
       "%s: Unexpected operation (bug), code = %d.\n",
       PROGRAM_NAME, match_opr);
       errno = EINVAL;
       sts = EXIT_FAILURE;
    }
  }

  if ((debug& DBG_SEL) != 0)
  {
    fprintf( stderr,
     " Pre-fdl().  sts = %d, brief = %d, quiet = %d, single = %d.\n",
     sts, brief, quiet, single);
  }

  /* Display any useful device data. */
  if (sts == 0)
  {
    fprintf_device_list( stdout,
     (((orv_data_file_name == NULL) ? 0 : FDL_DDF) |    /* Flags: ddf */
     ((brief == 0) ? 0 : FDL_BRIEF) |                   /*        brief */
     ((quiet == 0) ? 0 : FDL_QUIET) |                   /*        quiet */
     ((single == 0) ? 0 : FDL_SINGLE)),                 /*        one dev. */
     &orv_data);
  }

#ifndef NO_STATE_IN_EXIT_STATUS
  /* If success == status, then include a valid single-device state
   * (stored in the LL origin) in the exit status value (second lowest
   * hex digit).  0 -> 0x20, 1 -> 0x30, unknown -> 0x00.
   */
  if ((sts == 0) && (orv_data.state >= 0))
  {
    sts = (sts& (~0xf0))| (((orv_data.state == 0) ? 2 : 3)* 16);
# ifdef VMS
    sts |= STS$K_SUCCESS;       /* Retain success severity. */
# endif
  }
#endif /* ndef NO_STATE_IN_EXIT_STATUS */

  /* We could free the orv_data LL, output message, getaddrinfo(), and
   * various other malloc()'d  storage, but why bother?
   */

  exit( sts);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef VMS

/* LIB$INITIALIZE for DECC$ARGV_PARSE_STYLE on non-VAX systems. */

# ifdef __CRTL_VER
#  if !defined(__VAX) && (__CRTL_VER >= 70301000)

#   include <unixlib.h>

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Global storage. */

/* Flag to sense if decc_init() was called. */

static int decc_init_done = -1;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* decc_init()
 *
 * Uses LIB$INITIALIZE to set a collection of C RTL features without
 * requiring the user to define the corresponding logical names.
 */

/* Structure to hold a DECC$* feature name and its desired value. */

typedef struct
{
  char *name;
  int value;
} decc_feat_t;

/* Array of DECC$* feature names and their desired values. */

decc_feat_t decc_feat_array[] =
{
   /* Preserve command-line case with SET PROCESS/PARSE_STYLE=EXTENDED */
 { "DECC$ARGV_PARSE_STYLE", 1 },

   /* Preserve case for file names on ODS5 disks. */
 { "DECC$EFS_CASE_PRESERVE", 1 },

   /* Enable multiple dots (and most characters) in ODS5 file names,
    * while preserving VMS-ness of ";version".
    */
 { "DECC$EFS_CHARSET", 1 },

   /* List terminator. */
 { (char *)NULL, 0 }
};

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* LIB$INITIALIZE initialization function. */

void decc_init(void)
{
  int feat_index;
  int feat_value;
  int feat_value_max;
  int feat_value_min;
  int i;
  int sts;

  /* Set the global flag to indicate that LIB$INITIALIZE worked. */

  decc_init_done = 1;

  /* Loop through all items in the decc_feat_array[]. */

  for (i = 0; decc_feat_array[i].name != NULL; i++)
  {
    /* Get the feature index. */
    feat_index = decc$feature_get_index(decc_feat_array[i].name);
    if (feat_index >= 0)
    {
      /* Valid item.  Collect its properties. */
      feat_value = decc$feature_get_value(feat_index, 1);
      feat_value_min = decc$feature_get_value(feat_index, 2);
      feat_value_max = decc$feature_get_value(feat_index, 3);

      if ((decc_feat_array[i].value >= feat_value_min) &&
       (decc_feat_array[i].value <= feat_value_max))
      {
        /* Valid value.  Set it if necessary. */
        if (feat_value != decc_feat_array[i].value)
        {
          sts = decc$feature_set_value( feat_index,
                                        1,
                                        decc_feat_array[i].value);
        }
      }
      else
      {
        /* Invalid DECC feature value. */
        printf(" INVALID DECC FEATURE VALUE, %d: %d <= %s <= %d.\n",
         feat_value,
         feat_value_min, decc_feat_array[i].name, feat_value_max);
      }
    }
    else
    {
      /* Invalid DECC feature name. */
      printf(" UNKNOWN DECC FEATURE: %s.\n", decc_feat_array[i].name);
    }
  }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Get "decc_init()" into a valid, loaded LIB$INITIALIZE PSECT. */

#   pragma nostandard

/* Establish the LIB$INITIALIZE PSECT, with proper alignment and
 * attributes.
 */
globaldef {"LIB$INITIALIZ"} readonly _align (LONGWORD)
int spare[8] = { 0 };
globaldef {"LIB$INITIALIZE"} readonly _align (LONGWORD)
void (*x_decc_init)() = decc_init;

/* Fake reference to ensure loading the LIB$INITIALIZE PSECT. */

#   pragma extern_model save
/* The declaration for LIB$INITIALIZE() is missing in the VMS system header
 * files.  Addionally, the lowercase name "lib$initialize" is defined as a
 * macro, so that this system routine can be reference in code using the
 * traditional C-style lowercase convention of function names for readability.
 * (VMS system functions declared in the VMS system headers are defined in a
 * similar way to allow using lowercase names within the C code, whereas the
 * "externally" visible names in the created object files are uppercase.)
 */
#   ifndef lib$initialize       /* Note: This "$" may annoy Sun compilers. */
#    define lib$initialize LIB$INITIALIZE
#   endif
int lib$initialize(void);
#   pragma extern_model strict_refdef
int dmy_lib$initialize = (int)lib$initialize;
#   pragma extern_model restore

#   pragma standard

#  endif /* !defined(__VAX) && (__CRTL_VER >= 70301000) */
# endif /* __CRTL_VER */
#endif /* VMS */

/*--------------------------------------------------------------------*/
