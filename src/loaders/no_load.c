/* Old Liquid Tracker "NO" module loader for xmp
 * Copyright (C) 2007 Claudio Matsuoka
 *
 * $Id: no_load.c,v 1.1 2007-08-12 19:23:55 cmatsuoka Exp $
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "load.h"

/* Nir Oren's Liquid Tracker old "NO" format. I have only one NO module,
 * Moti Radomski's "Time after time" from ftp.modland.com.
 */

int no_load(FILE * f)
{
	struct xxm_event *event;
	int i, j, k;
	uint32 id;
	int nsize;

	LOAD_INIT();

	id = read32b(f);
	if (id != 0x4e4f0000)		/* NO 0x00 0x00 */
		return -1;

	strcpy(xmp_ctl->type, "Liquid module (old)");

	nsize = read8(f);
	for (i = 0; i < nsize; i++) {
		uint8 x = read8(f);
		if (i < XMP_DEF_NAMESIZE)
			xmp_ctl->name[i] = x;
	}

	read16l(f);
	read16l(f);
	read16l(f);
	read16l(f);
	read8(f);
	xxh->pat = read8(f);
	read8(f);
	xxh->chn = read8(f);
	xxh->trk = xxh->pat * xxh->chn;
	read8(f);
	read16l(f);
	read16l(f);
	read8(f);
	xxh->ins = xxh->smp = 63;

	for (i = 0; i < 256; i++) {
		uint8 x = read8(f);
		if (x == 0xff)
			break;
		xxo[i] = x;
	}
	fseek(f, 255 - i, SEEK_CUR);
	xxh->len = i;

	MODULE_INFO();

	INSTRUMENT_INIT();

	/* Read instrument names */
	for (i = 0; i < xxh->ins; i++) {
		int hasname;

		xxi[i] = calloc(sizeof(struct xxm_instrument), 1);

		nsize = read8(f);
		hasname = 0;
		for (j = 0; j < nsize; j++) {
			uint8 x = read8(f);
			if (x != 0x20)
				hasname = 1;
			if (j < 32)
				xxih[i].name[j] = x;
		}
		if (!hasname)
			xxih[i].name[0] = 0;

		read32l(f);
		read32l(f);
		xxi[i][0].vol = read8(f);
		read8(f);
		read8(f);
		xxs[i].len = read16l(f);
		xxs[i].lps = read16l(f);
		xxs[i].lpe = read16l(f);
		read32l(f);
		read16l(f);

		xxih[i].nsm = !!(xxs[i].len);
		xxs[i].lps = 0;
		xxs[i].lpe = 0;
		xxs[i].flg = xxs[i].lpe > 0 ? WAVE_LOOPING : 0;
		xxi[i][0].fin = 0;
		xxi[i][0].pan = 0x80;
		xxi[i][0].sid = i;

		if (V(1) && (strlen((char*)xxih[i].name) || (xxs[i].len > 1))) {
			report("[%2X] %-22.22s  %04x %04x %04x %c V%02x\n", i,
			       xxih[i].name, xxs[i].len, xxs[i].lps, xxs[i].lpe,
			       xxs[i].flg & WAVE_LOOPING ? 'L' : ' ',
			       xxi[i][0].vol);
		}
	}

	PATTERN_INIT();

	/* Read and convert patterns */
	reportv(0, "Stored patterns: %d ", xxh->pat);

	for (i = 0; i < xxh->pat; i++) {
printf("%d  %x\n", i, ftell(f));
		PATTERN_ALLOC(i);
		xxp[i]->rows = 64;
		TRACK_ALLOC(i);

		for (j = 0; j < xxp[i]->rows; j++) {
			for (k = 0; k < xxh->chn; k++) {
				int b;
				event = &EVENT (i, k, j);

				b = read8(f);
				if (b != 0xff)
					event->note = b;
				b = read8(f);
				if (b != 0xff)
					event->ins = b;
				b = read8(f);
				b = read8(f);
			}
		}
		reportv(0, ".");
	}
	reportv(0, "\n");

	/* Read samples */
	reportv(0, "Stored samples : %d ", xxh->smp);
	for (i = 0; i < xxh->ins; i++) {
		xmp_drv_loadpatch(f, xxi[i][0].sid, xmp_ctl->c4rate,
				XMP_SMP_UNS, &xxs[xxi[i][0].sid], NULL);
		reportv(0, ".");
	}
	reportv(0, "\n");

	return 0;
}