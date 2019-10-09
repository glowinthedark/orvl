       ORVL: A Portable Control Program for Orvibo (S20) Devices

       Program version: 0.2
       Revision date:   2017-03-22
       Author:          Steven M. Schweda  sms@antinode.info

------------------------------------------------------------------------

      Copyright and License
      ---------------------

   Copyright 2017 Steven M. Schweda.

   This file is part of ORVL.

   ORVL is subject to the terms of the Perl Foundation Artistic License
   2.0.  A copy of the License is included in the ORVL kit file
   "artistic_license_2_0.txt", and on the Web at:
   http://www.perlfoundation.org/artistic_license_2_0

------------------------------------------------------------------------

      Disclaimer
      ----------

   The Artistic License 2.0 includes the following disclaimer.  It is
repeated here for convenience and emphasis.

(14) Disclaimer of Warranty: THE PACKAGE IS PROVIDED BY THE COPYRIGHT
HOLDER AND CONTRIBUTORS "AS IS' AND WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES. THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, OR NON-INFRINGEMENT ARE DISCLAIMED TO THE EXTENT
PERMITTED BY YOUR LOCAL LAW. UNLESS REQUIRED BY LAW, NO COPYRIGHT HOLDER
OR CONTRIBUTOR WILL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, OR
CONSEQUENTIAL DAMAGES ARISING IN ANY WAY OUT OF THE USE OF THE PACKAGE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

------------------------------------------------------------------------

      Introduction
      ------------

   This document describes ORVL, a portable C program which is intended
to control Orvibo S20 power-control devices (sockets).

   Web page: http://antinode.info/orvl/orvl.html

   "Orvibo" is a trademark of ORVIBO Ltd.  http://www.orvibo.com

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Features:

   o Free.

   o Command-line (non-graphical) interface, suitable for scripting.

   o Portable.  ORVL runs on many UNIX and UNIX-like systems, VMS, and
     Windows.  (That is, on conventional computers, not on Android or
     iOS devices, where an Orvibo-supplied application, "WiWo", exists.)

   o Self-contained.  One C-language source file.  No additional
     packages (GUI, networking, scripting) are needed.

   o Capabilities: Sense or switch power on an Orvibo S20 device.
     Change the name (or remote-access password) of a device.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Limitations:

   o Free.  Complaints are always welcome, but no reliable support is
     offered by the author.

   o Orvibo S20 devices must be configured for the wireless network
     using some other program, such as the Orvibo "WiWo" application.
     Currently, ORVL can communicate with a device only after that
     device has connected to the local wireless network.

   o Currently, ORVL knows nothing about the timer capabilities of the
     S20 device.

   o ORVL knows nothing about any device type other than the S20.

------------------------------------------------------------------------

      Building ORVL
      -------------

   Customization
   -------------

   Comments in the C source file (orvl.c) describe some possible
customizations, and the C macros which control them.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   UNIX
   ----

   On UNIX (or UNIX-like) systems (AIX, HP-UX, GNU/Linux, macOS,
Solaris, Tru64, and so on), simply compile and link.  For example:

      cc -o orvl orvl.c

   On some systems, some compile/link options may be needed.  Comments
in the source file offer some suggestions.  For example, on Solaris:

      cc -o orvl -DNEED_SYS_FILIO_H orvl.c -lsocket

(Who needs a stinking "configure" script?)

   As usual, the resulting executable must reside in a directory on the
user's PATH, or else the user must specify an explicit path to the
shell.  For example: "./orvl".

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   VMS
   ---

      cc orvl
      link orvl

   As usual, the resulting executable must reside in a directory on the
user's DCL$PATH, or else the user can define a DCL foreign-command
symbol using a command like the following (where "DEV:[DIR]" represents
the actual device and directory where the executable can be found):

      orvl == "$ DEV:[DIR]orvl.exe"

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Windows
   -------

   A very simple Visual Studio solution/project file set is included in
the "win32" directory: win32\orvl.sln (and friends).

   As usual, the resulting executable must reside in a directory on the
user's PATH, or else the user must specify an explicit path to the
shell (CMD or PowerShell).  For example: "win32\Release\orvl".

------------------------------------------------------------------------

   Orvibo S20 Fundamentals
   -----------------------

   Orvibo has not published an interface specification for its devices,
so ORVL relies on publicly available data based on reverse engineering.
These data are incomplete and not perfectly reliable, but seem to be
adequate for basic operations.

   The Orvibo S20 and ORVL use UDP datagrams for communication.  The
details are outlined in a document, "Orvibo Wifi Socket", found at:

      http://pastebin.com/LfUhsbcS

An edited version of that file is supplied in the ORVL kit:

      Orvibo_Wifi_Socket.txt

   The "GLOBAL DISCOVERY DATA" and "DISCOVERY DATA" response sections of
the original document describe what is called "Time since manufacture?"
in three bytes.  Actually, this seems to be a current UTC time (seconds
since 1900-01-01), in four bytes (including the byte labeled
"??? Unknown ???" in the orginal document).  The original arithmetic
shown there used "255" where it should have used "256".

   Additional documents of interest:

https://stikonas.eu/wordpress/2015/02/24/reverse-engineering-orvibo-s20-socket/
https://github.com/fernadosilva/orvfms/blob/master/TECHNICAL_DATA.txt

   Your own Web search may find more.  (Or, you may contribute more,
based on your own research.)

------------------------------------------------------------------------

      ORVL Usage
      ----------

    ORVL 0.2      Usage: orvl [ options ] operation [ identifier ]

Options:    debug[=value]       Set debug flags, all or selected.
            ddf[=file_spec]     Use device data file.  Default: ORVL_DDF
                                 (normally an env-var or logical name).
            name=device_name    New device name for "set" operation.
            password=passwd     New remote password for "set" operation.
            brief               Simplify [q]list and off/on reports.
            quiet               Suppress [q]list and off/on reports.
            sort={ip|mac}       Sort devs by IP or MAC addr.  Default: ip

Operations: help, usage         Display this help/usage text.
            list                List devices.  (Minimal device queries.)
            qlist               Query and list devices.  (Query device(s)
                                 to get detailed device information.)
            off, on             Switch off/on.  Identifier required.
            set                 Set dev data (name, password).  Ident req'd.
            version             Show program version.

Identifier: DNS name            DNS name, numeric IP address, or dev name.
            IP address          Used with operations "off", "on", or "set",
            Device name         or to limit a [q]list report to one device.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   ORVL Program Operation
   ----------------------

   ORVL command-line option and operation keywords may be abbreviated to
the shortest unambiguous strings.  Keyword case is not significant.
(Case may be significant for device names, and for file names on some
systems).  for example, the following commands are equivalent:

      orvl ddf brief qlist
      orvl dd br ql
      orvl DD Brief Ql

   On the command line, any option keywords must precede the (required)
operation keyword.

   For the device control operations, "off" and "on", and for the the
"set" operation, a device identifier is required.  For the list/sense
operations, "list" and "qlist", a device identifier is optional.  At
most, one device identifier is allowed, and it must follow the operation
keyword.

   If no device identifier is specified for a list/sense operation, then
all known devices are included in the report.  If a (valid) device
identifier is specified for a list/sense operation, then only that
device is included in the report.

   A device identifier may be a DNS name, an IP (IPv4) address, or a
device name (which can be assigned to a device using ORVL, the WiWo
application, or some other program).

   A DNS name is resolved to an IP address in the usual way, and the
usual dotted-octet format can be used for a numeric IP address.  An
Orvibo device normally gets its IP address using DHCP, so these
names/addresses may be reliable only if the DHCP server (such as a home
IP router) is configured to provide a fixed address for each Orvibo
device.

   If an identifier can't be resolved as a DNS name or interpreted as an
IP address, then ORVL treats it as a device name.  A device name is
stored on the device itself, so using a device name can can require
additional message exchanges with the devices to obtain their device
names.

   To sense or control a device, ORVL needs the IP address and the MAC
address of that device.  ORVL can get these data from the devices by
using broadcast and specific query messages.  Device name information
can also be gathered this way.  After ORVL has taken a full device
inventory, it can match a user-specified device name in a command
against the names of the known devices.  Conversely, to deal with a
user-specified device name in a command, ORVL first takes a full device
inventory.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Device Data File (DDF)
   ----------------------

   The Orvibo WiWo application can be started once, and then used to
perform multiple operations.  Unlike the WiWo application, ORVL is not
persistent; ORVL is executed once for each operation.  If necessary,
ORVL can take a full device inventory every time it is run, but this is
not very efficient, and taking the inventory can take some time, perhaps
several seconds, depending on the device count.

   Alternatively, ORVL can use a device data file (DDF) to provide a
device inventory.  Using a DDF can make some common operations faster.
Of course, if a DDF is used, and the device configuration changes (a new
device is added, an old device is removed, the IP address of a device
changes, a device name is changed, and so on), then the user must update
the DDF to reflect the new configuration.  The user must judge the
benefits and costs of using a DDF.

   A DDF can be created manually, but the easiest way is normally to let
ORVL take an inventory (using the "qlist" operation), and save the
result.  (The user can then edit this file manually, if desired.)  A
command like the following does this:

      orvl qlist > orvl.dat

      pipe orvl qlist > orvl.dat        ! VMS.

   A typical resulting DDF might look like this:

#      ORVL  0.2  --  Devices (probe: 2)          2017-03-21:16:52:39
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.120       ac:cf:23:48:ed:12  >Socket00<          # Off    Socket-US
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # Off    Socket-US

   "#" is the comment delimiter.  The first three lines compose a
heading which shows the ORVL program name and version, the fact that the
data came from probing devices (not from a file), the number of devices
which are known, and the date-time when the report was generated.

   After the heading, the devices appear, one per line.  The device
inventory data are in the first three fields, "IP address", "MAC
address", and ">Device name<".  These are the data which ORVL uses for
device communication.  The "State" and "Type" items are included as
comments, but are ignored by ORVL.  ("State", in particular, is subject
to change.)

   The ORVL "qlist" report has a fixed format, but the actual spacing
between fields is not critical for use as a DDF, so manual editing is
fairly safe.  The order of the fields and the delimiters used (".", ":")
_are_ critical, however.

   The ">" and "<" characters around the device name are used in case
space characters are embedded in the name.  Testing on the iOS WiWo app
suggests that it does not allow "<" and ">" characters in a device name.

   By default, ORVL uses the environment variable ORVL_DDF to specify a
DDF path/name.  To use a file named "orvl.dat" in one's home directory,
for example:

      ORVL_DDF=${HOME}/orvl.dat ; export ORVL_DDF       # UNIX (Bourne)

      define ORVL_DDF sys$login:orvl.dat                ! VMS

      set ORVL_DDF = %HOMEPATH%\orvl.dat                (Windows CMD)

      $env:ORVL_DDF = "$env:USERPROFILE\orvl.dat"       # Windows PS

Then, specify the option "ddf" in the ORVL command:

      orvl ddf list

Alternatively, specify the path/name of the DDF using a "ddf=" option.
For example:

      orvl ddf=orvl.dat list

. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

   It's possible for DDF data to be inconsistent with the actual device
data, if the device configuration changes after the DDF is created.  If
that happens, the ORVL device report can vary, depending on whether
devices queries are used.  Data from a device query override data read
from a DDF.  For example, here's a DDF on a VMS system:

WISP $ type ORVL_DDF
#      ORVL  0.1  --  Devices (probe: 2)          2017-03-03:00:18:10
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.120       ac:cf:23:48:ed:12  >Socket00<          # Off    Socket-US
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # On     Socket-US

   Using this DDF, a "list" operation shows mostly the data from the DDF
(with only the "State" data from simple queries):

WISP $ orvl ddf list
#      ORVL  0.2  --  Devices (ddf: 2)            2017-03-21:23:11:10
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.120       ac:cf:23:48:ed:12  >Socket00<          # Off    
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # On     

   However, even using this DDF, a "qlist" operation will show device
data from detailed device queries, including a different device name for
one of the devices:

WISP $ orvl ddf qlist
#      ORVL  0.2  --  Devices (ddf: 2)            2017-03-21:23:14:10
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.120       ac:cf:23:48:ed:12  >DeskLamp<          # Off    Socket-US
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # On     Socket-US

   ORVL never writes to the DDF.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Naming a Device
   ---------------

   To use ORVL to assign a name to a device, specify the name using the
"name=" option, and the "set" operation.  For example:

      orvl name=DeskLamp set Socket00

Typical output:

alp $ orvl brief name=DeskLamp set Socket00
10.0.0.120       ac:cf:23:48:ed:12  >DeskLamp<          # Off    Socket-US

   See "Device Name Details", below, for more information on device
names.

   After ORVL sends the commands to set device data, it sends a query
command to verify that the setting occurred as requested.  Failure
should cause error messages and a failure exit status.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Device Name Details
   -------------------

   The maximum length of a device name is 16 characters.

   Documentation on the WiWo app seems to be sparse.  As this is
written, the (iOS) WiWo app seems to allow space characters in a device
name, but then may not display them.  This behavior (bug or intentional)
in the WiWo app could change in the future.  ORVL should work with
spaces in device names (and does not hide them), but, as with other
programs, the user needs to use escapes or quotation on the command line
to get such spaces into the program.  For example, on VMS:

alp $ orvl brief on "Socket 01"
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # On     Socket-US

   On a UNIX(-like) system, the shells can also use backslash escapes or
apostrophe quotation.  For example, on a Mac:

mba$ ./orvl brief off Socket\ 01
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # Off    Socket-US

mba$ ./orvl brief on 'Socket 01'
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # On     Socket-US

   The same methods must be used for any other shell-special characters
which might appear in a device name.

   When a device has not been assigned a device name, the WiWo app seems
to display a generic device name, like "Socket".  ORVL displays an
unnamed device as ">(unset)<", and requires a DNS name or IP address to
specify such a device.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   List/Sense Command Examples
   ---------------------------

   To see the states of all devices, use the operation "list" or
"qlist".  Getting a "list" report can be faster, but the "list" report
may not include the device names, unless a DDF is used, and may not
include the device type.  For example:

      orvl list
      orvl ddf list
      orvl qlist
      orvl brief qlist          # "brief" suppresses the report heading.

   To get the state of one device, specify one of its identifiers.  For
example:

      orvl list 10.0.0.120
      orvl list orvibo-00       # This DNS name resolves to 10.0.0.120,
                                # in the author's network.
      orvl brief qlist Socket00 # Equivalent to 10.0.0.120, but slower.

Similarly, using a DDF:

      orvl ddf list 10.0.0.120
      orvl ddf list orvibo-00
      orvl ddf brief list Socket00

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Control Command Examples
   ------------------------

   To control a device, use the "off" or "on" operation keyword, and
specify a particular device.  For example:

      orvl off 10.0.0.120
      orvl brief on orvibo-00   # "brief" suppresses the report heading.
      orvl quiet off Socket00   # "quiet" suppresses the usual report.

   Typical behavior (here, on a Mac):

mba$ ./orvl off 10.0.0.120

#      ORVL  0.2  --  Devices (probe: 1)          2017-03-21:21:51:06
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.120       ac:cf:23:48:ed:12  ><                  # Off    

mba$ orvl brief on orvibo-00
10.0.0.120       ac:cf:23:48:ed:12  ><                  # On     

mba$ ./orvl quiet off Socket00
mba$

   As with the list/sense operations, using a DDF, or specifying a DNS
name or IP address (instead of a device name), can save time by avoiding
device queries.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Exit Status
   -----------

   When ORVL exits, it returns a status value which indicates success or
failure.  For general operations, like "help", "list", "qlist", "usage"
and "version", on UNIX(-like) or Windows systems, this will be zero for
success, non-zero for failure.  On VMS: SS$_NORMAL (1) for success, and
some error-severity value like %X10000002 (%NONAME-E-NOMSG) for failure.
For example (on a Mac):

      mba$ orvl version
      ORVL 0.2

      mba$ echo $?
      0

      mba$ orvl brief list fred
      ORVL: Device name not matched (loc=0): >fred<.

      mba$ echo $?
      1

   For operations where a particular device is specified, like "off",
"on", or "set", or for "list" or "qlist" with a device identifier, the
exit status should include the device state encoded in bits 7:4 of the
value.  For example, on a Mac:

      mba$ orvl brief on Socket00
      10.0.0.120       ac:cf:23:48:ed:12  >Socket00<      # On   Socket-US
      mba$ echo $?
      48

      mba$ orvl quiet off Socket00
      mba$ echo $?
      32

      mba$ ./orvl quiet qlist Socket00
      mba$ echo $?
      32

   That is, "off" is encoded as 32 (16* 2), and "on" is encoded as 48
(16* 3).  The state is encoded in the exit status only when the
operation was a success, so the low four bits are always the same as in
the usual success code (zero on the non-VMS systems, and one on VMS).
For example, on VMS:

      alp $ orvl brief on orvibo-00
      10.0.0.120       ac:cf:23:48:ed:12  ><                  # On
      alp $ write sys$output $status
      %X00000031

      alp $ orvl quiet off orvibo-00
      alp $ write sys$output $status
      %X00000021

      alp $ orvl quiet qlist orvibo-00
      alp $ write sys$output $status
      %X00000021

   On Windows (CMD):

      C:\Users\sms\orvibo\orvl>win32\Debug\orvl quiet qlist orvibo-00
      C:\Users\sms\orvibo\orvl>echo %errorlevel%
      32

   On Windows (PowerShell):

      PS C:\Users\sms\orvibo\orvl> win32\Debug\orvl quiet on orvibo-00
      PS C:\Users\sms\orvibo\orvl> echo $lastexitcode
      48

   This feature makes it possible to determine the reported device state
without capturing and parsing the text output from such commands.

   Delivery of UDP messages is not guaranteed, and, on occasion, for
whatever reason, a device may fail to respond to a message.  ORVL does
retry an operation when it fails, but retries can also fail.  Also, ORVL
does not make multiple attempts to take a device inventory, so, if a DDF
is not used, and a device fails to respond to an initial broadcast
query, then it may be treated as nonexistent.  (C macros in orvl.c with
"TASK_RETRY" in their names control this retry behavior.)

   A well-written script (or other program) which uses ORVL will always
check the exit status returned by ORVL to detect a failure or an
unexpected state value, and then retry the operation, or report the
problem to the responsible party.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Other Options
   -------------
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

   "debug[=value]" enables various diagnostic messages, which can help
when debugging or tracing the execution of the program.  Different bits
in the integer "value" enable different categories of messages.  If no
value is specified, then all categories are enabled.  Comments in the
source file describe which bits do what.  (Look for "DBG_" in orvl.c.)

. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

   "sort={ip|mac}" controls the order in which devices appear in "list"
or "qlist" reports.  By default, devices appear in order of IP address.
With "sort=mac", devices appear in order of MAC address.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Other Operations
   ----------------
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

   "version" causes ORVL to display its version.  For example:

      alp $ orvl version
      ORVL 0.2

. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

   "heartbeat" causes ORVL to send a heartbeat message to a specified
device.  If the device is currently claimed/subscribed, then it should
return a "heartbeat" response.  ORVL puts out an error message, and
returns a failure status, if the expected response is not received.

   One could guess that such a heartbeat message exchange resets some
timer in the device related to maintaining its claimed/subscribed
status, but that's only a guess.

   This command may have some use as a diagnostic, but, for a
non-persistent program like ORVL, it may not be useful.

------------------------------------------------------------------------

      Miscellaneous
      -------------
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Device Listings
   ---------------

   For operations which involve communicating with devices, like "list",
"qlist", "off", "on", and "set" (not "help", "usage", or "version"),
ORVL normally ends its execution with a device listing which may include
device name and state information.  Specifying the "quiet" option
suppresses this report.

   Specifying the "brief" option suppresses the heading of this report.
The heading includes the program name and version, whether the data came
from probing devices or from a file (DDF), the number of devices which
responded, and the date-time when the report was generated.  For
example:

alp $ orvl qlist
#      ORVL  0.2  --  Devices (probe: 2)          2017-03-21:17:33:04
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.120       ac:cf:23:48:ed:12  >DeskLamp<          # Off    Socket-US
10.0.0.121       ac:cf:23:9c:b2:14  >Sock-01<           # On     Socket-US

   If the operation uses a broadcast query (like the above, "qlist"
without a specific device identifier), then the "Devices" count should
include all the devices which responded to the broadcast query.  If a
DDF is used, and no device identifier is specified, then the "Devices"
count should include all the devices in the DDF.

   If the user specifies a device identifier in the command, then the
body of the report, that is, the devices in the listing after the
heading, should include only the specified device, but the "Devices"
count in the header may be higher, because it may include other devices
which responded to a broadcast query.  When a DDF is not used,
specifying a device name (not a DNS name or IP address) causes ORVL to
use a broadcast query to collect the required device name information.

   Thus, asking about a device by its DNS name or IP address will
normally return a "Devices" count of 1.  For example:

alp $ orvl qlist 10.0.0.121
#      ORVL  0.2  --  Devices (probe: 1)          2017-03-21:17:58:53
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.121       ac:cf:23:9c:b2:14  >Sock-01<           # On     Socket-US

   But, asking about the same device by its device name (without using a
DDF) may return a higher "Devices" count, which would include all the
devices responding to the broadcast query needed to handle the device
name.  For example:

alp $ orvl qlist Sock-01
#      ORVL  0.2  --  Devices (probe: 2)          2017-03-21:18:06:55
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.121       ac:cf:23:9c:b2:14  >Sock-01<           # On     Socket-US

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Timing Considerations
   ---------------------

   The Orvibo S20 can return multiple messages (datagrams) in response
to a single operation/query message.  After it sends a message, ORVL
goes into a loop, attempting to read and process every response which it
receives.  The C macro SOCKET_TIMEOUT determines how long ORVL will wait
for a response before exiting the read loop.  By default, SOCKET_TIMEOUT
is set to 0.5s.  Shorter times will speed operations.  Very short times
could cause loss of messages.  The author has done little
experimentation to determine an optimal value.

   An operation like "set" involves multiple messages, so it can take
significantly longer than a simple "list" query by DNS name or IP
address.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Case Preservation on VMS
   ------------------------

   Generally, case matters for only device names.  On VMS systems, ORVL
preserves the case of command-line parameters on systems where it can.
That is, on sufficiently recent non-VAX systems where
      SET PROCESS /PARSE_STYLE = EXTENDED
is in effect.

   On VAX (or very old non-VAX) systems, or on non-VAX systems with SET
PROCESS /PARSE_STYLE = TRADITIONAL, the user will need to quote device
names which include upper-case characters.  For example (here, on a
VAX):

      WISP $ orvl ddf brief list Socket00
      ORVL: Device name not matched (loc=0): >socket00<.

      WISP $ orvl ddf brief list "Socket00"
      10.0.0.120       ac:cf:23:48:ed:12  >Socket00<          # On

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   DDF, Broadcast Messages, and Sub-networks
   -----------------------------------------

   When ORVL takes a device inventory, it uses a broadcast message
(Global Discovery, "qa", to IP address 255.255.255.255).  Normally, an
IP router does not pass such broadcast messages from one sub-network to
another, and that prevents ORVL from detecting devices which are on a
different sub-network.

   In the following examples, WISP is a SIMH (emulated) VAX (running
VMS V7.3) on the 10.0.1.x sub-network, and the Orvibo devices are all on
the 10.0.0.x sub-network.  In this situation, with the devices on a
different sub-network, a general listing, or an operation specifying a
device name, fails.  For example:

WISP $ orvl list                        ! No DDF: General list fails...
ORVL: No devices found (loc=list).

WISP $ orvl brief list "Socket00"       ! No DDF: Device name fails...
ORVL: No devices found (loc=cdl).

   Operations which do not require a full (broadcast) device inventory
can still work between sub-networks.  This includes any operation on a
specific device which is identified by a DNS name or IP address.  For
example:

WISP $ orvl brief list orvibo-00        ! DNS name: Operation works...
10.0.0.120       ac:cf:23:48:ed:12  ><                  # Off

WISP $ orvl brief on 10.0.0.120         ! IP address: Operation works...
10.0.0.120       ac:cf:23:48:ed:12  ><                  # On

   When ORVL uses a DDF, it gets its device inventory from the DDF, so
it does not need to take a live device inventory, so it does not need to
use broadcast messages.  Using a DDF, general listings work, and a
device name can be used as a device identifier, even between
sub-networks.  For example:

WISP $ orvl ddf list                    ! DDF: General list works...
#      ORVL  0.2  --  Devices (ddf: 2)            2017-03-21:20:58:41
# IP address        MAC address     >Device name<       # State  Type
#-----------------------------------------------------------------------
10.0.0.120       ac:cf:23:48:ed:12  >Socket00<          # Off    
10.0.0.121       ac:cf:23:9c:b2:14  >Socket 01<         # On     

WISP $ orvl ddf brief list "Socket00"   ! DDF: Device name works...
10.0.0.120       ac:cf:23:48:ed:12  >Socket00<          # Off    

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   Process Privileges on VMS
   -------------------------

   On VMS systems, sending a UDP broacast message (as for a "qlist"
operation without a DDF) may require elevated process privilege (BYPASS,
OPER, or SYSPRV), unless such broadcast has been enabled system-wide by
a command like "TCPIP SET PROTOCOL UDP /BROADCAST".

   ORVL attempts to acquire OPER privilege when necessary, which should
work if the user has OPER (or SETPRV) as an authorized privilege.  This
privilege requirement can be avoided by using a DDF, or by specifying
devices using only DNS names or IP addresses, not device names.  To
make full operation possible for users without such privileges, the ORVL
executable must be installed with the OPER privilege.

  To install ORVL with the OPER privilege, it must first be linked with
the /NOTRACEBACK option (and without /DEBUG).  For example:

      link /notraceback orvl

   Then, use the Install utility (INSTALL) to install the executable
image with the required privilege:

      install add /privileged = oper DEV:[DIR]orvl

(Here, "DEV:[DIR]" represents the actual device and directory where the
executable can be found.)

   Use "replace" instead of "add", if the image has already been
installed.

   Without the required privilege, an operation like "qlist" may fail
with messages like the following:

alp $ orvl qlist
ORVL: Set privilege (OPER) failed.  sts = %x00000681 .
ORVL: errno = 65535, vaxc$errno = 00000681.
ORVL: not all requested privileges authorized
ORVL: setsockopt( snd-bc1) failed.
ORVL: errno = 13, vaxc$errno = 00000024.
ORVL: permission denied

   (ORVL tries to set the privilege, but proceeds even if that fails.
Then the first socket-related I/O involving broadcast may fail, as in
this case.)

------------------------------------------------------------------------

      History
      -------
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

2017-03-08  Version 0.1

- New.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

2017-03-22  Version 0.2

- Added a "set" operation with "name=device_name" and "password=passwd"
  options to set some device properties.

- Added a (probably useless) "heartbeat" operation to send an "hb"
  message to a device (and detect an "hb" response).

- In a DDF, space characters in a device name were mishandled.  The
  device name was truncated at the first space.

- A device report heading is now put out only if some device data
  follow.

- Diagnostic bit mask ("debug=value") values have changed.

------------------------------------------------------------------------
