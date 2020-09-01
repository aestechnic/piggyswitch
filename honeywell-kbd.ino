
// Teensy 2.0 piggyback controller for a Honeywell Microswitch keyboard
// Copyright (C) 2020  aestechnic
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// --------------------------------------------------------------------- 


// switch output lines in bit order F7-F6-F5-F4-B6-B5-B4-F1
// (order of no particular significance, but matches scancode table)
#define SW_OUT(B,F) ((F & 0xF0) | ((B & 0x70) >> 3) | ((F & 0x02) >> 1))

// switch input lines in bit order D7-D6-D4-D5
// (order corresponds to MSB-LSB at the 74154 demultiplexer inputs)
#define SW_IN(D) (((D >> 5) & 0x01) | ((D >> 3) & 0x02) | ((D >> 4) & 0x0C))

void setup() {
	int i;

	Serial.begin(9600); // for diagnostic messages

	// all pins as input with pullup
	// (pullup good for unused pins; harmless for used ones)
	for(i=0; i<=23; i++)
		pinMode(i, INPUT_PULLUP);

	Serial.println("Keyboard ready.");
}

// get the currently selected switch group (0-15)
unsigned char get_in(void)
{
	unsigned char a, b;

	do { // read twice and compare in case of a transition glitch
		a = (PIND & 0xF0);
		b = (PIND & 0xF0);
	} while(a != b); 

	return SW_IN(a);
}

// get the outputs from the currently selected switch group
// (8 bits, active low)
unsigned char get_out(void)
{ 
	unsigned char F, B;

	F = PINF;
	B = PINB;

	return SW_OUT(B,F);
}

// scancodes are by position (not caption) as if on ISO UK kbd
// with some exceptions (modifiers, DEL, ESC etc)

#define KEY_XMIT   KEY_PRINTSCREEN
#define KEY_BLANK  KEY_MENU
#define KEY_CLR    KEY_NUM_LOCK

const unsigned int scancode[16][8] = {
	0, KEY_CLR, KEY_1, KEY_E, 0, KEY_D, KEY_X, 0,
	0, KEY_F1, KEY_2, KEY_R, 0, KEY_F, KEY_C, 0,
	0, KEY_F2, KEY_4, KEY_T, 0, KEY_G, KEY_V, 0,
	0, KEY_F3, KEY_5, KEY_U, 0, KEY_H, KEY_B, 0,
	0, KEY_F4, KEY_6, KEY_I, 0, KEY_J, KEY_N, 0,
	0, KEY_F5, KEY_8, KEY_O, 0, KEY_K, KEY_S, KEY_BACKSPACE,
	0, KEY_F6, KEY_9, KEY_P, 0, KEY_L, 0, KEY_F7,
	0, KEY_Q, KEY_3, KEY_W, KEY_NON_US_BS, KEY_A, KEY_Z, 0,
	0, KEY_7, KEY_0, KEY_Y, 0, 0, KEY_M, 0,
	0, 0, KEY_MINUS, KEY_LEFT_BRACE, KEY_PERIOD, KEY_SEMICOLON, KEY_COMMA, KEY_EQUAL,
	KEY_NON_US_NUM, 0, 0, KEY_RIGHT_BRACE, KEY_SLASH, KEY_QUOTE, 0, KEY_BLANK,
	0 /* KEY_LF */, KEY_DOWN, KEY_DELETE, KEY_ESC, KEY_SPACE, KEY_ENTER, KEY_UP, KEY_LEFT,
	0, KEY_RIGHT, KEYPAD_7, KEYPAD_4, 0, KEYPAD_1, 0, 0,
	0, KEY_HOME, KEYPAD_8, KEYPAD_5, KEYPAD_0, KEYPAD_2, 0, 0,
	0, KEY_XMIT, KEYPAD_9, KEYPAD_6, KEYPAD_PERIOD, KEYPAD_3, 0, 0,
	0 /* SHIFT */, 0, 0, 0, 0 /* CTL */, KEY_CAPS_LOCK, 0, 0
	};

// emulate non-locking caps lock
void fix_caps(unsigned char *state)
{
	static int caps = 0; // assume off at startup

	if((state[15] & 0x20) == 0) { // CAPS LOCK is down
		if(caps) // already registered, don't send again
			state[15] |= 0x20;
		else // first time, make a note
			caps = 1;
	} else if(caps) { // just released, send again and make note
		caps = 0;
		state[15] &= 0xDF;
	}
}

void submit(unsigned char *state, unsigned char extras)
{
	int i, j;
	int n = 0;
	uint8_t keys[6];
	unsigned int mods = 0;

	for(i=0; i<6; i++)
		keys[i] = 0;

	if( (extras & 0x01) == 0 ) {
		Serial.print(" RPT ");
		mods |= MODIFIERKEY_GUI;
	}
	if( (extras & 0x02) == 0 ) {
		Serial.print(" END-OF-MESSAGE ");
		keys[n] = KEY_TAB;
		n++;
	}
	if( (extras & 0x04) == 0 ) {
		Serial.print(" BREAK ");
		mods |= MODIFIERKEY_ALT;
	}
	
	if((state[15] & 0x01) == 0) // either SHIFT key
		mods |= MODIFIERKEY_SHIFT;
	if((state[15] & 0x10) == 0) // CTL
		mods |= MODIFIERKEY_CTRL;
	if((state[11] & 0x01) == 0) // LF (as "AltGr")
		mods |= MODIFIERKEY_RIGHT_ALT;

	for(i=0; i<=15; i++) {
		unsigned char c = state[i];
		for(j=0; j<=7; j++) {
			if( (c & 0x01) == 0 ) {
				Serial.print("(");
				Serial.print(i);
				Serial.print(", ");
				Serial.print(j);
				Serial.print(") ");
				if(scancode[i][j] && n<6) {
					keys[n] = scancode[i][j];
					n++;
				}
			}
			c = (c >> 1);
		}
	}
	Serial.println("");

	Keyboard.set_key1(keys[0]);
	Keyboard.set_key2(keys[1]);
	Keyboard.set_key3(keys[2]);
	Keyboard.set_key4(keys[3]);
	Keyboard.set_key5(keys[4]);
	Keyboard.set_key6(keys[5]);
	Keyboard.set_modifier(mods);
	Keyboard.send_now();
}
	

void loop() {
	static unsigned char state[16];
	int i;

	// wait for switch group 0
	while(get_in() != 0);
	
	// read the entire keyboard state (the 8048 scans downwards)
	for(i=15; i>= 0; i--) {
		// wait for next switch group
		while(get_in() != i);
		delayMicroseconds(30);
		state[i] = get_out();
	}

	// if CTL+SHIFT+XMIT then reprogram
	if( ((state[15] & 0x11) == 0) && ((state[14] & 0x02) == 0) ) {
		_reboot_Teensyduino_();
		delay(100); // give reboot time to take place
	}

	fix_caps(state);
	submit(state, PINB);
}

