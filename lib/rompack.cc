/*
 * Open Runes of Magic server - data packing/unpacking
 * Copyright (C) 2013-2015 Rink Springer <rink@rink.nu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "rompack.h"
#include <stdint.h>
#include <assert.h>
#include <string.h> // for memcpy()

#include <stdio.h>

bool
ROMPack::Unpack(const uint8_t* src, int srclen, uint8_t* dst, int* outlen)
{
	uint32_t code;
	uint8_t* out;
	const uint8_t* in;
	uint8_t* buf;
	*outlen = 0;

#define COPY(to, from, count) \
	while (count > 0) { \
		*(uint8_t*)to = *(uint8_t*)from; \
		to++, from++, count--; \
	}

#define HANDLE_COUNT(acc, mask) \
	acc &= mask; \
	if (acc == 0) { \
		while (*(uint8_t*)in == 0) \
			(acc) += 255, in++; \
		(acc) += *(uint8_t*)in + mask; \
		in++; \
	}

	/*
	 * The first byte 'b' is special: if it is >= 0x11, this means a
	 * literal copy of b - 0x11 bytes is to take place and we'll
	 * process as if a 00 .. 0F byte was processed.
	 *
	 * Note that this doesn't seem to be used often, if at all.
	 */
	in = src;
	out = dst;
	if (*(uint8_t*)in > 0x11) {
		code = *(uint8_t*)in - 0x11;
		in++;
		if (code < 4) {
			COPY(out, in, code);
			goto try_00to0F_standard;
		}

		COPY(out, in, code);
		goto try_00to0F_alternative;
	}

try_00to0F_standard:
	/*
	 * Fetch command; 00..0F have their standard meaning here.
	 */
	code = *(uint8_t*)in;
	in++;
	if (code >= 0x10) goto try_standard;

	/*
	 * Standard behaviour of byte 00..0f
	 *
	 * code: 0000aaaa                    count = 3 + aaaa
	 *       00000000 <#zeroes> <byte>   count = 3 + 15 + <#zeroes> * 255
	 *
	 * Copies count bytes of literals, advancing both in and out.
	 */
	HANDLE_COUNT(code, 15);
	code += 3;
	COPY(out, in, code);

try_00to0F_alternative:
	/*
	 * If we are here, a 00..0F command has the alternative meaning.
	 */
	code = *(uint8_t*)in;
	in++;
	if (code >= 0x10) goto try_standard;

	/*
	 * Alternative behaviour of byte 00..0f
	 *
	 * 0000aaxx bbbbbbbb
	 *
	 * Copies 3 bytes from 'out - aa - bbbbbbbb * 4 - 0x801' to out; then
	 * adds xx literal bytes.
	 */
	buf = out - (code >> 2) /* aa */- (*(uint8_t*)in * 4) /* bbbbbbbb */ - 0x801;
	in++;

	// Copy 3 bytes from buf -> out
	*(uint8_t*)out = *buf;
	out++, buf++;
	*(uint8_t*)out = *buf;
	out++, buf++;
	*(uint8_t*)out = *buf;
	out++;

handle_offset_count:
	/*
	 * Previous command likely ended with an offset piece; the lowest two
	 * bits of that one represent a copy. Do so here.
	 *
	 * Beware that processing a code byte here will switch to the
	 * alternative behaviour of byte 00..0F.
	 */
	code = *(uint8_t*)(in - 2) & 3;
	if (code == 0)
		goto try_00to0F_standard;

	COPY(out, in, code);

	// Obtain next code
	code = *(uint8_t*)in;
	in++;

try_standard:
	/*
	 * Decode a code using the most common meaning.
	 */
	if (code >= 0x40) {
		/*
		 * code: cccbbbaa dddddddd, where ccc >= 2 and count = ccc + 1
		 *
		 * Copies 'count' bytes from 'out - bbb - dddddddd * 8 - 1' to
		 * 'in' and copy 'aa' literals.
		 *
		 */
		buf = out - ((code >> 2) & 7) /* bbb */ - ((uint32_t)*(uint8_t*)in * 8) /* dddddddd * 8 */ - 1;
		code = (code >> 5) /* ccc */ + 1;
		in++;

		COPY(out, buf, code);
		goto handle_offset_count;
	}
	if (code >= 0x20 /* && code < 0x40 */) {
		/*
		 * code: 001aaaaa oooooooooooooobb                    count = 2 + aaaa
		 *    or 00100000 <#zeroes> <val> oooooooooooooobb    count = 2 + 31 + (#zeroes * 255) + val
		 *
		 * Copies 'count' bytes from 'out - oooooooooooooo - 1' to
		 * 'out' and copy 'bb' literals.
		 */
		HANDLE_COUNT(code, 31);

		buf = (out - ((*(uint16_t*)in) >> 2)) /* oooooooooooooo */ - 1;
		in += 2;
	} else /* code < 0x20 */ {
#if 0
		/*
		 * The original code checks the following condition, but it
		 * seems impossible to end up there... this may be a bug,
		 * especially since 'in' isn't incremented...
		 */
		if (code < 0x10) {

			code = code >> 2;
			buf = out - code - (*(uint8_t*)in * 4) - 1;
		} else /* code >= 0x10 && code < 0x20 */ {
#else
		if (code < 0x10)
			return 0; // XXX ?!
			assert(code >= 0x10);
#endif
			/*
			 * code: 0010baaa oooooooooooooobb                   count = 2 + aaa
			 *    or 00100000 <#zeroes> <val> oooooooooooooobb   count = 2 + 7 + (#zeroes * 255) + val
			 *    or 00100aaa 0000000000000000                   aaa > 0 (end of stream)
			 *
			 * On end-of-stream condition, terminates processing,
			 * yielding success if the entire input stream has been
			 * consumed.
			 *
			 * Copies 'count' bytes from 'buf - b * 2048 -
			 * oooooooooooooo - 16384' to 'out' and copy 'bb'
			 * literals.
			 */
			buf = out - ((code & 8) << 0xb);
			HANDLE_COUNT(code, 7);
#if 0 
		}
#endif
		buf -= (*(uint16_t*)in >> 2) /* oooooooooooooo */;
		in += 2;
		if (buf == out) {
			// End-of-stream marker found; store length
			*outlen = out - dst;

			// We succeeded only if we processed the entire input stream
			return in == src + srclen;
		}
		buf -= 0x4000;
	}

	code += 2;
	COPY(out, buf, code);
	goto handle_offset_count;

#undef HANDLE_COUNT
#undef COPY
}

bool
ROMPack::Pack(const uint8_t* src, int srclen, uint8_t* dst, int* outlen)
{
	/*
	 * XXX We are lazy: we'll just use the 'copy literal', not compressing the
	 *     data at all.
	 */
	uint8_t* out = dst;
	*out++ = 0x00; // command byte
	{
		int len = srclen - 18;
		while (len > 255) {
			*out++ = 0; len -= 255;
		}
		assert(len != 0); // XXX need to find a way around this
		*out++ = len;
	}

	// Now copy the data bytes
	memcpy(out, src, srclen);
	out += srclen;

	// Write the end-of-stream marker
	*out++ = 0x11;
	*out++ = 0x00;
	*out++ = 0x00;

	// All done!
	*outlen = out - dst;
	return true;
}

/* vim:set ts=2 sw=2: */
