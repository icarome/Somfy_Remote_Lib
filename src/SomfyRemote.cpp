#include "SomfyRemote.h"

#define SYMBOL 640

SomfyRemote::SomfyRemote(byte emitterPin, uint32_t remote)
	: emitterPin(emitterPin), remote(remote) {}

void SomfyRemote::setup() {
	pinMode(2, OUTPUT);
	digitalWrite(2, LOW);
}

void SomfyRemote::sendCommand(Command command, uint32_t remoteid, uint16_t rollingCode, int repeat) {
	//const uint16_t rollingCode = 0;//rollingCodeStorage->nextCode();
	byte frame[7];
	buildFrame(frame, command, rollingCode, remoteid);
	sendFrame(frame, 2);
	//return frame;
	//for (int i = 0; i < repeat; i++) {
	//	sendFrame(frame, 7);
	//}
	//Serial.println("Linha 6");
}

void SomfyRemote::printFrame(byte *frame) {
	for (byte i = 0; i < 7; i++) {
		if (frame[i] >> 4 == 0) {  //  Displays leading zero in case the most significant
			Serial.print("0");     // nibble is a 0.
		}
		Serial.print(frame[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
}

void SomfyRemote::buildFrame(byte *frame, Command command, uint16_t code, uint32_t remoteid) {
	const byte button = static_cast<byte>(command);
	frame[0] = 0xA7;          // Encryption key. Doesn't matter much
	frame[1] = button << 4;   // Which button did  you press? The 4 LSB will be the checksum
	frame[2] = code >> 8;     // Rolling code (big endian)
	frame[3] = code;          // Rolling code
	frame[4] = remoteid >> 16;  // Remote address
	frame[5] = remoteid >> 8;   // Remote address
	frame[6] = remoteid;        // Remote address
	// Checksum calculation: a XOR of all the nibbles
	byte checksum = 0;
	for (byte i = 0; i < 7; i++) {
		checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
	}
	checksum &= 0b1111;  // We keep the last 4 bits only
	// Checksum integration
	frame[1] |= checksum;
	// Obfuscation: a XOR of all the bytes
	for (byte i = 1; i < 7; i++) {
		frame[i] ^= frame[i - 1];
	}
	// Serial.print("Obfuscated    : ");
	// printFrame(frame);
}

void SomfyRemote::sendFrame(byte *frame, byte sync) {
	if (sync == 2) {  // Only with the first frame.
		// Wake-up pulse & Silence
		sendHigh(9415);
		sendLow(9565);
		delay(80);
	}
	// Hardware sync: two sync for the first frame, seven for the following ones.
	for (int i = 0; i < sync; i++) {
		sendHigh(4 * SYMBOL);
		sendLow(4 * SYMBOL);
	}

	// Software sync
	sendHigh(4550);
	sendLow(SYMBOL);

	// Data: bits are sent one by one, starting with the MSB.
	for (byte i = 0; i < 56; i++) {
		if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1) {
			sendLow(SYMBOL);
			sendHigh(SYMBOL);
		} else {
			sendHigh(SYMBOL);
			sendLow(SYMBOL);
		}
	}
	// Inter-frame silence
	sendLow(415);
	delay(30);
}

void SomfyRemote::sendHigh(uint16_t durationInMicroseconds) {
#if defined(ESP32) || defined(ESP8266)
	digitalWrite(2, HIGH);
	delayMicroseconds(durationInMicroseconds);
#elif defined(ARDUINO_ARCH_AVR)
	// TODO fast write
	digitalWrite(2, HIGH);
	delayMicroseconds(durationInMicroseconds);
#endif
}

void SomfyRemote::sendLow(uint16_t durationInMicroseconds) {
#if defined(ESP32) || defined(ESP8266)
	digitalWrite(2, LOW);
	delayMicroseconds(durationInMicroseconds);
#elif defined(ARDUINO_ARCH_AVR)
	// TODO fast write
	digitalWrite(2, LOW);
	delayMicroseconds(durationInMicroseconds);
#endif
}

Command getSomfyCommand(const String &string) {
	if (string.equalsIgnoreCase("My")) {
		return Command::My;
	} else if (string.equalsIgnoreCase("Up")) {
		return Command::Up;
	} else if (string.equalsIgnoreCase("MyUp")) {
		return Command::MyUp;
	} else if (string.equalsIgnoreCase("Down")) {
		return Command::Down;
	} else if (string.equalsIgnoreCase("MyDown")) {
		return Command::MyDown;
	} else if (string.equalsIgnoreCase("UpDown")) {
		return Command::UpDown;
	} else if (string.equalsIgnoreCase("Prog")) {
		return Command::Prog;
	} else if (string.equalsIgnoreCase("SunFlag")) {
		return Command::SunFlag;
	} else if (string.equalsIgnoreCase("Flag")) {
		return Command::Flag;
	} else if (string.length() == 1) {
		return static_cast<Command>(strtol(string.c_str(), nullptr, 16));
	} else {
		return Command::My;
	}
}
