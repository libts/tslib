/*
 * Copyright (C) 2008-2009 Palm, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __CY8MRLN_H
#define __CY8MRLN_H

#ifdef __KERNEL__
#include <linux/spi/spi.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#endif /* __KERNEL */


/* IOCTLs */
#define CY8MRLN_IOCTL_GET_SCANRATE	_IOR('c', 0x01, int)
#define CY8MRLN_IOCTL_GET_SLEEPMODE	_IOR('c', 0x02, int)
#define CY8MRLN_IOCTL_GET_XFER_OPTION	_IOR('c', 0x03, int)
#define CY8MRLN_IOCTL_GET_SPI_MODE	_IOR('c', 0x04, int)
#define CY8MRLN_IOCTL_GET_SPI_CLOCK	_IOR('c', 0x05, int)
#define CY8MRLN_IOCTL_GET_READ_OPTION	_IOR('c', 0x06, int)
#define CY8MRLN_IOCTL_GET_VERBOSE_MODE	_IOR('c', 0x07, int)

#define CY8MRLN_IOCTL_SET_SCANRATE	_IOW('c', 0x08, int)
#define CY8MRLN_IOCTL_SET_SLEEPMODE	_IOW('c', 0x09, int)
#define CY8MRLN_IOCTL_SET_XFER_OPTION	_IOW('c', 0x0a, int)
#define CY8MRLN_IOCTL_SET_SPI_MODE	_IOW('c', 0x0b, char)
#define CY8MRLN_IOCTL_SET_SPI_CLOCK	_IOW('c', 0x0c, int)
#define CY8MRLN_IOCTL_SET_READ_OPTION	_IOW('c', 0x0d, int)
#define CY8MRLN_IOCTL_SET_VERBOSE_MODE	_IOW('c', 0x0e, int)
#define CY8MRLN_IOCTL_SET_BYTES		_IOW('c', 0x0f, int)
#define CY8MRLN_IOCTL_GET_NUM_DATA_BYTES	_IOR('c', 0x10, int)
#define CY8MRLN_IOCTL_SET_NUM_DATA_BYTES	_IOW('c', 0x11, int)
#define CY8MRLN_IOCTL_GET_QUERY_DATA	_IOR('c', 0x12, unsigned short)
#define CY8MRLN_IOCTL_SET_VECTORS	_IOW('c', 0x13, int)
#define CY8MRLN_IOCTL_SET_PROG_PHASE	_IOW('c', 0x14, int)
#define CY8MRLN_IOCTL_SET_FW_DATA	_IOW('c', 0x15, int)
#define CY8MRLN_IOCTL_GET_TIMESTAMP_MODE _IOR('c', 0x16, int)
#define CY8MRLN_IOCTL_SET_TIMESTAMP_MODE _IOW('c', 0x17, int)
/* rpswtest */
// The following two ioctls are deprecated
// #define CY8MRLN_IOCTL_GET_CALIBRATION_MODE      _IOR('c', 0x18, int)
// #define CY8MRLN_IOCTL_SET_CALIBRATION_MODE      _IOW('c', 0x19, int)
#define CY8MRLN_IOCTL_GET_INTERLEAVED_SCAN_MODE _IOR('c', 0x1a, int)
#define CY8MRLN_IOCTL_SET_INTERLEAVED_SCAN_MODE _IOW('c', 0x1b, int)

/* Wake-On-Touch or WOT */
//#define CY8MRLN_IOCTL_SET_WOT_MODE		_IOW('c', 0x1c, int)
#define CY8MRLN_IOCTL_SET_WOT_THRESHOLD		_IOW('c', 0x1d, int)
#define CY8MRLN_IOCTL_GET_WOT_THRESHOLD		_IOR('c', 0x1e, int)
//#define CY8MRLN_IOCTL_SET_WOT_PRESCALER	_IOW('c', 0x1f, int)
//#define CY8MRLN_IOCTL_GET_WOT_PRESCALER	_IOR('c', 0x20, int)

/*
 * The WOT SCAN Rate the rate at which the PSoC will
 * wake up from sleep and perform a WOT scan
 */
#define CY8MRLN_IOCTL_GET_IDAC_TABLE		_IOR('c', 0x21, unsigned char[154])
#define CY8MRLN_IOCTL_SET_WOT_SCANRATE		_IOW('c', 0x22, int)
#define CY8MRLN_IOCTL_GET_WOT_SCANRATE		_IOR('c', 0x23, int)
#define CY8MRLN_IOCTL_START_WOT_SCAN		_IOW('c', 0x24, int)
#define CY8MRLN_IOCTL_GET_WOT_BASELINE_LO	_IOR('c', 0x25, int)
#define CY8MRLN_IOCTL_GET_WOT_BASELINE_HI	_IOR('c', 0x26, int)
#define CY8MRLN_IOCTL_SET_WOT_BASELINE_LO	_IOW('c', 0x27, int)
#define CY8MRLN_IOCTL_SET_WOT_BASELINE_HI	_IOW('c', 0x28, int)
#define CY8MRLN_IOCTL_GET_FPC_INFO			_IOR('c', 0x29, unsigned char)
#define CY8MRLN_IOCTL_ENTER_WOT_RAW_DATA	_IOR('c', 0x30, int)
#define CY8MRLN_IOCTL_EXIT_WOT_RAW_DATA		_IOR('c', 0x31, int)


/*CY8MRLN_IOCTL_START_WOT_SCAN_FORCED:
 * Combines CY8MRLN_IOCTL_START_WOT_SCAN and CY8MRLN_IOCTL_SET_WOT_THRESHOLD
 * in one IOCTLs.  I.E. the wot threshold is sent together with wot start request.
 * This IOCTLE will ejects/force PSoC out of WOT mode so that the WOT sensitivity
 * specified in the IOCTL parameter will be used.
 */
#define CY8MRLN_IOCTL_START_WOT_SCAN_FORCED	_IOW('c', 0x32, int)

/*
*  Used for forcing calibration of the IDAC outside of flashing and also storing that calibration
*/
#define CY8MRLN_IOCTL_IDAC_CALIBRATE			_IOW('c', 0x33, int)
#define CY8MRLN_IOCTL_IDAC_CALIBRATION_STORE	_IOW('c', 0x34, int)

/*
*   Causes the PSoC to calibrate its internal low speed oscillator
*/
#define CY8MRLN_IOCTL_ILO_CALIBRATE		_IOW('c', 0x35, int)
#define CY8MRLN_IOCTL_EXIT_WOT_SCAN		_IOR('c', 0x36, int)

/* WOT Sensitivity/Threshold ranges */
#define WOT_THRESHOLD_MIN 10
#define WOT_THRESHOLD_MAX 73

/* WOT Scan Rate Index */
enum{
	WOT_SCANRATE_512HZ = 0,
	WOT_SCANRATE_256HZ,
	WOT_SCANRATE_171HZ,
	WOT_SCANRATE_128HZ
};

/* WOT MIN & MAX configuration value for scan rate
 * NB: min. value is max freq and
 *     max. value is min freq.
 */
#define WOT_SCANRATE_MAX	WOT_SCANRATE_128HZ
#define WOT_SCANRATE_MIN	WOT_SCANRATE_512HZ

/* debug */


/* PSoC Power State */
enum {
	CY8MRLN_OFF_STATE = 0,
	CY8MRLN_SLEEP_STATE,
	CY8MRLN_ON_STATE
};

/* Scan/Version SPI data xfer method. */
enum {
	CY8MRLN_TWO_INT_THREE_SETUP_BYTES = 0,
	CY8MRLN_TWO_INT_TWO_SETUP_BYTES,
	CY8MRLN_ONE_INT_THREE_SETUP_BYTES,
	CY8MRLN_ONE_INT_TWO_SETUP_BYTES,
	CY8MRLN_ONE_INT_ZERO_SETUP_BYTES
};

/* DMA vs. PIO. */
enum {
	CY8MRLN_PIO_XFER_OPTION = 0,
	CY8MRLN_DMA_XFER_OPTION
};

enum {
	SCAN_AT_256HZ = 0,
	SCAN_AT_170HZ,
	SCAN_AT_128HZ,
	SCAN_AT_102HZ
};

#define CY8MRLN_DEFALUT_NUM_DATA_BYTES		(7 * 11 * 2 + 4)

/* PSoC Programming Vector Structure. */
typedef struct cy8mrln_vector {
	unsigned int id;
	unsigned int bits;
	unsigned int bytes;
	unsigned char *data;
} cy8mrln_vector_t;

/* PSoC Hex FW file line structure. */
typedef struct cy8mrln_fw_data {
	unsigned int index;
	unsigned int bytes;
	unsigned int addr;
	unsigned int type;
	unsigned int checksum;
	unsigned char *blk;
} cy8mrln_fw_data_t;

struct cy8mrln_platform_data {
	int xres_gpio;		/* not used. */
	int enable_gpio;
	int scl_gpio;		/* Programming mode clk */
	int mosi_gpio;		/* Used for SDA line mirroring in programming */
	int sda_gpio;		/* Programming mode data */
	int flip_x;		/* Flip the X coordinate? */
	int flip_y;		/* Flip the Y coordinate? */
	/* Function pointer that switches between the programming
	 * mode and the normal functional mode.
	 */
	int (*switch_mode)(int flag);
};

/* SPI stuff. */
#define SPI_READ_BIT			0x80
#define CY8MRLN_DATA_REG_START		0x00
#define CY8MRLN_CONTROL_REG		0x00

/* Command definitions. */
#define CY8MRLN_WAKE_UP				0x00
#define CY8MRLN_GO_ACTIVE			0x00
#define CY8MRLN_SCAN_START			0x01
#define CY8MRLN_SLEEP				0x02
#define CY8MRLN_QUERY				0x04
#define CY8MRLN_STORE_IDAC_CALIBRATION		0x08
#define CY8MRLN_CALIBRATE_IDAC			0x0A
#define CY8MRLN_AUTO_SCAN			0x0B
#define CY8MRLN_CALIBRATE_ILO			0x0C
#define CY8MRLN_BASELINE_ADJUST			0x0D
#define CY8MRLN_WOT_RAW_DATA			0x0F
#define CY8MRLN_CMD_INVALID			0xFF


enum {
		WOT_DBG_OFF = 0,	//0 OFF
		WOT_DBG_ON_PENDING,	//1 ON_P
		WOT_DBG_ON,		//2 ON
		WOT_DBG_OFF_PENDING 	//3 OFF_P
};

/* 11_0A is the latest version. The 08_0c is the initial
 * version of FW that actually doesn't support QUERY
 * command.
 */
#define VERSION_11_0A                   0x110a
#define VERSION_08_0C                   0x080c

/* PSoC Programming Vectors. These vectors are defined
 * by Cypress and reside in user space updater application.
 */
enum {
	INIT1 = 0,
	INIT2,
	INIT3_5V,
	INIT3_3V,
	ID_SETUP,
	TARGET_ID,
	READ_ID,
	ERASE_ALL,
	SET_BANK_NUMBER,
	SET_BANK_NUMBER_END,
	WRITE_BYTE_START,
	WRITE_BYTE_END,
	SET_BLOCK_NUMBER,
	SET_BLOCK_NUMBER_END,
	PROGRAM_BLOCK,
	VERIFY_SETUP,
	READ_BYTE,
	SECURITY,
	CHECKSUM,
	READ_CHECKSUM,
	WAIT_AND_POLL_END,
	NUM_VECTORS
};


/* Different phases of programming the PSoC. */
enum {
	NORM_OP = 0,
	INIT_VECTORS,
	INIT_TARGET,
	VERIFY_ID,
	PROGRAM,
	VERIFY,
	SECURE,
	VERIFY_CHECKSUM,
	EXIT_PROG
};

/* Different types of Cypress Hex FW data. */
enum {
	FW_START = 0,
	FW_END = 255,
	SECURITY_OFFSET = 256,
	SECURITY_DATA = 257,
	CHECKSUM_OFFSET = 258,
	CHECKSUM_DATA = 259,
	END_OF_FILE = 260,
	NUM_FW_LINES = 261
};

#define NUM_TP_DRV_STATUS_REG_BYTES 1
#define DRV_STATUS_MASK__BUF_OVERFLOW 		0x80

#define TP_DRV_STATUS_REG__FRAME_TYPE_MASK	0x60
#define FRAME_TYPE_MASK__NORMAL_SCAN		0x00
#define FRAME_TYPE_MASK__WOT_ENTRY_SCAN 	0x20
#define FRAME_TYPE_MASK__WOT_EXIT_SCAN 		0x40

/* Cypress PSoC Attributes. */

// ****************************************************************************
// ************* USER ATTENTION REQUIRED: TARGET SUPPLY VOLTAGE ***************
// ****************************************************************************
// This directive causes the proper Initialization vector #3 to be sent
// to the Target, based on what the Target Vdd programming voltage will
// be. Either 5V (if #define enabled) or 3.3V (if #define disabled).
//#define TARGET_VOLTAGE_IS_5V


// ****************************************************************************
// **************** USER ATTENTION REQUIRED: PROGRAMMING MODE *****************
// ****************************************************************************
// This directive selects whether code that uses reset programming mode or code
// that uses power cycle programming is use. Reset programming mode uses the
// external reset pin (XRES) to enter programming mode. Power cycle programming
// mode uses the power-on reset to enter programming mode.
// Applying signals to various pins on the target device must be done in a
// deliberate order when using power cycle mode. Otherwise, high signals to GPIO
// pins on the target will power the PSoC through the protection diodes.
//#define RESET_MODE

// ****************************************************************************
// ****************** USER ATTENTION REQUIRED: TARGET PSOC ********************
// ****************************************************************************
// The directives below enable support for various PSoC devices. The root part
// number to be programmed should be un-commented so that its value becomes
// defined.  All other devices should be commented out.
// Select one device to be supported below:

// **** CY8C21x23 devices ****
//#define CY8C21123
//#define CY8C21223
//#define CY8C21323
//#define CY8C21002

// **** CY8C21x34 devices ****
//#define CY8C21234
//#define CY8C21334
//#define CY8C21434
//#define CY8C21534
//#define CY8C21634
//#define CY8C21001

// **** CY8C24x23A devices ****
//#define CY8C24123A
//#define CY8C24223A
//#define CY8C24423A
//#define CY8C24000A

// **** CY8C24x94 devices ****
//#define CY8C24794
#define CY8C24894
//#define CY8C24994
//#define CY8C24094

// **** CY8C27x34 devices ****
//#define CY8C27143
//#define CY8C27243
//#define CY8C27443
//#define CY8C27543
//#define CY8C27643
//#define CY8C27002

// **** CY8C29x66 devices ****
//#define CY8C29466
//#define CY8C29566
//#define CY8C29666
//#define CY8C29866
//#define CY8C29002

//-----------------------------------------------------------------------------
// This section sets the Family that has been selected. These are used to
// simplify other conditional compilation blocks.
//-----------------------------------------------------------------------------
#ifdef CY8C21123
    #define CY8C21x23
#endif
#ifdef CY8C21223
    #define CY8C21x23
#endif
#ifdef CY8C21323
    #define CY8C21x23
#endif
#ifdef CY8C21002
    #define CY8C21x23
#endif
#ifdef CY8C21234
    #define CY8C21x34
#endif
#ifdef CY8C21334
    #define CY8C21x34
#endif
#ifdef CY8C21434
    #define CY8C21x34
#endif
#ifdef CY8C21534
    #define CY8C21x34
#endif
#ifdef CY8C21634
    #define CY8C21x34
#endif
#ifdef CY8C21001
    #define CY8C21x34
#endif
#ifdef CY8C24123A
    #define CY8C24x23A
#endif
#ifdef CY8C24223A
    #define CY8C24x23A
#endif
#ifdef CY8C24423A
    #define CY8C24x23A
#endif
#ifdef CY8C24000A
    #define CY8C24x23A
#endif
#ifdef CY8C24794
    #define CY8C24x94
#endif
#ifdef CY8C24894
    #define CY8C24x94
#endif
#ifdef CY8C24994
    #define CY8C24x94
#endif
#ifdef CY8C24094
    #define CY8C24x94
#endif
#ifdef CY8C27143
    #define CY8C27x43
#endif
#ifdef CY8C27243
    #define CY8C27x43
#endif
#ifdef CY8C27443
    #define CY8C27x43
#endif
#ifdef CY8C27543
    #define CY8C27x43
#endif
#ifdef CY8C27643
    #define CY8C27x43
#endif
#ifdef CY8C27002
    #define CY8C27x43
#endif
#ifdef CY8C29466
    #define CY8C29x66
#endif
#ifdef CY8C29566
    #define CY8C29x66
#endif
#ifdef CY8C29666
    #define CY8C29x66
#endif
#ifdef CY8C29866
    #define CY8C29x66
#endif
#ifdef CY8C29002
    #define CY8C29x66
#endif
//-----------------------------------------------------------------------------
// The directives below are used to define various sets of vectors that differ
// for more than one set of PSoC parts.
//-----------------------------------------------------------------------------
// **** Select a Checksum Setup Vector ****
#ifdef CY8C21x23
    #define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C21x34
    #define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C24x23A
    #define CHECKSUM_SETUP_24_24A
#endif
#ifdef CY8C24x94
    #define CHECKSUM_SETUP_24_29
#endif
#ifdef CY8C27x43
    #define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C29x66
    #define CHECKSUM_SETUP_24_29
#endif

// **** Select a Program Block Vector ****
#ifdef CY8C21x23
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C21x34
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C24x23A
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C24x94
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C27x43
    #define PROGRAM_BLOCK_27
#endif
#ifdef CY8C29x66
    #define PROGRAM_BLOCK_21_24_29
#endif

//-----------------------------------------------------------------------------
// The directives below are used to control switching banks if the device is
// has multiple banks of Flash.
//-----------------------------------------------------------------------------
// **** Select a Checksum Setup Vector ****
#ifdef CY8C24x94
    #define MULTI_BANK
#endif
#ifdef CY8C29x66
    #define MULTI_BANK
#endif

/* Block-Verify Uses 64-Bytes of RAM */
#define TARGET_DATABUFF_LEN    64

/* The number of Flash blocks in each part is defined here. This is used in
 * main programming loop when programming and verifying the blocks.
 */
#ifdef CY8C21x23
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK              64
    #define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C21x34
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK             128
    #define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C24x23A
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK              64
    #define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C24x94
    #define NUM_BANKS                     2
    #define BLOCKS_PER_BANK             128
    #define SECURITY_BYTES_PER_BANK      32
#endif
#ifdef CY8C27x43
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK             256
    #define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C29x66
    #define NUM_BANKS                     4
    #define BLOCKS_PER_BANK             128
    #define SECURITY_BYTES_PER_BANK      32
#endif


/* In system programming error code. */
#define PASS		(0)
#define HILOW_ERROR	(-1)
#define SI_ID_ERROR	(-2)
#define VERIFY_ERROR	(-3)
#define CHECKSUM_ERROR	(-4)


/* TRANSITION_TIMEOUT is a loop counter for a 100msec timeout when waiting for
 * a high-to-low transition. Each pass through the loop takes approximately 15
 * usec. 100 msec is about 6740 loops.
 */
#define TRANSITION_TIMEOUT    	6740

/* XRES_DELAY is the time duration for which XRES is asserted. This defines
 * a 63 usec delay.
 * The minimum Xres time (from the device datasheet) is 10 usec.
 */
#define XRES_CLK_DELAY   	63

/* POWER_CYCLE_DELAY is the time required when power is cycled to the target
 * device to create a power reset after programming has been completed. The
 * actual time of this delay will vary from system to system depending on the
 * bypass capacitor size.  A delay of 1000 usec is used here.
 */
#define POWER_CYCLE_DELAY	1000

#endif //__CY8MRLN_H
