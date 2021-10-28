#ifndef _UNICODE_JCH_HPP
#define _UNICODE_JCH_HPP
#include <stdint.h>
#include <vector>
#include <string>
#include "jc_array.h"
/*#pragma add("jc_files", "./unicode.jc");*/
namespace unicode {
	struct TWTF8Filter {
		int ch0 = 0;
		int nnxt = 0;
		intptr_t II = intptr_t(0L);
		int surrogate_1st = -1;
		intptr_t surrogate_II = intptr_t(0L);
		int ch_min = 0;
		intptr_t GetOffset() const {
			return this->II;
		}
		int NextByte(intptr_t I, int ch_I) {
			int ch = ch_I & 0xff;
			if ( this->nnxt > 0 ) {
				//continuation byte
				if ( uintptr_t(ch - 0x80) < 0x40u ) {
					this->ch0 = (this->ch0 << 6) + ch - 0x80;
					ch = this->ch0;
					--this->nnxt;
					if ( this->nnxt > 0 ) {
						ch = -1;
					} else if ( ch < this->ch_min ) {
						//invalid encoding
						ch = 0xfffd;
					} else if ( uintptr_t(ch - 0xd800) < 0x800u ) {
						//surrogate pair
						if ( ch & 0x0400 ) {
							//2nd
							if ( this->surrogate_1st >= 0 ) {
								ch = this->surrogate_1st * 0x400 + (ch & 0x3ff) + 0x10000;
								this->surrogate_1st = -1;
								this->II = this->surrogate_II;
							} else {
								//emit unpaired surrogate as is
								this->surrogate_1st = -1;
							}
						} else {
							//1st surrogate if 2nd, i.e., ED Bx xx follows
							//here unpaired surrogate won't make sense anyway (wcwidth is 0)
							this->surrogate_1st = ch & 0x3ff;
							this->surrogate_II = this->II;
							//emit nothing if unpaired...
							ch = -1;
						}
					} else if (ch >= 0x110000) {
						//invalid encoding
						ch = 0xfffd;
					}
				} else {
					//interrupted sequence, eat the fffd and emit ch - I-=1 won't work here
					this->nnxt = 0;
					//ch=0xfffd
					//I-=1
				}
			} else if ( uintptr_t(ch - 0xc0) < uintptr_t(0xf5 - 0xc0) ) {
				//leading byte
				this->II = I;
				this->ch0 = ch & 0x1f;
				this->nnxt = 1;
				this->ch_min = 0x80;
				if ( ch & 0x20 ) {
					this->nnxt = 2;
					this->ch_min = 0x800;
					if ( ch & 0x10 ) {
						this->nnxt = 3;
						this->ch0 &= 7;
						this->ch_min = 0x10000;
					}
				}
				ch = -1;
			} else {
				//emit ch as is
				this->II = I;
			}
			return ch;
		}
	};
	static inline int32_t GetUTF8CharacterLength(int chi) {
		if ( chi >= 65536 ) {
			return 4;
		} else if ( chi >= 2048 ) {
			return 3;
		} else if ( chi >= 128 ) {
			return 2;
		} else {
			return 1;
		}
	}
	static inline void AppendUTF8Char(std::string& output, int chi) {
		if ( chi >= 65536 ) {
			output--->push(char((chi >> 18 & 0xf) + 0xf0));
			output--->push(char(0x80 + (chi >> 12 & 63)));
			output--->push(char(0x80 + (chi >> 6 & 63)));
			output--->push(char(0x80 + (chi & 63)));
		} else if ( chi >= 2048 ) {
			output--->push(char((chi >> 12 & 0xf) + 0xe0));
			output--->push(char(0x80 + (chi >> 6 & 63)));
			output--->push(char(0x80 + (chi & 63)));
		} else if ( chi >= 128 ) {
			output--->push(char((chi >> 6) + 0xc0));
			output--->push(char(0x80 + (chi & 63)));
		} else {
			output--->push(char(chi));
		}
	}
	std::vector<uint16_t> WTF8ToUTF16(std::span<char> s);
	std::vector<int32_t> WTF8ToUTF32(std::span<char> s);
	std::string UTF16ToUTF8(std::span<uint16_t> s);
	std::string UTF32ToUTF8(std::span<int32_t> s);
};

#endif
