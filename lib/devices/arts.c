/* arts.c

   Part of the swftools package.

   Copyright (c) 2005 Matthias Kramm <kramm@quiss.org> 
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include "../mem.h"
#include "../gfxdevice.h"
#include "../gfxtools.h"
#include "../art/libart.h"
#include "artsutils.c"

typedef struct _clip {
    ArtSVP*svp;
    struct _clip*next;
} clip_t;

typedef struct _internal {
    gfxdevice_t*out;
    clip_t*clip;
} internal_t;

int arts_setparameter(struct _gfxdevice*dev, const char*key, const char*value)
{
    internal_t*i = (internal_t*)dev->internal;
    return i->out->setparameter(i->out,key,value);
}

void arts_startpage(struct _gfxdevice*dev, int width, int height)
{
    internal_t*i = (internal_t*)dev->internal;
    i->out->startpage(i->out,width,height);
}

void arts_startclip(struct _gfxdevice*dev, gfxline_t*line)
{
    internal_t*i = (internal_t*)dev->internal;
    ArtSVP* svp = gfxfillToSVP(line, 1);

    svp = art_svp_rewind_uncrossed(art_svp_uncross(svp),ART_WIND_RULE_ODDEVEN); /*FIXME*/

    if(i->clip) {
	ArtSVP*old = i->clip->svp;
	clip_t*n = i->clip;
	i->clip = (clip_t*)rfx_calloc(sizeof(clip_t));
	i->clip->next = n;
	i->clip->svp = art_svp_intersect(svp, old);
	art_svp_free(svp);
    } else {
	i->clip = rfx_calloc(sizeof(clip_t));
	i->clip->svp = svp;
    }
}

void arts_endclip(struct _gfxdevice*dev)
{
    internal_t*i = (internal_t*)dev->internal;

    if(i->clip) {
	clip_t*old = i->clip;
	i->clip = i->clip->next;
	art_svp_free(old->svp);old->svp = 0;
	old->next = 0;free(old);
    } else {
	fprintf(stderr, "Error: endclip without startclip\n");
    }
}

void arts_stroke(struct _gfxdevice*dev, gfxline_t*line, gfxcoord_t width, gfxcolor_t*color, gfx_capType cap_style, gfx_joinType joint_style, gfxcoord_t miterLimit)
{
    internal_t*i = (internal_t*)dev->internal;
    //i->out->stroke(i->out, line, width, color, cap_style, joint_style, miterLimit);
    ArtSVP* svp = gfxstrokeToSVP(line, width, cap_style, joint_style, miterLimit);
    if(i->clip) {
	ArtSVP*old = svp;
	svp = art_svp_intersect(svp, i->clip->svp);
	art_svp_free(old);
    }
    gfxline_t*gfxline = SVPtogfxline(svp);
    i->out->fill(i->out, gfxline, color);
    free(gfxline);
    art_svp_free(svp);
}

void arts_fill(struct _gfxdevice*dev, gfxline_t*line, gfxcolor_t*color)
{
    internal_t*i = (internal_t*)dev->internal;
    ArtSVP* svp = gfxfillToSVP(line, 1);
    
    svp = art_svp_rewind_uncrossed(art_svp_uncross(svp),ART_WIND_RULE_ODDEVEN); /*FIXME*/

    if(i->clip) {
	ArtSVP*old = svp;
	svp = art_svp_intersect(svp, i->clip->svp);
	art_svp_free(old);
    }
    gfxline_t*gfxline = SVPtogfxline(svp);
    i->out->fill(i->out, gfxline, color);
    free(gfxline);
    art_svp_free(svp);
}

void arts_fillbitmap(struct _gfxdevice*dev, gfxline_t*line, gfximage_t*img, gfxmatrix_t*matrix, gfxcxform_t*cxform)
{
    internal_t*i = (internal_t*)dev->internal;
    ArtSVP* svp = gfxfillToSVP(line, 1);
    if(i->clip) {
	ArtSVP*old = svp;
	svp = art_svp_intersect(svp, i->clip->svp);
	art_svp_free(old);
    }
    gfxline_t*gfxline = SVPtogfxline(svp);
    i->out->fillbitmap(i->out, gfxline, img, matrix, cxform);
    free(gfxline);
    art_svp_free(svp);
}

void arts_fillgradient(struct _gfxdevice*dev, gfxline_t*line, gfxgradient_t*gradient, gfxgradienttype_t type, gfxmatrix_t*matrix)
{
    internal_t*i = (internal_t*)dev->internal;
    ArtSVP* svp = gfxfillToSVP(line, 1);
    if(i->clip) {
	ArtSVP*old = svp;
	svp = art_svp_intersect(svp, i->clip->svp);
	art_svp_free(old);
    }
    gfxline_t*gfxline = SVPtogfxline(svp);
    i->out->fillgradient(i->out, gfxline, gradient, type, matrix);
    free(gfxline);
    art_svp_free(svp);
}

void arts_addfont(struct _gfxdevice*dev, gfxfont_t*font)
{
    internal_t*i = (internal_t*)dev->internal;
    i->out->addfont(i->out, font);
}

void arts_drawchar(struct _gfxdevice*dev, gfxfont_t*font, int glyphnr, gfxcolor_t*color, gfxmatrix_t*matrix)
{
    internal_t*i = (internal_t*)dev->internal;
    gfxline_t*glyph = gfxline_clone(font->glyphs[glyphnr].line);
    gfxline_transform(glyph, matrix);

    if(i->clip) {
	gfxbbox_t bbox = gfxline_getbbox(glyph);
	ArtSVP*dummybox = boxToSVP(bbox.xmin,bbox.ymin,bbox.xmax,bbox.ymax);
	ArtSVP*svp = art_svp_intersect(dummybox, i->clip->svp);
	gfxline_t*gfxline = SVPtogfxline(svp);
	gfxbbox_t bbox2 = gfxline_getbbox(gfxline);
	double w = bbox2.xmax - bbox2.xmin;
	double h = bbox2.ymax - bbox2.ymin;
	if(w < 0.001 || h < 0.001) /* character was clipped completely */ {
	} else if(fabs((bbox.xmax - bbox.xmin) - w) > 0.05 ||
                  fabs((bbox.ymax - bbox.ymin) - h) > 0.05) {
	    /* notable change in character size: character was clipped 
	       TODO: handle diagonal cuts
	     */
	    arts_fill(dev, glyph, color);
	} else {
	    i->out->drawchar(i->out, font, glyphnr, color, matrix);
	}
    } else {
	i->out->drawchar(i->out, font, glyphnr, color, matrix);
    }
    
    gfxline_free(glyph);
}

void arts_drawlink(struct _gfxdevice*dev, gfxline_t*line, char*action)
{
    internal_t*i = (internal_t*)dev->internal;
    i->out->drawlink(i->out, line, action);
}

void arts_endpage(struct _gfxdevice*dev)
{
    internal_t*i = (internal_t*)dev->internal;
    i->out->endpage(i->out);
}

gfxresult_t* arts_finish(struct _gfxdevice*dev)
{
    internal_t*i = (internal_t*)dev->internal;
    return i->out->finish(i->out);
}

void gfxdevice_arts_init(gfxdevice_t*dev, gfxdevice_t*out)
{
    internal_t*i = rfx_calloc(sizeof(internal_t));
    memset(dev, 0, sizeof(gfxdevice_t));
    dev->internal = i;

    dev->setparameter = arts_setparameter;
    dev->startpage = arts_startpage;
    dev->startclip = arts_startclip;
    dev->endclip = arts_endclip;
    dev->stroke = arts_stroke;
    dev->fill = arts_fill;
    dev->fillbitmap = arts_fillbitmap;
    dev->fillgradient = arts_fillgradient;
    dev->addfont = arts_addfont;
    dev->drawchar = arts_drawchar;
    dev->drawlink = arts_drawlink;
    dev->endpage = arts_endpage;
    dev->finish = arts_finish;

    i->out = out;
}

