/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

 /* Host communication command constants for Chrome EC */

#ifndef __CROS_EC_COMMANDS_H
#define __CROS_EC_COMMANDS_H

/*
 * Current version of this protocol
 *
 * TODO(crosbug.com/p/11223): This is effectively useless; protocol is
 * determined in other ways.  Remove this once the kernel code no longer
 * depends on it.
 */
#define EC_PROTO_VERSION          0x00000002

 /* Command version mask */
#define EC_VER_MASK(version) (1UL << (version))

/* I/O addresses for ACPI commands */
#define EC_LPC_ADDR_ACPI_DATA  0x62
#define EC_LPC_ADDR_ACPI_CMD   0x66

/* I/O addresses for host command */
#define EC_LPC_ADDR_HOST_DATA  0x200
#define EC_LPC_ADDR_HOST_CMD   0x204

/* I/O addresses for host command args and params */
/* Protocol version 2 */
#define EC_LPC_ADDR_HOST_ARGS    0x800  /* And 0x801, 0x802, 0x803 */
#define EC_LPC_ADDR_HOST_PARAM   0x804  /* For version 2 params; size is
					 * EC_PROTO2_MAX_PARAM_SIZE */
					 /* Protocol version 3 */
#define EC_LPC_ADDR_HOST_PACKET  0x800  /* Offset of version 3 packet */
#define EC_LPC_HOST_PACKET_SIZE  0x100  /* Max size of version 3 packet */

/* The actual block is 0x800-0x8ff, but some BIOSes think it's 0x880-0x8ff
 * and they tell the kernel that so we have to think of it as two parts. */
#define EC_HOST_CMD_REGION0    0x800
#define EC_HOST_CMD_REGION1    0x880
#define EC_HOST_CMD_REGION_SIZE 0x80

 /* EC command register bit functions */
#define EC_LPC_CMDR_DATA	(1 << 0)  /* Data ready for host to read */
#define EC_LPC_CMDR_PENDING	(1 << 1)  /* Write pending to EC */
#define EC_LPC_CMDR_BUSY	(1 << 2)  /* EC is busy processing a command */
#define EC_LPC_CMDR_CMD		(1 << 3)  /* Last host write was a command */
#define EC_LPC_CMDR_ACPI_BRST	(1 << 4)  /* Burst mode (not used) */
#define EC_LPC_CMDR_SCI		(1 << 5)  /* SCI event is pending */
#define EC_LPC_CMDR_SMI		(1 << 6)  /* SMI event is pending */

#define EC_LPC_ADDR_MEMMAP       0x900
#define EC_MEMMAP_SIZE         255 /* ACPI IO buffer max is 255 bytes */
#define EC_MEMMAP_TEXT_MAX     8   /* Size of a string in the memory map */

/* The offset address of each type of data in mapped memory. */
#define EC_MEMMAP_TEMP_SENSOR      0x00 /* Temp sensors 0x00 - 0x0f */
#define EC_MEMMAP_FAN              0x10 /* Fan speeds 0x10 - 0x17 */
#define EC_MEMMAP_TEMP_SENSOR_B    0x18 /* More temp sensors 0x18 - 0x1f */
#define EC_MEMMAP_ID               0x20 /* 0x20 == 'E', 0x21 == 'C' */
#define EC_MEMMAP_ID_VERSION       0x22 /* Version of data in 0x20 - 0x2f */
#define EC_MEMMAP_BATTERY_VERSION  0x24 /* Version of data in 0x40 - 0x7f */
#define EC_MEMMAP_HOST_CMD_FLAGS   0x27 /* Host cmd interface flags (8 bits) */
/* Unused 0x28 - 0x2f */
#define EC_MEMMAP_SWITCHES         0x30	/* 8 bits */
/* Reserve 0x38 - 0x3f for additional host event-related stuff */
/* Battery values are all 32 bits */
#define EC_MEMMAP_BATT_VOLT        0x40 /* Battery Present Voltage */
#define EC_MEMMAP_BATT_RATE        0x44 /* Battery Present Rate */
#define EC_MEMMAP_BATT_CAP         0x48 /* Battery Remaining Capacity */
#define EC_MEMMAP_BATT_FLAG        0x4c /* Battery State, defined below */
#define EC_MEMMAP_BATT_DCAP        0x50 /* Battery Design Capacity */
#define EC_MEMMAP_BATT_DVLT        0x54 /* Battery Design Voltage */
#define EC_MEMMAP_BATT_LFCC        0x58 /* Battery Last Full Charge Capacity */
#define EC_MEMMAP_BATT_CCNT        0x5c /* Battery Cycle Count */
/* Strings are all 8 bytes (EC_MEMMAP_TEXT_MAX) */
#define EC_MEMMAP_BATT_MFGR        0x60 /* Battery Manufacturer String */
#define EC_MEMMAP_BATT_MODEL       0x68 /* Battery Model Number String */
#define EC_MEMMAP_BATT_SERIAL      0x70 /* Battery Serial Number String */
#define EC_MEMMAP_BATT_TYPE        0x78 /* Battery Type String */
/* Unused 0xa6 - 0xdf */

/*
 * ACPI is unable to access memory mapped data at or above this offset due to
 * limitations of the ACPI protocol. Do not place data in the range 0xe0 - 0xfe
 * which might be needed by ACPI.
 */
#define EC_MEMMAP_NO_ACPI 0xe0

 /* Battery bit flags at EC_MEMMAP_BATT_FLAG. */
#define EC_BATT_FLAG_AC_PRESENT   0x01
#define EC_BATT_FLAG_BATT_PRESENT 0x02
#define EC_BATT_FLAG_DISCHARGING  0x04
#define EC_BATT_FLAG_CHARGING     0x08
#define EC_BATT_FLAG_LEVEL_CRITICAL 0x10

/* Switch flags at EC_MEMMAP_SWITCHES */
#define EC_SWITCH_LID_OPEN               0x01
#define EC_SWITCH_POWER_BUTTON_PRESSED   0x02
#define EC_SWITCH_WRITE_PROTECT_DISABLED 0x04
/* Was recovery requested via keyboard; now unused. */
#define EC_SWITCH_IGNORE1		 0x08
/* Recovery requested via dedicated signal (from servo board) */
#define EC_SWITCH_DEDICATED_RECOVERY     0x10
/* Was fake developer mode switch; now unused.  Remove in next refactor. */
#define EC_SWITCH_IGNORE0                0x20

/* Host command interface flags */
/* Host command interface supports LPC args (LPC interface only) */
#define EC_HOST_CMD_FLAG_LPC_ARGS_SUPPORTED  0x01
/* Host command interface supports version 3 protocol */
#define EC_HOST_CMD_FLAG_VERSION_3   0x02

/*****************************************************************************/
/*
 * ACPI commands
 *
 * These are valid ONLY on the ACPI command/data port.
 */

 /*
  * ACPI Read Embedded Controller
  *
  * This reads from ACPI memory space on the EC (EC_ACPI_MEM_*).
  *
  * Use the following sequence:
  *
  *    - Write EC_CMD_ACPI_READ to EC_LPC_ADDR_ACPI_CMD
  *    - Wait for EC_LPC_CMDR_PENDING bit to clear
  *    - Write address to EC_LPC_ADDR_ACPI_DATA
  *    - Wait for EC_LPC_CMDR_DATA bit to set
  *    - Read value from EC_LPC_ADDR_ACPI_DATA
  */
#define EC_CMD_ACPI_READ 0x80

  /*
   * ACPI Write Embedded Controller
   *
   * This reads from ACPI memory space on the EC (EC_ACPI_MEM_*).
   *
   * Use the following sequence:
   *
   *    - Write EC_CMD_ACPI_WRITE to EC_LPC_ADDR_ACPI_CMD
   *    - Wait for EC_LPC_CMDR_PENDING bit to clear
   *    - Write address to EC_LPC_ADDR_ACPI_DATA
   *    - Wait for EC_LPC_CMDR_PENDING bit to clear
   *    - Write value to EC_LPC_ADDR_ACPI_DATA
   */
#define EC_CMD_ACPI_WRITE 0x81

   /*
	* ACPI Burst Enable Embedded Controller
	*
	* This enables burst mode on the EC to allow the host to issue several
	* commands back-to-back. While in this mode, writes to mapped multi-byte
	* data are locked out to ensure data consistency.
	*/
#define EC_CMD_ACPI_BURST_ENABLE 0x82

	/*
	 * ACPI Burst Disable Embedded Controller
	 *
	 * This disables burst mode on the EC and stops preventing EC writes to mapped
	 * multi-byte data.
	 */
#define EC_CMD_ACPI_BURST_DISABLE 0x83

	 /*
	  * ACPI Query Embedded Controller
	  *
	  * This clears the lowest-order bit in the currently pending host events, and
	  * sets the result code to the 1-based index of the bit (event 0x00000001 = 1,
	  * event 0x80000000 = 32), or 0 if no event was pending.
	  */
#define EC_CMD_ACPI_QUERY_EVENT 0x84

	  /* Valid addresses in ACPI memory space, for read/write commands */

	  /* Memory space version; set to EC_ACPI_MEM_VERSION_CURRENT */
#define EC_ACPI_MEM_VERSION            0x00
/*
 * Test location; writing value here updates test compliment byte to (0xff -
 * value).
 */
#define EC_ACPI_MEM_TEST               0x01
 /* Test compliment; writes here are ignored. */
#define EC_ACPI_MEM_TEST_COMPLIMENT    0x02

/*
 * ACPI addresses 0x20 - 0xff map to EC_MEMMAP offset 0x00 - 0xdf.  This data
 * is read-only from the AP.  Added in EC_ACPI_MEM_VERSION 2.
 */
#define EC_ACPI_MEM_MAPPED_BEGIN   0x20
#define EC_ACPI_MEM_MAPPED_SIZE    0xe0

 /* Current version of ACPI memory address space */
#define EC_ACPI_MEM_VERSION_CURRENT 2


/*
 * This header file is used in coreboot both in C and ACPI code.  The ACPI code
 * is pre-processed to handle constants but the ASL compiler is unable to
 * handle actual C code so keep it separate.
 */
#ifndef __ACPI__

 /* LPC command status byte masks */
 /* EC has written a byte in the data register and host hasn't read it yet */
#define EC_LPC_STATUS_TO_HOST     0x01
/* Host has written a command/data byte and the EC hasn't read it yet */
#define EC_LPC_STATUS_FROM_HOST   0x02
/* EC is processing a command */
#define EC_LPC_STATUS_PROCESSING  0x04
/* Last write to EC was a command, not data */
#define EC_LPC_STATUS_LAST_CMD    0x08
/* EC is in burst mode */
#define EC_LPC_STATUS_BURST_MODE  0x10
/* SCI event is pending (requesting SCI query) */
#define EC_LPC_STATUS_SCI_PENDING 0x20
/* SMI event is pending (requesting SMI query) */
#define EC_LPC_STATUS_SMI_PENDING 0x40
/* (reserved) */
#define EC_LPC_STATUS_RESERVED    0x80

/*
 * EC is busy.  This covers both the EC processing a command, and the host has
 * written a new command but the EC hasn't picked it up yet.
 */
#define EC_LPC_STATUS_BUSY_MASK \
	(EC_LPC_STATUS_FROM_HOST | EC_LPC_STATUS_PROCESSING)

 /* Host command response codes */
enum ec_status {
	EC_RES_SUCCESS = 0,
	EC_RES_INVALID_COMMAND = 1,
	EC_RES_ERROR = 2,
	EC_RES_INVALID_PARAM = 3,
	EC_RES_ACCESS_DENIED = 4,
	EC_RES_INVALID_RESPONSE = 5,
	EC_RES_INVALID_VERSION = 6,
	EC_RES_INVALID_CHECKSUM = 7,
	EC_RES_IN_PROGRESS = 8,		/* Accepted, command in progress */
	EC_RES_UNAVAILABLE = 9,		/* No response available */
	EC_RES_TIMEOUT = 10,		/* We got a timeout */
	EC_RES_OVERFLOW = 11,		/* Table / data overflow */
	EC_RES_INVALID_HEADER = 12,     /* Header contains invalid data */
	EC_RES_REQUEST_TRUNCATED = 13,  /* Didn't get the entire request */
	EC_RES_RESPONSE_TOO_BIG = 14,   /* Response was too big to handle */
	EC_RES_BUS_ERROR = 15,          /* Communications bus error */
	EC_RES_BUSY = 16                /* Up but too busy.  Should retry */
};

/*
 * Host event codes.  Note these are 1-based, not 0-based, because ACPI query
 * EC command uses code 0 to mean "no event pending".  We explicitly specify
 * each value in the enum listing so they won't change if we delete/insert an
 * item or rearrange the list (it needs to be stable across platforms, not
 * just within a single compiled instance).
 */
enum host_event_code {
	EC_HOST_EVENT_LID_CLOSED = 1,
	EC_HOST_EVENT_LID_OPEN = 2,
	EC_HOST_EVENT_POWER_BUTTON = 3,
	EC_HOST_EVENT_AC_CONNECTED = 4,
	EC_HOST_EVENT_AC_DISCONNECTED = 5,
	EC_HOST_EVENT_BATTERY_LOW = 6,
	EC_HOST_EVENT_BATTERY_CRITICAL = 7,
	EC_HOST_EVENT_BATTERY = 8,
	EC_HOST_EVENT_THERMAL_THRESHOLD = 9,
	EC_HOST_EVENT_THERMAL_OVERLOAD = 10,
	EC_HOST_EVENT_THERMAL = 11,
	EC_HOST_EVENT_USB_CHARGER = 12,
	EC_HOST_EVENT_KEY_PRESSED = 13,
	/*
	 * EC has finished initializing the host interface.  The host can check
	 * for this event following sending a EC_CMD_REBOOT_EC command to
	 * determine when the EC is ready to accept subsequent commands.
	 */
	 EC_HOST_EVENT_INTERFACE_READY = 14,
	 /* Keyboard recovery combo has been pressed */
	 EC_HOST_EVENT_KEYBOARD_RECOVERY = 15,

	 /* Shutdown due to thermal overload */
	 EC_HOST_EVENT_THERMAL_SHUTDOWN = 16,
	 /* Shutdown due to battery level too low */
	 EC_HOST_EVENT_BATTERY_SHUTDOWN = 17,

	 /* Suggest that the AP throttle itself */
	 EC_HOST_EVENT_THROTTLE_START = 18,
	 /* Suggest that the AP resume normal speed */
	 EC_HOST_EVENT_THROTTLE_STOP = 19,

	 /* Hang detect logic detected a hang and host event timeout expired */
	 EC_HOST_EVENT_HANG_DETECT = 20,
	 /* Hang detect logic detected a hang and warm rebooted the AP */
	 EC_HOST_EVENT_HANG_REBOOT = 21,

	 /* PD MCU triggering host event */
	 EC_HOST_EVENT_PD_MCU = 22,

	 /* Battery Status flags have changed */
	 EC_HOST_EVENT_BATTERY_STATUS = 23,

	 /* EC encountered a panic, triggering a reset */
	 EC_HOST_EVENT_PANIC = 24,

	 /*
	  * The high bit of the event mask is not used as a host event code.  If
	  * it reads back as set, then the entire event mask should be
	  * considered invalid by the host.  This can happen when reading the
	  * raw event status via EC_MEMMAP_HOST_EVENTS but the LPC interface is
	  * not initialized on the EC, or improperly configured on the host.
	  */
	  EC_HOST_EVENT_INVALID = 32
};
/* Host event mask */
#define EC_HOST_EVENT_MASK(event_code) (1UL << ((event_code) - 1))

#include <pshpack4.h>

/* Arguments at EC_LPC_ADDR_HOST_ARGS */
struct ec_lpc_host_args {
	UINT8 flags;
	UINT8 command_version;
	UINT8 data_size;
	/*
	 * Checksum; sum of command + flags + command_version + data_size +
	 * all params/response data bytes.
	 */
	UINT8 checksum;
};

#include <poppack.h>

/* Flags for ec_lpc_host_args.flags */
/*
 * Args are from host.  Data area at EC_LPC_ADDR_HOST_PARAM contains command
 * params.
 *
 * If EC gets a command and this flag is not set, this is an old-style command.
 * Command version is 0 and params from host are at EC_LPC_ADDR_OLD_PARAM with
 * unknown length.  EC must respond with an old-style response (that is,
 * withouth setting EC_HOST_ARGS_FLAG_TO_HOST).
 */
#define EC_HOST_ARGS_FLAG_FROM_HOST 0x01
 /*
  * Args are from EC.  Data area at EC_LPC_ADDR_HOST_PARAM contains response.
  *
  * If EC responds to a command and this flag is not set, this is an old-style
  * response.  Command version is 0 and response data from EC is at
  * EC_LPC_ADDR_OLD_PARAM with unknown length.
  */
#define EC_HOST_ARGS_FLAG_TO_HOST   0x02

  /*****************************************************************************/
  /*
   * Byte codes returned by EC over SPI interface.
   *
   * These can be used by the AP to debug the EC interface, and to determine
   * when the EC is not in a state where it will ever get around to responding
   * to the AP.
   *
   * Example of sequence of bytes read from EC for a current good transfer:
   *   1. -                  - AP asserts chip select (CS#)
   *   2. EC_SPI_OLD_READY   - AP sends first byte(s) of request
   *   3. -                  - EC starts handling CS# interrupt
   *   4. EC_SPI_RECEIVING   - AP sends remaining byte(s) of request
   *   5. EC_SPI_PROCESSING  - EC starts processing request; AP is clocking in
   *                           bytes looking for EC_SPI_FRAME_START
   *   6. -                  - EC finishes processing and sets up response
   *   7. EC_SPI_FRAME_START - AP reads frame byte
   *   8. (response packet)  - AP reads response packet
   *   9. EC_SPI_PAST_END    - Any additional bytes read by AP
   *   10 -                  - AP deasserts chip select
   *   11 -                  - EC processes CS# interrupt and sets up DMA for
   *                           next request
   *
   * If the AP is waiting for EC_SPI_FRAME_START and sees any value other than
   * the following byte values:
   *   EC_SPI_OLD_READY
   *   EC_SPI_RX_READY
   *   EC_SPI_RECEIVING
   *   EC_SPI_PROCESSING
   *
   * Then the EC found an error in the request, or was not ready for the request
   * and lost data.  The AP should give up waiting for EC_SPI_FRAME_START,
   * because the EC is unable to tell when the AP is done sending its request.
   */

   /*
	* Framing byte which precedes a response packet from the EC.  After sending a
	* request, the AP will clock in bytes until it sees the framing byte, then
	* clock in the response packet.
	*/
#define EC_SPI_FRAME_START    0xec

	/*
	 * Padding bytes which are clocked out after the end of a response packet.
	 */
#define EC_SPI_PAST_END       0xed

	 /*
	  * EC is ready to receive, and has ignored the byte sent by the AP.  EC expects
	  * that the AP will send a valid packet header (starting with
	  * EC_COMMAND_PROTOCOL_3) in the next 32 bytes.
	  */
#define EC_SPI_RX_READY       0xf8

	  /*
	   * EC has started receiving the request from the AP, but hasn't started
	   * processing it yet.
	   */
#define EC_SPI_RECEIVING      0xf9

	   /* EC has received the entire request from the AP and is processing it. */
#define EC_SPI_PROCESSING     0xfa

/*
 * EC received bad data from the AP, such as a packet header with an invalid
 * length.  EC will ignore all data until chip select deasserts.
 */
#define EC_SPI_RX_BAD_DATA    0xfb

 /*
  * EC received data from the AP before it was ready.  That is, the AP asserted
  * chip select and started clocking data before the EC was ready to receive it.
  * EC will ignore all data until chip select deasserts.
  */
#define EC_SPI_NOT_READY      0xfc

  /*
   * EC was ready to receive a request from the AP.  EC has treated the byte sent
   * by the AP as part of a request packet, or (for old-style ECs) is processing
   * a fully received packet but is not ready to respond yet.
   */
#define EC_SPI_OLD_READY      0xfd


  /* Parameter length was limited by the LPC interface */
#define EC_PROTO2_MAX_PARAM_SIZE 0xfc

/* Maximum request and response packet sizes for protocol version 2 */
#define EC_PROTO2_MAX_REQUEST_SIZE (EC_PROTO2_REQUEST_OVERHEAD +	\
				    EC_PROTO2_MAX_PARAM_SIZE)
#define EC_PROTO2_MAX_RESPONSE_SIZE (EC_PROTO2_RESPONSE_OVERHEAD +	\
				     EC_PROTO2_MAX_PARAM_SIZE)

/*****************************************************************************/

/*
 * Value written to legacy command port / prefix byte to indicate protocol
 * 3+ structs are being used.  Usage is bus-dependent.
 */
#define EC_COMMAND_PROTOCOL_3 0xda

#define EC_HOST_REQUEST_VERSION 3

#include <pshpack4.h>

 /* Version 3 request from host */
struct ec_host_request {
	/* Struct version (=3)
	 *
	 * EC will return EC_RES_INVALID_HEADER if it receives a header with a
	 * version it doesn't know how to parse.
	 */
	UINT8 struct_version;

	/*
	 * Checksum of request and data; sum of all bytes including checksum
	 * should total to 0.
	 */
	UINT8 checksum;

	/* Command code */
	UINT16 command;

	/* Command version */
	UINT8 command_version;

	/* Unused byte in current protocol version; set to 0 */
	UINT8 reserved;

	/* Length of data which follows this header */
	UINT16 data_len;
};

#define EC_HOST_RESPONSE_VERSION 3

/* Version 3 response from EC */
struct ec_host_response {
	/* Struct version (=3) */
	UINT8 struct_version;

	/*
	 * Checksum of response and data; sum of all bytes including checksum
	 * should total to 0.
	 */
	UINT8 checksum;

	/* Result code (EC_RES_*) */
	UINT16 result;

	/* Length of data which follows this header */
	UINT16 data_len;

	/* Unused bytes in current protocol version; set to 0 */
	UINT16 reserved;
};

#include <poppack.h>

/*****************************************************************************/
/*
 * Notes on commands:
 *
 * Each command is an 16-bit command value.  Commands which take params or
 * return response data specify structs for that data.  If no struct is
 * specified, the command does not input or output data, respectively.
 * Parameter/response length is implicit in the structs.  Some underlying
 * communication protocols (I2C, SPI) may add length or checksum headers, but
 * those are implementation-dependent and not defined here.
 */

 /*****************************************************************************/
 /* General / test commands */

 /*
  * Get protocol version, used to deal with non-backward compatible protocol
  * changes.
  */
#define EC_CMD_PROTO_VERSION 0x00

#include <pshpack4.h>

struct ec_response_proto_version {
	UINT32 version;
};

/*
 * Hello.  This is a simple command to test the EC is responsive to
 * commands.
 */
#define EC_CMD_HELLO 0x01

struct ec_params_hello {
	UINT32 in_data;  /* Pass anything here */
};

struct ec_response_hello {
	UINT32 out_data;  /* Output will be in_data + 0x01020304 */
};

#include <poppack.h>

/* Get version number */
#define EC_CMD_GET_VERSION 0x02

enum ec_current_image {
	EC_IMAGE_UNKNOWN = 0,
	EC_IMAGE_RO,
	EC_IMAGE_RW,
	EC_IMAGE_RW_A = EC_IMAGE_RW,
	EC_IMAGE_RO_B,
	EC_IMAGE_RW_B
};

#include <pshpack4.h>

struct ec_response_get_version {
	/* Null-terminated version strings for RO, RW */
	char version_string_ro[32];
	char version_string_rw[32];
	char reserved[32];       /* Was previously RW-B string */
	UINT32 current_image;  /* One of ec_current_image */
};

#include <poppack.h>

/*
 * Read memory-mapped data.
 *
 * This is an alternate interface to memory-mapped data for bus protocols
 * which don't support direct-mapped memory - I2C, SPI, etc.
 *
 * Response is params.size bytes of data.
 */
#define EC_CMD_READ_MEMMAP 0x07

#include <pshpack1.h>

struct ec_params_read_memmap {
	UINT8 offset;   /* Offset in memmap (EC_MEMMAP_*) */
	UINT8 size;     /* Size to read in bytes */
};

#include <poppack.h>

/* Read versions supported for a command */
#define EC_CMD_GET_CMD_VERSIONS 0x0008

/**
 * struct ec_params_get_cmd_versions - Parameters for the get command versions.
 * @cmd: Command to check.
 */
#include <pshpack1.h>
struct ec_params_get_cmd_versions {
	uint8_t cmd;
};
#include <poppack.h>

/**
 * struct ec_params_get_cmd_versions_v1 - Parameters for the get command
 *         versions (v1)
 * @cmd: Command to check.
 */
#include <pshpack2.h>
struct ec_params_get_cmd_versions_v1 {
	uint16_t cmd;
};
#include <poppack.h>

/**
 * struct ec_response_get_cmd_version - Response to the get command versions.
 * @version_mask: Mask of supported versions; use EC_VER_MASK() to compare with
 *                a desired version.
 */
#include <pshpack4.h>
struct ec_response_get_cmd_versions {
	uint32_t version_mask;
};
#include <poppack.h>


/* Get protocol information */
#define EC_CMD_GET_PROTOCOL_INFO	0x0b

/* Flags for ec_response_get_protocol_info.flags */
/* EC_RES_IN_PROGRESS may be returned if a command is slow */
#define EC_PROTOCOL_INFO_IN_PROGRESS_SUPPORTED (1 << 0)

#include <pshpack4.h>

struct ec_response_get_protocol_info {
	/* Fields which exist if at least protocol version 3 supported */

	/* Bitmask of protocol versions supported (1 << n means version n)*/
	UINT32 protocol_versions;

	/* Maximum request packet size, in bytes */
	UINT16 max_request_packet_size;

	/* Maximum response packet size, in bytes */
	UINT16 max_response_packet_size;

	/* Flags; see EC_PROTOCOL_INFO_* */
	UINT32 flags;
};

#include <poppack.h>

#endif  /* !__ACPI__ */
/*****************************************************************************/
/*
 * Passthru commands
 *
 * Some platforms have sub-processors chained to each other.  For example.
 *
 *     AP <--> EC <--> PD MCU
 *
 * The top 2 bits of the command number are used to indicate which device the
 * command is intended for.  Device 0 is always the device receiving the
 * command; other device mapping is board-specific.
 *
 * When a device receives a command to be passed to a sub-processor, it passes
 * it on with the device number set back to 0.  This allows the sub-processor
 * to remain blissfully unaware of whether the command originated on the next
 * device up the chain, or was passed through from the AP.
 *
 * In the above example, if the AP wants to send command 0x0002 to the PD MCU,
 *     AP sends command 0x4002 to the EC
 *     EC sends command 0x0002 to the PD MCU
 *     EC forwards PD MCU response back to the AP
 */

 /* Offset and max command number for sub-device n */
#define EC_CMD_PASSTHRU_OFFSET(n) (0x4000 * (n))
#define EC_CMD_PASSTHRU_MAX(n) (EC_CMD_PASSTHRU_OFFSET(n) + 0x3fff)

/*****************************************************************************/
/*
 * Deprecated constants. These constants have been renamed for clarity. The
 * meaning and size has not changed. Programs that use the old names should
 * switch to the new names soon, as the old names may not be carried forward
 * forever.
 */
#define EC_HOST_PARAM_SIZE      EC_PROTO2_MAX_PARAM_SIZE
#define EC_LPC_ADDR_OLD_PARAM   EC_HOST_CMD_REGION1
#define EC_OLD_PARAM_SIZE       EC_HOST_CMD_REGION_SIZE

 /*****************************************************************************/
 /* PWM commands */

 /* Get fan target RPM */
#define EC_CMD_PWM_GET_FAN_TARGET_RPM 0x20

#include <pshpack4.h>

struct ec_response_pwm_get_fan_rpm {
	UINT32 rpm;
};

#include <poppack.h>

/* Set target fan RPM */
#define EC_CMD_PWM_SET_FAN_TARGET_RPM 0x21

#include <pshpack4.h>

/* Version 0 of input params */
struct ec_params_pwm_set_fan_target_rpm_v0 {
	UINT32 rpm;
};

#include <poppack.h>

#include <pshpack1.h>

/* Version 1 of input params */
struct ec_params_pwm_set_fan_target_rpm_v1 {
	UINT32 rpm;
	UINT8 fan_idx;
};

/* Get keyboard backlight */
#define EC_CMD_PWM_GET_KEYBOARD_BACKLIGHT 0x22
struct ec_response_pwm_get_keyboard_backlight {
	UINT8 percent;
	UINT8 enabled;
};

/* Set keyboard backlight */
#define EC_CMD_PWM_SET_KEYBOARD_BACKLIGHT 0x23
struct ec_params_pwm_set_keyboard_backlight {
	UINT8 percent;
};

#include <poppack.h>

/* Set target fan PWM duty cycle */
#define EC_CMD_PWM_SET_FAN_DUTY 0x24

#include <pshpack4.h>

/* Version 0 of input params */
struct ec_params_pwm_set_fan_duty_v0 {
	UINT32 percent;
};

/* Version 1 of input params */
struct ec_params_pwm_set_fan_duty_v1 {
	UINT32 percent;
	UINT8 fan_idx;
};

#include <poppack.h>

/*****************************************************************************/
/* List the features supported by the firmware */
#define EC_CMD_GET_FEATURES  0x000D

/* Supported features */
enum ec_feature_code {
	/*
	* This image contains a limited set of features. Another image
	* in RW partition may support more features.
	*/
	EC_FEATURE_LIMITED = 0,
	/*
	* Commands for probing/reading/writing/erasing the flash in the
	* EC are present.
	*/
	EC_FEATURE_FLASH = 1,
	/*
	* Can control the fan speed directly.
	*/
	EC_FEATURE_PWM_FAN = 2,
	/*
	* Can control the intensity of the keyboard backlight.
	*/
	EC_FEATURE_PWM_KEYB = 3,
	/*
	* Support Google lightbar, introduced on Pixel.
	*/
	EC_FEATURE_LIGHTBAR = 4,
	/* Control of LEDs  */
	EC_FEATURE_LED = 5,
	/* Exposes an interface to control gyro and sensors.
	* The host goes through the EC to access these sensors.
	* In addition, the EC may provide composite sensors, like lid angle.
	*/
	EC_FEATURE_MOTION_SENSE = 6,
	/* The keyboard is controlled by the EC */
	EC_FEATURE_KEYB = 7,
	/* The AP can use part of the EC flash as persistent storage. */
	EC_FEATURE_PSTORE = 8,
	/* The EC monitors BIOS port 80h, and can return POST codes. */
	EC_FEATURE_PORT80 = 9,
	/*
	* Thermal management: include TMP specific commands.
	* Higher level than direct fan control.
	*/
	EC_FEATURE_THERMAL = 10,
	/* Can switch the screen backlight on/off */
	EC_FEATURE_BKLIGHT_SWITCH = 11,
	/* Can switch the wifi module on/off */
	EC_FEATURE_WIFI_SWITCH = 12,
	/* Monitor host events, through for example SMI or SCI */
	EC_FEATURE_HOST_EVENTS = 13,
	/* The EC exposes GPIO commands to control/monitor connected devices. */
	EC_FEATURE_GPIO = 14,
	/* The EC can send i2c messages to downstream devices. */
	EC_FEATURE_I2C = 15,
	/* Command to control charger are included */
	EC_FEATURE_CHARGER = 16,
	/* Simple battery support. */
	EC_FEATURE_BATTERY = 17,
	/*
	* Support Smart battery protocol
	* (Common Smart Battery System Interface Specification)
	*/
	EC_FEATURE_SMART_BATTERY = 18,
	/* EC can detect when the host hangs. */
	EC_FEATURE_HANG_DETECT = 19,
	/* Report power information, for pit only */
	EC_FEATURE_PMU = 20,
	/* Another Cros EC device is present downstream of this one */
	EC_FEATURE_SUB_MCU = 21,
	/* Support USB Power delivery (PD) commands */
	EC_FEATURE_USB_PD = 22,
	/* Control USB multiplexer, for audio through USB port for instance. */
	EC_FEATURE_USB_MUX = 23,
	/* Motion Sensor code has an internal software FIFO */
	EC_FEATURE_MOTION_SENSE_FIFO = 24,
	/* Support temporary secure vstore */
	EC_FEATURE_VSTORE = 25,
	/* EC decides on USB-C SS mux state, muxes configured by host */
	EC_FEATURE_USBC_SS_MUX_VIRTUAL = 26,
	/* EC has RTC feature that can be controlled by host commands */
	EC_FEATURE_RTC = 27,
	/* The MCU exposes a Fingerprint sensor */
	EC_FEATURE_FINGERPRINT = 28,
	/* The MCU exposes a Touchpad */
	EC_FEATURE_TOUCHPAD = 29,
	/* The MCU has RWSIG task enabled */
	EC_FEATURE_RWSIG = 30,
	/* EC has device events support */
	EC_FEATURE_DEVICE_EVENT = 31,
	/* EC supports the unified wake masks for LPC/eSPI systems */
	EC_FEATURE_UNIFIED_WAKE_MASKS = 32,
	/* EC supports 64-bit host events */
	EC_FEATURE_HOST_EVENT64 = 33,
	/* EC runs code in RAM (not in place, a.k.a. XIP) */
	EC_FEATURE_EXEC_IN_RAM = 34,
	/* EC supports CEC commands */
	EC_FEATURE_CEC = 35,
	/* EC supports tight sensor timestamping. */
	EC_FEATURE_MOTION_SENSE_TIGHT_TIMESTAMPS = 36,
	/*
	* EC supports tablet mode detection aligned to Chrome and allows
	* setting of threshold by host command using
	* MOTIONSENSE_CMD_TABLET_MODE_LID_ANGLE.
	*/
	EC_FEATURE_REFINED_TABLET_MODE_HYSTERESIS = 37,
	/* The MCU is a System Companion Processor (SCP). */
	EC_FEATURE_SCP = 39,
	/* The MCU is an Integrated Sensor Hub */
	EC_FEATURE_ISH = 40,
	/* New TCPMv2 TYPEC_ prefaced commands supported */
	EC_FEATURE_TYPEC_CMD = 41,
	/*
	* The EC will wait for direction from the AP to enter Type-C alternate
	* modes or USB4.
	*/
	EC_FEATURE_TYPEC_REQUIRE_AP_MODE_ENTRY = 42,
	/*
	* The EC will wait for an acknowledge from the AP after setting the
	* mux.
	*/
	EC_FEATURE_TYPEC_MUX_REQUIRE_AP_ACK = 43
};

#define BIT(nr) (1UL << (nr))

#define EC_FEATURE_MASK_0(event_code) BIT(event_code % 32)
#define EC_FEATURE_MASK_1(event_code) BIT(event_code - 32)

#include <pshpack4.h>

struct ec_response_get_features {
	UINT32 flags[2];
};

#include <poppack.h>

/*****************************************************************************/
/* Fingerprint MCU commands: range 0x0400-0x040x */

/* Fingerprint SPI sensor passthru command: prototyping ONLY */
#define EC_CMD_FP_PASSTHRU 0x0400

#define EC_FP_FLAG_NOT_COMPLETE 0x1

#include <pshpack2.h>

struct ec_params_fp_passthru {
	uint16_t len;		/* Number of bytes to write then read */
	uint16_t flags;		/* EC_FP_FLAG_xxx */
	uint8_t data[];		/* Data to send */
};

#include <poppack.h>

/* Configure the Fingerprint MCU behavior */
#define EC_CMD_FP_MODE 0x0402

/* Put the sensor in its lowest power mode */
#define FP_MODE_DEEPSLEEP      BIT(0)
/* Wait to see a finger on the sensor */
#define FP_MODE_FINGER_DOWN    BIT(1)
/* Poll until the finger has left the sensor */
#define FP_MODE_FINGER_UP      BIT(2)
/* Capture the current finger image */
#define FP_MODE_CAPTURE        BIT(3)
/* Finger enrollment session on-going */
#define FP_MODE_ENROLL_SESSION BIT(4)
/* Enroll the current finger image */
#define FP_MODE_ENROLL_IMAGE   BIT(5)
/* Try to match the current finger image */
#define FP_MODE_MATCH          BIT(6)
/* Reset and re-initialize the sensor. */
#define FP_MODE_RESET_SENSOR   BIT(7)
/* Sensor maintenance for dead pixels. */
#define FP_MODE_SENSOR_MAINTENANCE BIT(8)
/* special value: don't change anything just read back current mode */
#define FP_MODE_DONT_CHANGE    BIT(31)

#define FP_VALID_MODES (FP_MODE_DEEPSLEEP      | \
			FP_MODE_FINGER_DOWN    | \
			FP_MODE_FINGER_UP      | \
			FP_MODE_CAPTURE        | \
			FP_MODE_ENROLL_SESSION | \
			FP_MODE_ENROLL_IMAGE   | \
			FP_MODE_MATCH          | \
			FP_MODE_RESET_SENSOR   | \
			FP_MODE_SENSOR_MAINTENANCE | \
			FP_MODE_DONT_CHANGE)

/* Capture types defined in bits [30..28] */
#define FP_MODE_CAPTURE_TYPE_SHIFT 28
#define FP_MODE_CAPTURE_TYPE_MASK  (0x7 << FP_MODE_CAPTURE_TYPE_SHIFT)
/*
 * This enum must remain ordered, if you add new values you must ensure that
 * FP_CAPTURE_TYPE_MAX is still the last one.
 */
enum fp_capture_type {
	/* Full blown vendor-defined capture (produces 'frame_size' bytes) */
	FP_CAPTURE_VENDOR_FORMAT = 0,
	/* Simple raw image capture (produces width x height x bpp bits) */
	FP_CAPTURE_SIMPLE_IMAGE = 1,
	/* Self test pattern (e.g. checkerboard) */
	FP_CAPTURE_PATTERN0 = 2,
	/* Self test pattern (e.g. inverted checkerboard) */
	FP_CAPTURE_PATTERN1 = 3,
	/* Capture for Quality test with fixed contrast */
	FP_CAPTURE_QUALITY_TEST = 4,
	/* Capture for pixel reset value test */
	FP_CAPTURE_RESET_TEST = 5,
	FP_CAPTURE_TYPE_MAX,
};
/* Extracts the capture type from the sensor 'mode' word */
#define FP_CAPTURE_TYPE(mode) (((mode) & FP_MODE_CAPTURE_TYPE_MASK) \
				       >> FP_MODE_CAPTURE_TYPE_SHIFT)

#include <pshpack4.h>

struct ec_params_fp_mode {
	uint32_t mode; /* as defined by FP_MODE_ constants */
};

struct ec_response_fp_mode {
	uint32_t mode; /* as defined by FP_MODE_ constants */
};

#include <poppack.h>

/* Retrieve Fingerprint sensor information */
#define EC_CMD_FP_INFO 0x0403

/* Number of dead pixels detected on the last maintenance */
#define FP_ERROR_DEAD_PIXELS(errors) ((errors) & 0x3FF)
/* Unknown number of dead pixels detected on the last maintenance */
#define FP_ERROR_DEAD_PIXELS_UNKNOWN (0x3FF)
/* No interrupt from the sensor */
#define FP_ERROR_NO_IRQ    BIT(12)
/* SPI communication error */
#define FP_ERROR_SPI_COMM  BIT(13)
/* Invalid sensor Hardware ID */
#define FP_ERROR_BAD_HWID  BIT(14)
/* Sensor initialization failed */
#define FP_ERROR_INIT_FAIL BIT(15)

#include <pshpack4.h>

struct ec_response_fp_info_v0 {
	/* Sensor identification */
	uint32_t vendor_id;
	uint32_t product_id;
	uint32_t model_id;
	uint32_t version;
	/* Image frame characteristics */
	uint32_t frame_size;
	uint32_t pixel_format; /* using V4L2_PIX_FMT_ */
	uint16_t width;
	uint16_t height;
	uint16_t bpp;
	uint16_t errors; /* see FP_ERROR_ flags above */
};

struct ec_response_fp_info {
	/* Sensor identification */
	uint32_t vendor_id;
	uint32_t product_id;
	uint32_t model_id;
	uint32_t version;
	/* Image frame characteristics */
	uint32_t frame_size;
	uint32_t pixel_format; /* using V4L2_PIX_FMT_ */
	uint16_t width;
	uint16_t height;
	uint16_t bpp;
	uint16_t errors; /* see FP_ERROR_ flags above */
	/* Template/finger current information */
	uint32_t template_size;  /* max template size in bytes */
	uint16_t template_max;   /* maximum number of fingers/templates */
	uint16_t template_valid; /* number of valid fingers/templates */
	uint32_t template_dirty; /* bitmap of templates with MCU side changes */
	uint32_t template_version; /* version of the template format */
};

#include <poppack.h>

/* Get the last captured finger frame or a template content */
#define EC_CMD_FP_FRAME 0x0404

/* constants defining the 'offset' field which also contains the frame index */
#define FP_FRAME_INDEX_SHIFT       28
/* Frame buffer where the captured image is stored */
#define FP_FRAME_INDEX_RAW_IMAGE    0
/* First frame buffer holding a template */
#define FP_FRAME_INDEX_TEMPLATE     1
#define FP_FRAME_GET_BUFFER_INDEX(offset) ((offset) >> FP_FRAME_INDEX_SHIFT)
#define FP_FRAME_OFFSET_MASK       0x0FFFFFFF

/* Version of the format of the encrypted templates. */
#define FP_TEMPLATE_FORMAT_VERSION 4

/* Constants for encryption parameters */
#define FP_CONTEXT_NONCE_BYTES 12
#define FP_CONTEXT_USERID_WORDS (32 / sizeof(uint32_t))
#define FP_CONTEXT_TAG_BYTES 16
#define FP_CONTEXT_ENCRYPTION_SALT_BYTES 16
#define FP_CONTEXT_TPM_BYTES 32

/* Constants for positive match parameters. */
#define FP_POSITIVE_MATCH_SALT_BYTES 16

struct ec_fp_template_encryption_metadata {
	/*
	 * Version of the structure format (N=3).
	 */
	uint16_t struct_version;
	/* Reserved bytes, set to 0. */
	uint16_t reserved;
	/*
	 * The salt is *only* ever used for key derivation. The nonce is unique,
	 * a different one is used for every message.
	 */
	uint8_t nonce[FP_CONTEXT_NONCE_BYTES];
	uint8_t encryption_salt[FP_CONTEXT_ENCRYPTION_SALT_BYTES];
	uint8_t tag[FP_CONTEXT_TAG_BYTES];
};

#include <pshpack4.h>

struct ec_params_fp_frame {
	/*
	 * The offset contains the template index or FP_FRAME_INDEX_RAW_IMAGE
	 * in the high nibble, and the real offset within the frame in
	 * FP_FRAME_OFFSET_MASK.
	 */
	uint32_t offset;
	uint32_t size;
};

#include <poppack.h>

/* Load a template into the MCU */
#define EC_CMD_FP_TEMPLATE 0x0405

/* Flag in the 'size' field indicating that the full template has been sent */
#define FP_TEMPLATE_COMMIT 0x80000000

#include <pshpack4.h>

struct ec_params_fp_template {
	uint32_t offset;
	uint32_t size;
	uint8_t data[];
};

/* Clear the current fingerprint user context and set a new one */
#define EC_CMD_FP_CONTEXT 0x0406

struct ec_params_fp_context {
	uint32_t userid[FP_CONTEXT_USERID_WORDS];
};

enum fp_context_action {
	FP_CONTEXT_ASYNC = 0,
	FP_CONTEXT_GET_RESULT = 1,
};

/* Version 1 of the command is "asynchronous". */
struct ec_params_fp_context_v1 {
	uint8_t action;		/**< enum fp_context_action */
	uint8_t reserved[3];    /**< padding for alignment */
	uint32_t userid[FP_CONTEXT_USERID_WORDS];
};

#include <poppack.h>

#define EC_CMD_FP_STATS 0x0407

#define FPSTATS_CAPTURE_INV  BIT(0)
#define FPSTATS_MATCHING_INV BIT(1)

#include <pshpack2.h>

struct ec_response_fp_stats {
	uint32_t capture_time_us;
	uint32_t matching_time_us;
	uint32_t overall_time_us;
	struct {
		uint32_t lo;
		uint32_t hi;
	} overall_t0;
	uint8_t timestamps_invalid;
	int8_t template_matched;
};

#include <poppack.h>

#define EC_CMD_FP_SEED 0x0408

#include <pshpack4.h>
struct ec_params_fp_seed {
	/*
	 * Version of the structure format (N=3).
	 */
	uint16_t struct_version;
	/* Reserved bytes, set to 0. */
	uint16_t reserved;
	/* Seed from the TPM. */
	uint8_t seed[FP_CONTEXT_TPM_BYTES];
};

#define EC_CMD_FP_ENC_STATUS 0x0409

/* FP TPM seed has been set or not */
#define FP_ENC_STATUS_SEED_SET BIT(0)

struct ec_response_fp_encryption_status {
	/* Used bits in encryption engine status */
	uint32_t valid_flags;
	/* Encryption engine status */
	uint32_t status;
};

#define EC_CMD_FP_READ_MATCH_SECRET 0x040A
struct ec_params_fp_read_match_secret {
	uint16_t fgr;
};

/* The positive match secret has the length of the SHA256 digest. */
#define FP_POSITIVE_MATCH_SECRET_BYTES 32
struct ec_response_fp_read_match_secret {
	uint8_t positive_match_secret[FP_POSITIVE_MATCH_SECRET_BYTES];
};

#include <poppack.h>

#endif  /* __CROS_EC_COMMANDS_H */
