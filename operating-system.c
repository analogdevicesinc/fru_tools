/*
 * operating_system.c
 * Copyright (C) 2012-2015 Analog Devices
 * Author : Robin Getz <robin.getz@analog.com>
 *
 * fru-dump is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * fru-dump is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * fru-dump includes the files fru.c and fru.h, which are released under a
 * BSD-like license. These files can be used seperately from fru-dump,
 * and include in non-GPL software.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#define __USE_XOPEN /* needed for strptime */
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>


#include "fru.h"

#define DUMP_BOARD     (0x01)
#define DUMP_SUPPLY    (0x02)
#define DUMP_CONNECTOR (0x04)
#define DUMP_I2C       (0x08)

int quiet = 0;
int verbose = 0;

void * x_calloc (size_t nmemb, size_t size)
{
	unsigned int *ptr;

	ptr = calloc(nmemb, size);
	if (ptr == NULL)
		printf_err("memory error - calloc returned zero\n");

	return (void *)ptr;
}

void printf_err (const char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	printf("fru_dump %s, built %s\n", VERSION, VERSION_DATE);
	vfprintf(stderr,fmt,ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void printf_warn (const char * fmt, ...)
{
	va_list ap;

	if (quiet || !verbose)
		return;

	va_start(ap,fmt);
	vprintf(fmt,ap);
	va_end(ap);
}

void printf_info (const char * fmt, ...)
{
	va_list ap;

	if (quiet)
		return;

	va_start(ap,fmt);
	vprintf(fmt,ap);
	va_end(ap);
}

/*
 * Read in the file of the disk, or from the EEPROM
 */
unsigned char * read_file(char *file_name)
{
	FILE *fp;
	unsigned char *p;
	size_t tmp;
	int i = 0;
	long size;

	fp = fopen(file_name, "rb");
	if(fp == NULL)
		printf_err("Cannot open file '%s'\n", file_name);

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	p = x_calloc(1, size);
	tmp = fread(p, size + 1, 1, fp); /* + 1 to trigger the end of file */
	if (!feof(fp))
		printf_err("Didn't read the entire input file (%s)\n", file_name);
	/*
	 * If an error  occurs,  or the  end-of-file is reached, 
	 * the return value is a short item count (or zero).
	 */
	if (tmp == 0)
		for (i = size-1; p[i] == 0; i--);

	printf_info("read %i bytes from %s\n", i+1, file_name);

	fclose(fp);
	return p;
}


void write_FRU(struct FRU_DATA *fru, char * file_name, bool packed)
{
	size_t tmp, len;
	unsigned char *buf = NULL;
	FILE *fp;

	/* Build as ASCII output */
	buf = build_FRU_blob(fru, &len, packed);

	/* If it's too big, try again, with 6-bit packed */
	if (len >= 255 && !packed) {
		free(buf);
		buf = build_FRU_blob(fru, &len, 1);
		if (len >= 255) {
			free(buf);
			printf_err("Not able to pack things into 255 char, no output\n");
			return;
		}
	}

	if (!strcmp("-", file_name)) {
		tmp = write(STDOUT_FILENO, buf, len);
		if (tmp != len)
			printf_err ("Didn't write entire file\n");
	} else {
		if((fp = fopen(file_name, "wb")) == NULL)
			printf_err("Cannot open file.\n");

		fwrite(buf, 1, len, fp);
		fclose(fp);
	}
	printf_info("wrote %i bytes to %s\n", len, file_name);

	free(buf);
}

static void dump_fru_field(const char * description, size_t offset, unsigned char * field)
{
	/* does field exist, and have length? */
	if (field) {
		printf("%s\t: ", description);
		if (FIELD_LEN(field)) {
			if (TYPE_CODE(field) == FRU_STRING_ASCII || offset) {
				printf("%s\n", &field[offset + 1]);
			} else {
				printf("Non-ASCII\n");
			}
		} else
			printf("Empty Field\n");
	}
}

void dump_BOARD(struct BOARD_INFO *fru)
{
	unsigned int i, j;
	time_t tmp = min2date(fru->mfg_date);

	printf("Date of Man\t: %s", ctime(&tmp));
	dump_fru_field("Manufacturer", 0, fru->manufacturer);
	dump_fru_field("Product Name", 0, fru->product_name);
	dump_fru_field("Serial Number", 0, fru->serial_number);
	dump_fru_field("Part Number", 0, fru->part_number);
	dump_fru_field("FRU File ID", 0, fru->FRU_file_ID);

	if (!strncasecmp((const char *)&fru->manufacturer[1], "Analog Devices", strlen("Analog Devices"))) {
		for (i = 0; i < CUSTOM_FIELDS; i++) {
			/* These are ADI custom fields */
			if (fru->custom[i] && fru->custom[i][0] & 0x3F) {
				switch (fru->custom[i][1]) {
					case 0:
						dump_fru_field("PCB Rev ", 1, fru->custom[i]);
						break;
					case 1:
						dump_fru_field("PCB ID  ", 1, fru->custom[i]);
						break;
					case 2:
						dump_fru_field("BOM Rev ", 1, fru->custom[i]);
						break;
					case 3:
						dump_fru_field("Uses LVDS", 1, fru->custom[i]);
						break;
					case 4:
						dump_fru_field("Tuning  ", 1, fru->custom[i]);
						break;
					default:
						dump_fru_field("Unknown ", 1, fru->custom[i]);
						break;
				}
			}
		}
	} else {
		printf("Custom Fields:\n");
		for (i = 0; i < CUSTOM_FIELDS; i++) {
			if (fru->custom[i] && fru->custom[i][0] & 0x3F) {
				printf("  Field %i (len=%i):", i, fru->custom[i][0] & 0x3F);
				for (j = 1 ; j <= (fru->custom[i][0] & 0x3F); j++)
					printf(" %02x", fru->custom[i][j] & 0xFF);
				printf("  |");
				for (j = 1 ; j <= (fru->custom[i][0] & 0x3F); j++)
					printf("%c", ((fru->custom[i][j] < 32) || (fru->custom[i][j] >= 127)) ? '.': fru->custom[i][j]);
				printf("|\n");
			}
		}
	}
}

/* 
 * DC Load and DC Output Multi-record Definitions
 * Table 8 from the VITA/ANSI 57.1 Spec
 */
const char * DC_Loads[] = {
	"P1 VADJ",			/* Load   :  0 */
	"P1 3P3V",			/* Load   :  1 */
	"P1 12P0V",			/* Load   :  2 */
	"P1 VIO_B_M2C",			/* Output :  3 */
	"P1 VREF_A_M2C",		/* Output :  4 */
	"P1 VREF_B_M2C",		/* Output :  5 */
	"P2 VADJ",			/* Load   :  6 */
	"P2 3P3V",			/* Load   :  7 */
	"P2 12P0V",			/* Load   :  8 */
	"P2 VIO_B_M2C",			/* Load   :  9 */
	"P2 VREF_A_M2C",		/* Load   : 10 */
	"P2 VREF_B_M2C",		/* Load   : 11 */
};

void dump_MULTIRECORD (struct MULTIRECORD_INFO *fru)
{
	unsigned char *p, *n, *z;
	int i;

	z = x_calloc(1, 12);

	for (i= 0; i <= NUM_SUPPLIES - 1; i++) {
		if (!fru->supplies[i])
			continue;
		p = fru->supplies[i];
		n = p + 5;
		switch(p[0]) {
			case 1:
				printf("DC Output\n");
				printf("  Output Number: %d (%s)\n", n[0] & 0xF, DC_Loads[n[0] & 0xF]);
				if (memcmp(&n[1], z, 11)) {
					printf("  Nominal volts:              %d (mV)\n", (n[ 1] | (n[ 2] << 8)) * 10);
					printf("  Maximum negative deviation: %d (mV)\n", (n[ 3] | (n[ 4] << 8)) * 10);
					printf("  Maximum positive deviation: %d (mV)\n", (n[ 5] | (n[ 6] << 8)) * 10);
					printf("  Ripple and Noise pk-pk:     %d (mV)\n",  n[ 7] | (n[ 8] << 8));
					printf("  Minimum current draw:       %d (mA)\n",  n[ 9] | (n[10] << 8));
					printf("  Maximum current draw:       %d (mA)\n",  n[11] | (n[12] << 8));
				} else
					printf("  All Zeros\n");
				break;
			case 2:
				printf("DC Load\n");
				printf("  Output number: %d (%s)\n", n[0] & 0xF, DC_Loads[n[0] & 0xF]);
				printf("  Nominal Volts:         %04d (mV)\n", (n[ 1] | (n[ 2] << 8)) * 10);
				printf("  minimum voltage:       %04d (mV)\n", (n[ 3] | (n[ 4] << 8)) * 10);
				printf("  maximum voltage:       %04d (mV)\n", (n[ 5] | (n[ 6] << 8)) * 10);
				printf("  Ripple and Noise pk-pk %04d (mV)\n",  n[ 7] | (n[ 8] << 8));
				printf("  Minimum current load   %04d (mA)\n",  n[ 9] | (n[10] << 8));
				printf("  Maximum current load   %04d (mA)\n",  n[11] | (n[12] << 8));
				break;
		}
	}
	free (z);

}

void dump_i2c (struct MULTIRECORD_INFO *fru)
{
	unsigned char *p, *n;
	unsigned int shift;

	if (!fru->i2c_devices) {
		printf("No I2C information\n");
		return;
	}

	p = fru->i2c_devices;
	while (*p){
		n = p;
		/* skip address for now */
		while (*p < '0') p++;
		/* print name */
		while (*p >= '0') {
			printf("%c", *p);
			p++;
		}
		/* now print the address */
		printf("\t");
		while ((*n -0x20) <= 0x0F) {
			printf("0x%02x|0x%02x (0b", (*n - 0x20) << 4, (*n - 0x20) << 3);
			for (shift = 0x08; shift > 0; shift >>= 1)
				printf("%s", (((*n - 0x20) & shift) == shift) ? "1" : "0");
			printf("nnn[RW]);  ");
			n++;
		}
		printf("\n");
	}
}

void dump_FMConnector (struct MULTIRECORD_INFO *fru)
{

	unsigned char *p, *n;

	if (!fru->connector) {
		printf("No Connector information\n");
		return;
	}

	p = fru->connector;
	n = p + 5;

	n += 3;
	switch (n[1]>>6) {
		case 0:
			printf("Single Width Card\n");
			break;
		case 1:
			printf("Double Width Card\n");
			break;
		default:
			printf("error - not the right size\n");
			break;
	}
	switch ((n[1] >> 4) & 0x3) {
		case 0:
			printf("P1 is LPC\n");
			break;
		case 1:
			printf("P1 is HPC\n");
			break;
		default:
			printf("P1 not legal size\n");
			break;
	}
	switch ((n[1] >> 2) & 0x3) {
		case 0:
			printf("P2 is LPC\n");
			break;
		case 1:
			printf("P2 is HPC\n");
			break;
		case 3:
			if (n[1]>>6 != 0)
				printf("P2 is not populated\n");
			break;
		default:
			printf("P2 not legal size\n");
			break;
	}
	printf("P1 Bank A Signals needed %d\n", n[2]);
	printf("P1 Bank B Signals needed %d\n", n[3]);
	printf("P1 GBT Transceivers needed %d\n", n[6] >> 4);
	if (((n[1] >> 2) & 0x3) != 3) {
		printf("P2 Bank A Signals needed %d\n", n[4]);
		printf("P2 Bank B Signals needed %d\n", n[5]);
		printf("P2 GBT Transceivers needed %d\n", n[6] & 0xF);
	}
	printf("Max JTAG Clock %d\n", n[7]);

}

void usage (void)
{
	printf("fru_dump %s, built %s\n", VERSION, VERSION_DATE);
	printf(" Copyright (C) 2012-2015 Analog Devices, Inc.\n"
		" This is free software; see the source for copying conditions.\n"
		" There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n"
		" PARTICULAR PURPOSE.\n\n");
	printf("dump information about FRU files for FMC Cards\n"
		"  file options\n"
		"    -i\tinput file\n"
		"    -o\toutput file, only makes sense when changing something\n");
	printf("  dump info\n"
		"    -b\tdump board info\n"
		"    -c\tdump connector info\n"
		"    -p\tdump power supply info\n"
		"    -2\tdump I2C info\n"
		"    -v\tverbose (show warnings)\n");
	printf("  set info (modifies output file)\n"
		"    -d <num>\tset date (Number of minutes from 0:00 hrs 01Jan1996)\n"
#ifndef __MINGW32__
		"    -d <date>\tset date (Date in RFC3339 format: 2012-12-21T15:12:30-05:00)\n"
#endif
		"    -d now\tset the date to the current time\n"
		"    -s <str>\tset serial number (string)\n"
		"    -t <str>\tset tuning parameters\n"
		"    -r <str>\tset pcb revision\n"
		"    -6\t\tforce output to be in 6-bit ASCII\n"
	);
}

int main(int argc, char **argv)
{
	char *input_file = NULL, *p = NULL;
	char *serial = NULL;
	char *tuning = NULL;
	char *pcb_rev = NULL;
	char *output_file = NULL;
	unsigned int date = 0;
	int c;
	unsigned char *raw_input_data = NULL;
	struct FRU_DATA *fru = NULL;
	int dump = 0;
	bool force_packing = false;
	bool output_required = false;

	opterr = 0;
	while ((c = getopt (argc, argv, "26bcpv?d:h:s:t:r:i:o:")) != -1)
	switch (c) {
		case 'b':
			dump |= DUMP_BOARD;
			break;
		case 'c':
			dump |= DUMP_CONNECTOR;
			break;
		case '2':
			dump |= DUMP_I2C;
			break;
		case '6':
			force_packing = true;
			break;
		case 'd':
		{
			struct tm time_tm, time_1996;
			output_required = true;

			memset(&time_tm, 0, sizeof(struct tm));
			memset(&time_1996, 0, sizeof(struct tm));

			if (!strcmp(optarg, "now")) {
				time_t tmp = time(NULL);
				time_tm = *gmtime(&tmp);
				time_1996.tm_year = 96;
				time_1996.tm_mday = 1;
				date = (int)difftime(mktime(&time_tm), mktime(&time_1996)) / 60;
				break;
			} else {
#ifndef __MINGW32__
				p = strptime(optarg, "%Y-%m-%dT%H:%M:%S%z", &time_tm);
#endif
			}
			if (p && time_tm.tm_year) {
				time_1996.tm_year = 96;
				time_1996.tm_mday = 1;

				/* this returns seconds, we need minutes */
				date = (int)difftime(mktime(&time_tm), mktime(&time_1996)) / 60;
			} else {
				date = atoi(optarg);
			}
			if (date > 0xFFFFFF)
					printf_err("err: date is too large to fit in three byte\n");
		}
			break;
		case 'p':
			dump |= DUMP_SUPPLY;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'o':
			output_file = optarg;
			if (!strcmp("-", output_file))
				quiet = 1;
			break;
		case 's':
			output_required = true;
			serial = optarg;
			break;
		case 't':
			output_required = true;
			tuning = optarg;
			break;
		case 'r':
			output_required = true;
			pcb_rev = optarg;
			break;
		case 'i':
			input_file = optarg;
			break;
		case '?':
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		default:
			printf_info("Unknown option: %c\n", c);
			usage();
			exit(EXIT_FAILURE);
	}

	if (!input_file &&  (optind == (argc -1)))
		input_file = argv[optind];

	if (input_file)
		raw_input_data = read_file(input_file);
	else
		printf_err("no input file specified\n");

	if (output_required && !output_file)
		printf_err("no output file specified\n");

	if (raw_input_data) {
		fru = parse_FRU(raw_input_data);
		free(raw_input_data);
	}

	if (serial) {
		free(fru->Board_Area->serial_number);
		fru->Board_Area->serial_number = x_calloc(1, strlen(serial)+3);
		fru->Board_Area->serial_number[0] = strlen(serial) | (FRU_STRING_ASCII << 6);
		memcpy(&fru->Board_Area->serial_number[1], serial, strlen(serial));
		printf_info("changing serial number to %s\n", serial);
	}

	if (tuning) {
		fru->Board_Area->custom[4] = x_calloc(1, strlen(tuning)+3);
		fru->Board_Area->custom[4][0] = strlen(tuning) | (FRU_STRING_ASCII << 6);
		fru->Board_Area->custom[4][1] = 4;
		memcpy(&fru->Board_Area->custom[4][2], tuning, strlen(tuning));
		printf_info("changing tuning parameter to '%s'\n", tuning, strlen(tuning));
	}

	if (date) {
		time_t tmp;
		tmp = min2date(date);
		fru->Board_Area->mfg_date = date;
		printf_info("changing date to %s", ctime(&tmp));
	}

	if (pcb_rev) {
		fru->Board_Area->custom[0] = x_calloc(1, strlen(pcb_rev)+3);
		fru->Board_Area->custom[0][0] = (strlen(pcb_rev) + 1) | (FRU_STRING_BINARY << 6);
		fru->Board_Area->custom[0][1] = 0; //custom field index
		memcpy(&fru->Board_Area->custom[0][2], pcb_rev, strlen(pcb_rev));
		printf_info("changing pcb_rev to '%s'\n", pcb_rev, strlen(pcb_rev));
	}

	if (fru && dump & DUMP_BOARD)
		dump_BOARD(fru->Board_Area);

	if (fru && dump & DUMP_SUPPLY)
		dump_MULTIRECORD(fru->MultiRecord_Area);

	if (fru && dump & DUMP_CONNECTOR)
		dump_FMConnector(fru->MultiRecord_Area);

	if (fru && dump & DUMP_I2C)
		dump_i2c(fru->MultiRecord_Area);

	if (output_file)
		write_FRU(fru, output_file, force_packing);

	free_FRU(fru);

	exit(EXIT_SUCCESS);
}
