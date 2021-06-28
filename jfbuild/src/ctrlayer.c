// CTR interface layer
// for the Build Engine

# include <3ds.h>

#include <stdlib.h>
#include <math.h>

#include "build.h"
#include "cache1d.h"
#include "pragmas.h"
#include "a.h"
#include "osd.h"

// undefine to restrict windowed resolutions to conventional sizes
#define ANY_WINDOWED_SIZE

int   _buildargc = 1;
const char **_buildargv = NULL;

char quitevent=0, appactive=1;
int xres=-1, yres=-1, bpp=0, fullscreen=0, bytesperline, imageSize;
intptr_t frameplace=0;
char modechange=1;
char offscreenrendering=0;
char videomodereset = 0;

uint8_t *framebuffer;
uint16_t *fb;
uint16_t ctrlayer_pal[256];

// input
int inputdevices=0;
char keystatus[256];
static char keynames[256][24];
int mousex=0,mousey=0,mouseb=0;
static char mouseacquired=0,moustat=0;
int joyaxis[4], joyb=0;
char joynumaxes=0, joynumbuttons=0;

void (*keypresscallback)(int,int) = 0;
void (*mousepresscallback)(int,int) = 0;
void (*joypresscallback)(int,int) = 0;


int wm_msgbox(const char *name, const char *fmt, ...)
{
}

int wm_ynbox(const char *name, const char *fmt, ...)
{
}

int wm_filechooser(const char *initialdir, const char *initialfile, const char *type, int foropen, char **choice)
{
}

void wm_setapptitle(const char *name)
{
}

void wm_setwindowtitle(const char *name)
{
}

//
//
// ---------------------------------------
//
// System
//
// ---------------------------------------
//
//

int main(int argc, char *argv[])
{
	int r;

	chdir("sdmc:/3ds/jfsw");

	osSetSpeedupEnable(true);

	gfxInit(GSP_RGB565_OES,GSP_RGB565_OES,false);
	gfxSetDoubleBuffering(GFX_TOP, false);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	gfxSet3D(false);
	consoleInit(GFX_BOTTOM, NULL);

	framebuffer = malloc( 400 * 240 );

	fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

	_buildargc = argc;
	_buildargv = (const char **)argv;

	baselayer_init();

	r = app_main(_buildargc, (char const * const*)_buildargv);

	return r;
}

//
// initsystem() -- init SDL systems
//
int initsystem(void)
{
	frameplace = 0;
	return 0;
}

//
// uninitsystem() -- uninit SDL systems
//
void uninitsystem(void)
{
	uninitinput();
	uninitmouse();
	uninittimer();
	shutdownvideo();
}

//
// initputs() -- prints a string to the intitialization window
//
void initputs(const char *str)
{
}

//
// debugprintf() -- prints a debug string to stderr
//
void debugprintf(const char *f, ...)
{
#ifdef DEBUGGINGAIDS
	va_list va;

	va_start(va,f);
	Bvfprintf(stderr, f, va);
	va_end(va);
#endif
}

void ctr_clear_console()
{
    printf("\e[1;1H\e[2J");
}

//
//
// ---------------------------------------
//
// All things Input
//
// ---------------------------------------
//
//

//
// initinput() -- init input system
//
int initinput(void)
{
    inputdevices = 2|4;  // keyboard (1) mouse (2) joystick (4)
    mouseacquired = 0;

    memset(keynames, 0, sizeof(keynames));

    joynumbuttons = 15;
    joynumaxes = 4;

    return 0;
}

//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
	uninitmouse();
}

const char *getkeyname(int num)
{
}

static const char *joynames[15] =
{
    "A", "B", "X", "Y", "LT", "RT", "ZL", "ZR", "START", "SELECT", "DUP", "DDOWN", "DLEFT", "DRIGHT"
};

const char *getjoyname(int what, int num)
{

    static char tmp[64];

    switch (what)
    {
        case 0:  // axis
            if ((unsigned)num > (unsigned)joynumaxes)
                return NULL;
            Bsprintf(tmp, "Axis %d", num);
            return (char *)tmp;

        case 1:  // button
            if ((unsigned)num > (unsigned)joynumbuttons)
                return NULL;
            return joynames[num];

        default: return NULL;
    }
}


unsigned char bgetchar(void)
{
	return;
}

int bkbhit(void)
{
	return false;
}

void bflushchars(void)
{
}


//
// set{key|mouse|joy}presscallback() -- sets a callback which gets notified when keys are pressed
//
void setkeypresscallback(void (*callback)(int, int)) { keypresscallback = callback; }
void setmousepresscallback(void (*callback)(int, int)) { mousepresscallback = callback; }
void setjoypresscallback(void (*callback)(int, int)) { joypresscallback = callback; }

//
// initmouse() -- init mouse input
//
int initmouse(void)
{
	moustat=1;
	return 0;
}

//
// uninitmouse() -- uninit mouse input
//
void uninitmouse(void)
{
	moustat=0;
}

//
// readmousexy() -- return mouse motion information
//
void readmousexy(int *x, int *y)
{
	if (!appactive || !moustat) { *x = *y = 0; return; }

	*x = mousex;
	*y = mousey;
	mousex = mousey = 0;
}

//
// readmousebstatus() -- return mouse button information
//
void readmousebstatus(int *b)
{
}

//
// releaseallbuttons()
//
void releaseallbuttons(void)
{
    joyb = 0;
}


//
//
// ---------------------------------------
//
// All things Timer
// Ken did this
//
// ---------------------------------------
//
//

static u64 timerfreq=0;
static u32 timerlastsample=0;
static u32 timerticspersec=0;
static double msperu64tick = 0;
static void (*usertimercallback)(void) = NULL;

//
// inittimer() -- initialise timer
//
int inittimer(int tickspersecond)
{
    if (timerfreq) return 0;	// already installed

    printf("Initializing timer\n");

    totalclock = 0;
    timerfreq = 268123480.0;
    timerticspersec = tickspersecond;
    timerlastsample = svcGetSystemTick() * timerticspersec / timerfreq;

    usertimercallback = NULL;

    msperu64tick = 1000.0 / (double)268123480.0;

    return 0;
}

//
// uninittimer() -- shut down timer
//
void uninittimer(void)
{
    if (!timerfreq) return;

    timerfreq=0;

    msperu64tick = 0;
}

//
// sampletimer() -- update totalclock
//
void sampletimer(void)
{

    uint64_t i;
    int32_t n;

    if (!timerfreq) return;
    i = svcGetSystemTick();
    n = (int32_t)((i*timerticspersec / timerfreq) - timerlastsample);
    //printf("tick\n");
    if (n <= 0) return;

    totalclock += n;
    timerlastsample += n;

    if (usertimercallback)
        for (; n > 0; n--) usertimercallback();

}

//
// getticks() -- returns a millisecond ticks count
//
unsigned int getticks(void)
{
	return (uint32_t)svcGetSystemTick();
}

//
// getusecticks() -- returns a microsecond ticks count
//
unsigned int getusecticks(void)
{
	return (uint32_t)svcGetSystemTick() * 1000;
}


//
// gettimerfreq() -- returns the number of ticks per second the timer is configured to generate
//
int gettimerfreq(void)
{
	return 268123480.0;
}


//
// installusertimercallback() -- set up a callback function to be called when the timer is fired
//
void (*installusertimercallback(void (*callback)(void)))(void)
{
	void (*oldtimercallback)(void);

	oldtimercallback = usertimercallback;
	usertimercallback = callback;

	return oldtimercallback;
}



//
//
// ---------------------------------------
//
// All things Video
//
// ---------------------------------------
//
//


//
// getvalidmodes() -- figure out what video modes are available
//
static int sortmodes(const struct validmode_t *a, const struct validmode_t *b)
{
	int x;

	if ((x = a->fs   - b->fs)   != 0) return x;
	if ((x = a->bpp  - b->bpp)  != 0) return x;
	if ((x = a->xdim - b->xdim) != 0) return x;
	if ((x = a->ydim - b->ydim) != 0) return x;

	return 0;
}

#define ADDMODE(x,y,c,f,n) if (validmodecnt<MAXVALIDMODES) { \
    validmode[validmodecnt].xdim=x; \
    validmode[validmodecnt].ydim=y; \
    validmode[validmodecnt].bpp=c; \
    validmode[validmodecnt].fs=f; \
    validmode[validmodecnt].extra=n; \
    validmodecnt++; \
}

static char modeschecked=0;
void getvalidmodes(void)
{

    if (modeschecked) return;
    validmodecnt=0;
    ADDMODE(400,240,8,1,-1)
    modeschecked=1;
}


//
// checkvideomode() -- makes sure the video mode passed is legal
//
int checkvideomode(int *x, int *y, int c, int fs, int forced)
{
	int i, nearest=-1, dx, dy, odx=9999, ody=9999;

	getvalidmodes();

	if (c > 8) return -1;

	// fix up the passed resolution values to be multiples of 8
	// and at least 320x200 or at most MAXXDIMxMAXYDIM
	if (*x < 320) *x = 320;
	if (*y < 200) *y = 200;
	if (*x > MAXXDIM) *x = MAXXDIM;
	if (*y > MAXYDIM) *y = MAXYDIM;
	*x &= 0xfffffff8l;

	for (i=0; i<validmodecnt; i++) {
		if (validmode[i].bpp != c) continue;
		if (validmode[i].fs != fs) continue;
		dx = klabs(validmode[i].xdim - *x);
		dy = klabs(validmode[i].ydim - *y);
		if (!(dx | dy)) {   // perfect match
			nearest = i;
			break;
		}
		if ((dx <= odx) && (dy <= ody)) {
			nearest = i;
			odx = dx; ody = dy;
		}
	}

#ifdef ANY_WINDOWED_SIZE
	if (!forced && (fs&1) == 0 && (nearest < 0 || (validmode[nearest].xdim!=*x || validmode[nearest].ydim!=*y)))
		return 0x7fffffffl;
#endif

	if (nearest < 0) {
		// no mode that will match (eg. if no fullscreen modes)
		return -1;
	}

	*x = validmode[nearest].xdim;
	*y = validmode[nearest].ydim;

	return nearest;     // JBF 20031206: Returns the mode number
}


void shutdownvideo(void)
{
    frameplace = 0;
    free(framebuffer);

	return 0;
}

//
// setvideomode() -- set SDL video mode sdlappicon
//
int setvideomode(int x, int y, int c, int fs)
{
	xres = x;
    yres = y;
    bpp =  c;
    fullscreen = fs;
    bytesperline = 400;
    numpages =  1;
    frameplace = 0;
    modechange = 1;
    videomodereset = 0;

    OSD_ResizeDisplay(x, y);
		fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

	return 0;
}


//
// resetvideomode() -- resets the video system
//
void resetvideomode(void)
{
	videomodereset = 1;
	modeschecked = 0;
}


//
// begindrawing() -- locks the framebuffer for drawing
//
void begindrawing(void)
{
    if (offscreenrendering) return;

    frameplace = (intptr_t)framebuffer;

    if(modechange){
        modechange=0;
    }
}


//
// enddrawing() -- unlocks the framebuffer
//
void enddrawing(void)
{
}


//
// showframe() -- update the display
//
void showframe(void)
{
  if (offscreenrendering) return;
  int x,y;

    for(x=0; x<400; x++){
        for(y=0; y<240;y++){
            fb[((x*240) + (239 -y))] = ctrlayer_pal[framebuffer[y*400 + x]];
        }
    }
}


//
// setpalette() -- set palette values
//
int setpalette(int UNUSED(start), int UNUSED(num), unsigned char * UNUSED(dapal))
{
    int32_t i, n;

    if (bpp > 8)
        return 0;

	uint8_t *pal = curpalettefaded;
	uint8_t r, g, b;
	uint16_t *table = ctrlayer_pal;
	for(i=0; i<256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		table[0] = RGB8_to_565(r,g,b);
		table++;
		pal += 4;
	}

    return 0;
}

//
// setgamma
//
int setgamma(float gamma)
{
    return 0;
}


//
//
// ---------------------------------------
//
// Miscellany
//
// ---------------------------------------
//
//


void SetKey(uint32_t keycode, int state){
    if(state)
        joyb |=  1 << keycode;
    else
        joyb &=  ~(1 << keycode);
}


char lastButton = 0;
touchPosition oldtouch, touch;

void handleevents_touch()
{
    if(hidKeysDown() & KEY_TOUCH){
        hidTouchRead(&oldtouch);
    }

    if(hidKeysHeld() & KEY_TOUCH){
        hidTouchRead(&touch);

        mousex += (touch.px - oldtouch.px)*10;
        mousey += (touch.py - oldtouch.py)*10;
        oldtouch = touch;

    }
}

void handleevents_axis()
{
	circlePosition circle;
	hidCircleRead(&circle);

	circlePosition cstick;
	hidCstickRead(&cstick);

    joyaxis[0] = (circle.dx*100);
    joyaxis[1] = (-circle.dy*100);
    joyaxis[2] = (cstick.dx*100);
    joyaxis[3] = (-cstick.dy*100);

//printf("\x1b[1;0H%03d; %03d", circle.dx, circle.dy);
//printf("\x1b[2;0H%03d; %03d", cstick.dx, cstick.dy);
}

void handleevents_buttons(u32 keys, int state){

    if( keys & KEY_A)
        SetKey(0, state);
    if( keys & KEY_B)
        SetKey(1, state);
    if( keys & KEY_X)
        SetKey(2, state);
    if( keys & KEY_Y)
        SetKey(3, state);
    if( keys & KEY_SELECT)
        SetKey(4, state);
    if( keys & KEY_START)
        SetKey(6, state);
    if( keys & KEY_ZL)
        SetKey(7, state);
    if( keys & KEY_ZR)
        SetKey(8, state);
    if( keys & KEY_L)
        SetKey(9, state);
    if( keys & KEY_R)
        SetKey(10, state);
    if( keys & KEY_DUP)
        SetKey(11, state);
    if( keys & KEY_DDOWN) 
        SetKey(12, state);
    if( keys & KEY_DLEFT)
        SetKey(13, state);
    if( keys & KEY_DRIGHT) 
        SetKey(14, state);
}

//
// handleevents() -- process the SDL message queue
//   returns !0 if there was an important event worth checking (like quitting)
//
int handleevents(void)
{
    aptMainLoop();
    svcSleepThread(1);

    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kUp = hidKeysUp();

    if(kDown)
        handleevents_buttons(kDown, 1);
    if(kUp)
        handleevents_buttons(kUp, 0);

    handleevents_axis();
    handleevents_touch();

    sampletimer();

    return 0;
}

int ctr_swkbd(const char *hintText, const char *inText, char *outText)
{
    SwkbdState swkbd;

    char mybuf[16];

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, 15);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetInitialText(&swkbd, inText);
    swkbdSetHintText(&swkbd, hintText);

    SwkbdButton button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));

    if (button == SWKBD_BUTTON_CONFIRM)
    {
        strncpy(outText, mybuf, strlen(mybuf));
        return 0;
    }
    return -1;
}
