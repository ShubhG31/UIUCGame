/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    /*printk("packet : %x %x %x\n", a, b, c); */
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
		unsigned int display_value;
		int LED_location;
		int DECIMAL_location;
		int i;
    switch (cmd) {
	case TUX_INIT:
		MTCP_LED_USR;
	case TUX_BUTTONS:
		if(arg == NULL){
			return -EINVAL;
		}
		*((unsigned long *)arg) = *((unsigned long *)arg) & 0x00;
		return 0;

// Opcode MTCP_LED_SET
//	Set the User-set LED display values. These will be displayed on the
//	LED displays when the LED display is in USR mode (see the MTCP_LED_USR
//	and MTCP_LED_CLK commands). The first byte of argument specifies
//	which of the LED's to set, and also determines how many bytes will
//	follow - one byte for each led to set.
//
// 
// 	Mapping from 7-segment to bits
// 	The 7-segment display is:
//		  _A
//		F| |B
//		  -G
//		E| |C
//		  -D .dp
//
// 	The map from bits to segments is:
// 
// 	__7___6___5___4____3___2___1___0__
// 	| A | E | F | dp | G | C | B | D | 
// 	+---+---+---+----+---+---+---+---+
// 
// 	Arguments: >= 1 bytes
//		byte 0 - Bitmask of which LED's to set:

//		__7___6___5___4____3______2______1______0___
// 		| X | X | X | X | LED3 | LED2 | LED1 | LED0 | 
// 		----+---+---+---+------+------+------+------+
//
//	The number of bytes which should follow should be equal to the
//	number of bits set in byte 0. The bytes should be sent in order of 
//	increasing LED number. (e.g LED0, LED2, LED3 for a bitmask of 0x0D)
//
// 	Response: 1 byte
//		byte 0 - MTCP_ACK
//
// Opcode, led_on, led1_mask, led2_mask, led3_mask, led4_mask, 
	case TUX_SET_LED:

		// unsigned int display_value;
		// int LED_location;
		// int DECIMAL_location;
		// int i;
		
		display_value = (arg) & 0xFFFF;
		display_value = 0xF000;
		
		//
		LED_location = (int)arg >> 16;
		LED_location = LED_location & 0xF;


		DECIMAL_location = arg >> 24;
		DECIMAL_location = DECIMAL_location & 0xF;


		char buffer[6] = {0};

		buffer[0] = MTCP_LED_SET;

		buffer[1] = 0xF;//LED_location;

		// LED1 -> far right LED
		buffer[2]= 0xFF;
		buffer[3]= 0xFF;
		buffer[4]= 0xFF;
		buffer[5]= 0x0;
		// LED4 -> far left LED
		i=2;
		for(i = 0; i<4; i++){
			if(LED_location & 0x01<<i){
				buffer[2+i] = display_packet_mapping(display_value>>(i*4) & 0xf, DECIMAL_location>>i & 0x1);
			}
		}
		// if(LED_location & 0x01){
		// 	buffer[i] = display_packet_mapping(display_value & 0xf, (DECIMAL_location>>1 & 0x1));
		// 	i+=1;
		// }
		// if(LED_location & 0x01<<1){
		// 	buffer[i] = display_packet_mapping(display_value<<4 & 0xf, DECIMAL_location>>2 & 0x1);
		// 	i+=1;
		// }
		// if(LED_location & 0x01<<2){
		// 	buffer[i] = display_packet_mapping(display_value<<8 & 0xf, DECIMAL_location>>3 & 0x1);
		// 	i+=1;
		// }
		// if(LED_location & 0x01<<3){
		// 	buffer[i] = display_packet_mapping(display_value<<12 & 0xf, DECIMAL_location>>4 & 0x1);
		// 	i+=1;
		// }
		tuxctl_ldisc_put(tty, buffer, 6);

		return 0;
	case TUX_LED_ACK:
		return 0;
	case TUX_LED_REQUEST:
		return 0;
	case TUX_READ_LED:
		return 0;
	default:
	    return -EINVAL;
    }
}

// 	Mapping from 7-segment to bits
// 	The 7-segment display is:
//		  _A
//		F| |B
//		  -G
//		E| |C
//		  -D .dp
//
// 	The map from bits to segments is:
// 
// 	__7___6___5___4____3___2___1___0__
// 	| A | E | F | dp | G | C | B | D | 
// 	+---+---+---+----+---+---+---+---+
// 
int display_packet_mapping(int value, int decimal_enable){
	int packet = 0;

	if(value == 0){
		packet = 0xE7; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 1){
		packet = 0x06; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 2){
		packet = 0xCB; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 3){
		packet = 0x8F; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 4){
		packet = 0x2E; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 5){
		packet = 0xAD; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 6){
		packet = 0xED; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 7){
		packet = 0x86; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 8){
		packet = 0xEF; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 9){
		packet = 0xAF; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 10){
		packet = 0xEE; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 11){
		packet = 0x6D; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 12){
		packet = 0xE0; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 13){
		packet = 0x4F; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 14){
		packet = 0xE9; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	if(value == 15){
		packet = 0xE8; 
		if(decimal_enable){
			packet = packet | 0x1f;
		}
	}
	return packet;
}