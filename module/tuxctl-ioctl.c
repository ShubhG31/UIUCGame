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

int display_packet_mapping(int value, int decimal_enable);
int ioctl_INIT_help(struct tty_struct* tty);
int ioctl_LED_help(struct tty_struct* tty, unsigned long arg);
int ioctl_Button_help(struct tty_struct* tty, unsigned long arg);

volatile static  int ack; 
static unsigned int button_data;
static unsigned int Old_LED; 
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
	if (a==MTCP_ACK) {
		ack = 1;
	}
	if(MTCP_RESET == a){
		// printk("RESET0000000000000000000000000000000000000000000");
		// initialize the TUX
		ioctl_INIT_help(tty);
		// set the LED to old_LED value
		ioctl_LED_help(tty,Old_LED);
		
	}
/**
; MTCP_BIOC_EVT	
;	Generated when the Button Interrupt-on-change mode is enabled and 
;	a button is either pressed or released.
;
; 	Packet format:
;		Byte 0 - MTCP_BIOC_EVENT
;		byte 1  +-7-----4-+-3-+-2-+-1-+---0---+
;			| 1 X X X | C | B | A | START |
;			+---------+---+---+---+-------+
;		byte 2  +-7-----4-+---3---+--2---+--1---+-0--+
;			| 1 X X X | right | down | left | up |
;			+---------+-------+------+------+----+
			*******the left and down switch is not shown above*******
**/
	if(a == MTCP_BIOC_EVENT){
		int left = (c >> 1) & 0x1;
		int down = (c >> 2) & 0x1;
		int up = c & 0x1;
		int right = (c >> 3) & 0x1; 
		c = ((right<<3)| (left<<2) | (down << 1)  | up);
		button_data = (c & 0xf) << 4 | (b & 0xf);
		// button_data 
		// printk("%x\n",button_data);
	}
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
    switch (cmd) {
	
	case TUX_INIT:
		ioctl_INIT_help(tty);
		return 0;
	
	case TUX_BUTTONS:
		ioctl_Button_help(tty, arg);
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
		ioctl_LED_help(tty, arg);
		return 0;
	case TUX_LED_ACK:
	case TUX_LED_REQUEST:
	case TUX_READ_LED:
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
/* 
 * display_packet_mapping 
 *   DESCRIPTION: Reads the hex value from agrument to convert to the 7 segment data for the TUX to disipher
 * 				  
 *                
 *   INPUTS: value -- value needed to be converted, decimal_enable -- need to enable decimal point or not 
 *   OUTPUTS: none
 *   RETURN VALUE: value of the packet of the 7 segment data
 *   SIDE EFFECTS: none
 */
int display_packet_mapping(int value, int decimal_enable){
	// packet is set to 0 
	int packet = 0;
	// get the hex value of the value 
	value = value & 0xF;
	// if value == 0, datasegment is 0xE7 which is 0 
	if(value == 0){
		packet = 0xE7; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 1, datasegment is 0x06 which is 1
	else if(value == 1){
		packet = 0x06; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 2, datasegment is 0xCB which is 2
	else if(value == 2){
		packet = 0xCB; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 3, datasegment is 0x8F which is 3
	else if(value == 3){
		packet = 0x8F; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 4, datasegment is 0x2E which is 4
	else if(value == 4){
		packet = 0x2E; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 5, datasegment is 0xAD which is 5
	else if(value == 5){
		packet = 0xAD; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 6, datasegment is 0xED which is 6
	else if(value == 6){
		packet = 0xED; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 7, datasegment is 0x86 which is 7
	else if(value == 7){
		packet = 0x86; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 8, datasegment is 0xEF which is 8
	else if(value == 8){
		packet = 0xEF; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 9, datasegment is 0xAF which is 9
	else if(value == 9){
		packet = 0xAF; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 10, datasegment is 0xEE which is A
	else if(value == 10){
		packet = 0xEE; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 11, datasegment is 0x6D which is B
	else if(value == 11){
		packet = 0x6D; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 12, datasegment is 0xE1 which is C
	else if(value == 12){
		packet = 0xE1; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 13, datasegment is 0x4F which is D
	else if(value == 13){
		packet = 0x4F; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 14, datasegment is 0xE9 which is E
	else if(value == 14){
		packet = 0xE9; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	// if value == 15, datasegment is 0xE8 which is F
	else if(value == 15){
		packet = 0xE8; 
		if(decimal_enable){
			// enable the dp bit
			packet = packet | 0x10;
		}
	}
	return packet;
}
/* 
 * ioctl_INIT_help 
 *   DESCRIPTION: Initalizes the TUX 
 * 				  
 *                
 *   INPUTS: tty -- pointer to send packets to the TUX
 *   OUTPUTS: none
 *   RETURN VALUE: returns int to tell if something was successful or not 
 *   SIDE EFFECTS: none
 */
int ioctl_INIT_help(struct tty_struct* tty){
				char buf[2];
				ack = 1;
				buf[0] = MTCP_BIOC_ON;
				buf[1] = MTCP_LED_USR;
				tuxctl_ldisc_put(tty, buf, 2);
				return 0;
		  }
/* 
 * ioctl_LED_help 
 *   DESCRIPTION: sets the LED values on the TUX
 * 				  
 *                
 *   INPUTS: arg -- argument to send to TUX to display values, tty -- tux access pointer to help write to tux
 *   OUTPUTS: success or fail of writing to the TUX LED
 *   RETURN VALUE: value of the packet of the 7 segment data
 *   SIDE EFFECTS: none
 */
int ioctl_LED_help(struct tty_struct* tty, unsigned long arg){
	// 16 bit values of the LEDs
	uint16_t display_value;
	// LED locations that will be on 
	int LED_location;
	// Decimal location that will be on 
	int DECIMAL_location;
	// for loop counter intialized
	int i;
	// buffer for ioctl call initialized to 0 
	char buffer[6] = {0};
	
	// if acknowledgment is 0 then TUX failed earlier so return 0 
	if(ack == 0){
		return 0 ;
	}
	// store old_LED with current LED argument 
	Old_LED = arg; 
	// display value stores the 16 bits of the arguement
	display_value = (arg) & 0xFFFF;

	// LED_location is get the top 16 bits
	LED_location = (int)arg >> 16;
	// get the argument[16:20]
	LED_location = LED_location & 0xF;

	// decimal location determined in the top 8 bits 
	DECIMAL_location = arg >> 24;
	// get the 4 bits of the decimal location
	DECIMAL_location = DECIMAL_location & 0xF;

	// buffer is loaded with the MTCP_LED_SET to tell TUX microprocessor what command is being used 
	buffer[0] = MTCP_LED_SET;

	// hard set the LED_Location with all on 
	buffer[1] = 0xf;//LED_location;

	// DECIMAL_location = 0xE;
	// LED_location = 0xb;
	// buffer[1] = LED_location;
	// LED1 -> far right LED
	// buffer[2]= 1;
	// buffer[3]= 0;
	// buffer[4]= 0;
	// buffer[5] = 0;
	// buffer[2]= display_packet_mapping(0x7, 1);//hex_segments[1] | (0x10 ) ;
	// LED4 -> far left LED
	// printk("%x, %x, %x\n",DECIMAL_location, LED_location, display_value);
	
	// for loop to go though the display value and load the buffer 
	for(i = 0; i<4; i++){
		// check is the led_location is suppose to be on or not 
		if(LED_location & (0x01<<i)){
			// set buffer to the 7 segment data 
			buffer[2+i] = display_packet_mapping(display_value>>(i*4) & 0xf, DECIMAL_location>>i & 0x1);
		}
		// since LED_location is not enabled set LED off 
		else{
			// set buffer to be 0 to turn off LED
			buffer[2+i] = 0;
		}
		// printk("%x\n",buffer[i+2]);
	}
	// set Awknowledge bit to 0 since put call will be made
	ack = 0;
	// make put call to set LEDS on buffer
	tuxctl_ldisc_put(tty, buffer, 6);

	return 0;
}
/* 
 * ioctl_Button_help  
 *   DESCRIPTION: Reads the value of the button and set the argument to the button pointer given
 * 				  
 *                
 *   INPUTS: arg -- argument to write to TUX button value, tty -- tux access pointer to help read to tux 
 *   OUTPUTS: none 
 *   RETURN VALUE: integer value to tell if success or failure
 *   SIDE EFFECTS: none
 */
int ioctl_Button_help(struct tty_struct* tty, unsigned long arg){
	// if pointer is 0, then return -EINVAL
	if(arg == 0){
			return -EINVAL;
		}
		// *((unsigned long *)arg) = *((unsigned long *)arg) & button_data;
		// printk('%u\n',button_data);
		// printk("%x\n",button_data);

		// send button packet to the user level 
		copy_to_user((unsigned int *)arg, &button_data, 2);
		return 0;
}
