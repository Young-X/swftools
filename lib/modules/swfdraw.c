// swfdraw.c

typedef struct _SWFSHAPEDRAWER
{
    SHAPE*shape;
    TAG*tag;
    int tagfree;
    SCOORD lastx;
    SCOORD lasty;
    SRECT bbox;
    char isfinished;
} SWFSHAPEDRAWER;

static void swf_ShapeDrawerSetLineStyle(drawer_t*draw, void*style);
static void swf_ShapeDrawerSetFillStyle(drawer_t*draw, void*style);
static void swf_ShapeDrawerMoveTo(drawer_t*draw, FPOINT * to);
static void swf_ShapeDrawerLineTo(drawer_t*draw, FPOINT * to);
static void swf_ShapeDrawerSplineTo(drawer_t*draw, FPOINT * c1, FPOINT*  to);
static void swf_ShapeDrawerFinish(drawer_t*draw);
static void swf_ShapeDrawerClear(drawer_t*draw);

static void swf_ShapeDrawerInit(drawer_t*draw, TAG*tag, int fillstylebits, int linestylebits)
{
    SWFSHAPEDRAWER*sdraw = malloc(sizeof(SWFSHAPEDRAWER));
    memset(sdraw, 0, sizeof(SWFSHAPEDRAWER));
    draw->internal = sdraw;

    draw->setLineStyle = swf_ShapeDrawerSetLineStyle;
    draw->setFillStyle = swf_ShapeDrawerSetFillStyle;
    draw->moveTo = swf_ShapeDrawerMoveTo;
    draw->lineTo = swf_ShapeDrawerLineTo;
    draw->splineTo = swf_ShapeDrawerSplineTo;
    draw->finish = swf_ShapeDrawerFinish;
    draw->dealloc = swf_ShapeDrawerClear;
    
    sdraw->tagfree = 0;
    if(tag == 0) {
	tag = swf_InsertTag(0, ST_DEFINESHAPE);
	sdraw->tagfree = 1;
    }
    sdraw->tag = tag;
    swf_ShapeNew(&sdraw->shape);
    draw->pos.x = 0;
    draw->pos.y = 0;

    swf_SetU8(sdraw->tag,0);
    sdraw->shape->bits.fill = fillstylebits;
    sdraw->shape->bits.line = linestylebits;
    
    sdraw->bbox.xmin = sdraw->bbox.ymin = SCOORD_MAX;
    sdraw->bbox.xmax = sdraw->bbox.ymax = SCOORD_MIN;

    sdraw->isfinished = 0;

    swf_ShapeSetStyle(sdraw->tag,sdraw->shape,linestylebits?1:0,fillstylebits?1:0,0/*?*/);
}

void swf_Shape01DrawerInit(drawer_t*draw, TAG*tag)
{
    swf_ShapeDrawerInit(draw, tag, 1, 0);
}

void swf_Shape11DrawerInit(drawer_t*draw, TAG*tag)
{
    swf_ShapeDrawerInit(draw, tag, 1, 1);
}

static void swf_ShapeDrawerSetLineStyle(drawer_t*draw, void*style)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
}
static void swf_ShapeDrawerSetFillStyle(drawer_t*draw, void*style)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
}
static void swf_ShapeDrawerMoveTo(drawer_t*draw, FPOINT * to)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
    int x = to->x*20;
    int y = to->y*20;
    swf_ShapeSetMove(sdraw->tag,sdraw->shape,x,y);
    sdraw->lastx = x;
    sdraw->lasty = y;
    draw->pos = *to;
}
static void swf_ShapeDrawerLineTo(drawer_t*draw, FPOINT * to)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
    int x = to->x*20;
    int y = to->y*20;
    if(sdraw->lastx < sdraw->bbox.xmin) sdraw->bbox.xmin = sdraw->lastx;
    if(sdraw->lasty < sdraw->bbox.ymin) sdraw->bbox.ymin = sdraw->lasty;
    if(sdraw->lastx > sdraw->bbox.xmax) sdraw->bbox.xmax = sdraw->lastx;
    if(sdraw->lasty > sdraw->bbox.ymax) sdraw->bbox.ymax = sdraw->lasty;
    if(x < sdraw->bbox.xmin) sdraw->bbox.xmin = x;
    if(y < sdraw->bbox.ymin) sdraw->bbox.ymin = y;
    if(x > sdraw->bbox.xmax) sdraw->bbox.xmax = x;
    if(y > sdraw->bbox.ymax) sdraw->bbox.ymax = y;
    swf_ShapeSetLine(sdraw->tag,sdraw->shape,x-sdraw->lastx,y-sdraw->lasty);
    sdraw->lastx = x;
    sdraw->lasty = y;
    draw->pos = *to;
}
static void swf_ShapeDrawerSplineTo(drawer_t*draw, FPOINT * c1, FPOINT*  to)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
    int tx = c1->x*20;
    int ty = c1->y*20;
    int x = to->x*20;
    int y = to->y*20;
    if(sdraw->lastx < sdraw->bbox.xmin) sdraw->bbox.xmin = sdraw->lastx;
    if(sdraw->lasty < sdraw->bbox.ymin) sdraw->bbox.ymin = sdraw->lasty;
    if(sdraw->lastx > sdraw->bbox.xmax) sdraw->bbox.xmax = sdraw->lastx;
    if(sdraw->lasty > sdraw->bbox.ymax) sdraw->bbox.ymax = sdraw->lasty;
    if(x < sdraw->bbox.xmin) sdraw->bbox.xmin = x;
    if(y < sdraw->bbox.ymin) sdraw->bbox.ymin = y;
    if(x > sdraw->bbox.xmax) sdraw->bbox.xmax = x;
    if(y > sdraw->bbox.ymax) sdraw->bbox.ymax = y;
    if(tx < sdraw->bbox.xmin) sdraw->bbox.xmin = tx;
    if(ty < sdraw->bbox.ymin) sdraw->bbox.ymin = ty;
    if(tx > sdraw->bbox.xmax) sdraw->bbox.xmax = tx;
    if(ty > sdraw->bbox.ymax) sdraw->bbox.ymax = ty;
    swf_ShapeSetCurve(sdraw->tag,sdraw->shape, tx-sdraw->lastx,ty-sdraw->lasty, x-tx,y-ty);
    sdraw->lastx = x;
    sdraw->lasty = y;
    draw->pos = *to;
}
static void swf_ShapeDrawerFinish(drawer_t*draw)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
    if(sdraw->bbox.xmin == SCOORD_MAX) {
	/* no points at all -> empty bounding box */
	sdraw->bbox.xmin = sdraw->bbox.ymin = 
	sdraw->bbox.xmax = sdraw->bbox.ymax = 0;
    }
    sdraw->isfinished = 1;
    swf_ShapeSetEnd(sdraw->tag);
}

static void swf_ShapeDrawerClear(drawer_t*draw)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
    if(sdraw->tagfree) {
	swf_DeleteTag(sdraw->tag);
	sdraw->tag = 0;
    }
    swf_ShapeFree(sdraw->shape);
    sdraw->shape = 0;

    free(draw->internal);
    draw->internal = 0;
}

SRECT swf_ShapeDrawerGetBBox(drawer_t*draw)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
    return sdraw->bbox;
}

SHAPE* swf_ShapeDrawerToShape(drawer_t*draw)
{
    SWFSHAPEDRAWER*sdraw = (SWFSHAPEDRAWER*)draw->internal;
    SHAPE* shape = malloc(sizeof(SHAPE));
    if(!sdraw->isfinished) {
	fprintf(stderr, "Warning: you should Finish() your drawer before calling DrawerToShape");
	swf_ShapeDrawerFinish(draw);
    }
    memcpy(shape, sdraw->shape, sizeof(SHAPE));
    shape->bitlen = (sdraw->tag->len-1)*8;
    shape->data = (U8*)malloc(sdraw->tag->len-1);
    memcpy(shape->data, &sdraw->tag->data[1], sdraw->tag->len-1);
    return shape;
}
