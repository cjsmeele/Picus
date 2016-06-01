/**
 * \file
 * \author    Chris Smeele
 * \copyright Copyright (c) 2016, Chris Smeele
 *
 * \page License
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "console.hh"

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>

void Console::puts(const char *s) {
    while (*s)
        putch(*s++);
}

void Console::clear() {
    for (int i = 0; i < 25; i++)
        putch('\n');
}

// Note: The following printf code was originally from my Stoomboot project:
// https://github.com/cjsmeele/stoomboot/blob/master/stage2/src/console.c
// Not much was changed to make it work in this environment.

#define CONSOLE_PRINTF_RESET_FORMAT() do { \
		inFormat = false; \
		memset(&flags, 0, sizeof(flags)); \
		width    = 0; \
		widthBufferIndex = 0; \
	} while (0)

typedef struct {
	bool alternative     : 1;
	bool uppercaseHex    : 1;
	bool leftAdjusted    : 1;
	bool padWithZeroes   : 1;
	bool groupDigits     : 1;
	bool alwaysPrintSign : 1;
	bool padSign         : 1;
	uint8_t _padding     : 1;
} PrintfFlags;

#define CONSOLE_PRINTF_DECIMAL_DIGIT_GROUP_CHAR '\''
#define CONSOLE_PRINTF_HEX_DIGIT_GROUP_CHAR     '.'

static size_t printfDecimal(Console &con, uint32_t num, bool sign, PrintfFlags *flags, size_t width) {
	size_t length = 0;

	if (sign) {
		if ((int32_t)num < 0) {
			con.putch('-');
			length++;
		} else if (flags->alwaysPrintSign) {
			con.putch('+');
			length++;
		} else if (flags->padSign) {
			con.putch(' ');
			length++;
		}

		num = abs((int32_t)num);
	}

	char buffer[14] = { };
	size_t i = 13;

	do {
		if (flags->groupDigits && (i == 10 || i == 6 || i == 2))
			buffer[--i] = CONSOLE_PRINTF_DECIMAL_DIGIT_GROUP_CHAR;

		buffer[--i] = (char)((num % 10) + '0');
		num /= 10;
	} while (num);

	length += 13 - i;

	if (width && width > length) {
		size_t j = 0;
		if (flags->leftAdjusted) {
			con.puts(&buffer[i]);

			for (; j<(width - length); j++)
				con.putch(' ');

		} else {
			for (; j<(width - length); j++)
				con.putch(flags->padWithZeroes ? '0' : ' ');

			con.puts(&buffer[i]);
		}

		length += j;
	} else {
		con.puts(&buffer[i]);
	}

	return length;
}

static size_t printfHex(Console &con, uint32_t num, PrintfFlags *flags, size_t width) {
	size_t length = 0;

	char buffer[20] = { };
	size_t i = 19;

	bool groupCharAdded = false;

	do {
		if (flags->groupDigits && i == 15) {
			buffer[--i] = CONSOLE_PRINTF_HEX_DIGIT_GROUP_CHAR;
			groupCharAdded = true;
		}

		uint8_t n = num & 0xf;
		buffer[--i] = (char)(n > 9 ? n - 10 + 'a' : n + '0');
		num >>= 4;
	} while (num);

	length += 19 - i - (groupCharAdded ? 1 : 0);

	if (flags->alternative)
		con.puts("0x");

	if (width && width > length) {
		size_t j = 0;
		if (flags->leftAdjusted) {
			con.puts(&buffer[i]);

			for (; j<(width - length); j++)
				con.putch(' ');

		} else {
			for (; j<(width - length); j++) {
				con.putch(flags->padWithZeroes ? '0' : ' ');
				if (flags->padWithZeroes && flags->groupDigits && width == 8 && j == 3) {
					con.putch(CONSOLE_PRINTF_HEX_DIGIT_GROUP_CHAR);
					groupCharAdded = true;
				}
			}

			if (groupCharAdded)
				length++;

			con.puts(&buffer[i]);
		}

		length += j;

	} else {
		con.puts(&buffer[i]);
	}

	if (flags->alternative)
		length += 2;

	return length;
}

int Console::printf(const char *format, ...) {

	va_list vaList;
	va_start(vaList, format);

	// Format state {

	PrintfFlags flags;
	memset(&flags, 0, sizeof(flags));

	bool inFormat = false;
	size_t width  = 0; // Minimum formatted text length.
	char   widthBuffer[9]   = { };
	size_t widthBufferIndex = 0;

	// }

	int length = 0;

	char c;
	int i = 0;
	while ((c = format[i++])) {
		if (inFormat) {
			if (c == '%') {
				// Note: '%' can be used anywhere in a '%' format substring to cancel formatting.
				putch(c);
				CONSOLE_PRINTF_RESET_FORMAT();

			} else if ((widthBufferIndex && c == '0') || (c >= '1' && c <= '9')) {
				if (widthBufferIndex >= sizeof(widthBufferIndex)) {
					// Ignore silently.
				} else {
					widthBuffer[widthBufferIndex++] = c;
				}
			} else {
				if (widthBufferIndex) {
					widthBuffer[widthBufferIndex] = 0;
                    width = (size_t)strtol(widthBuffer, NULL, 10);
				}
				// Flags {
				if (c == '-') {
					flags.leftAdjusted  = true;
					flags.padWithZeroes = false;
				} else if (c == '0') {
					flags.padWithZeroes = true;
					flags.leftAdjusted  = false;
				} else if (c == '#') {
					flags.alternative = true;
				} else if (c == '+') {
					flags.alwaysPrintSign = true;
					flags.padSign         = false;
				} else if (c == ' ') {
					flags.alwaysPrintSign = false;
					flags.padSign         = true;
				} else if (c == '\'') {
					flags.groupDigits = true;
				// }
				// Conversion specifiers {
				} else {
					bool isConversion = false;
					static const char *conversionChars = "xdcups";
					for (size_t j=0; j<strlen(conversionChars); j++)
						if (c == conversionChars[j]) {
							isConversion = true;
							break;
						}

					if (!isConversion) {
						// Format error.
						CONSOLE_PRINTF_RESET_FORMAT();
						continue;
					}

					if (c == 'd') {
						int32_t num;
						num = (int32_t)va_arg(vaList, int);

						length += printfDecimal(*this, num, true, &flags, width);

					} else if (c == 'u' || c == 'x') {
						uint32_t num;
						num = (uint32_t)va_arg(vaList, unsigned int);

						if (c == 'u')
							length += printfDecimal(*this, num, false, &flags, width);
						else if (c == 'x')
							length += printfHex(*this, num, &flags, width);

					} else if (c == 's') {
						const char *str = (char*)va_arg(vaList, char*);
						if (!str)
							str = "<null>";

						size_t slen = strlen(str);
						if (slen < width) {
							if (flags.leftAdjusted) {
								for (size_t j=0; j<(width - slen); j++)
									putch(' ');
								puts(str);
							} else {
								puts(str);
								for (size_t j=0; j<(width - slen); j++)
									putch(' ');
							}
							length += width;
						} else {
							puts(str);
							length += slen;
						}
					} else if (c == 'c') {
						char ch = (char)va_arg(vaList, int);
						putch(ch);
						length++;
					}

					CONSOLE_PRINTF_RESET_FORMAT();
				}
				// }
			}
		} else if (c == '%') {
			inFormat = true;
		} else if (c == '\n') {
			putch('\r');
			putch('\n');
			length++;
		} else {
			putch(c);
			length++;
		}
	}

	va_end(vaList);

	return length;
}

#undef CONSOLE_PRINTF_RESET_FORMAT
