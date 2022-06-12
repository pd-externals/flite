/* -*- Mode: C -*- */
/*=============================================================================*\
 * File: flite.c
 * Author: Bryan Jurish <moocow@ling.uni-potsdam.de>
 * Description: speech synthesis for PD
 *
 *  PD interface to 'flite' C libraries.
 *
 * v0.03 updated by Lucas Cordiviola <lucarda27@hotmail.com>
 *
 *=============================================================================*/


#include <m_pd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


/* black magic for Microsoft's compiler */
#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#ifdef __GNUC__
# define MOO_UNUSED __attribute__((unused))
#else
# define MOO_UNUSED
#endif


#define debug(fmt, args...) fprintf(stderr, fmt, ##args);

#include <math.h>
#include <flite.h>
#include <cst_wave.h>


/*--------------------------------------------------------------------
 * DEBUG
 *--------------------------------------------------------------------*/
//#define FLITE_DEBUG 1
//#undef FLITE_DEBUG


/*--------------------------------------------------------------------
 * Globals
 *--------------------------------------------------------------------*/

cst_voice *register_cmu_us_awb();
cst_voice *register_cmu_us_kal();
cst_voice *register_cmu_us_kal16();
cst_voice *register_cmu_us_rms();
cst_voice *register_cmu_us_slt();

void usenglish_init(cst_voice *v);
cst_lexicon *cmulex_init(void);



#define DEFAULT_BUFSIZE 256
#define DEFAULT_BUFSTEP 256

/*=====================================================================
 * Structures and Types
 *=====================================================================*/

static const char *flite_description =
  "flite: Text-to-Speech external v" VERSION " \n";
  
static const char *thread_waiting = "flite: Wait for the running thread to finish";
  

//static char *flite_acknowledge = "flite: based on code by ";
//static char *flite_version = "flite: PD external v%s by Bryan Jurish";
// "flite: Text-to-Speech external v" VERSION " by Bryan Jurish\n"


/*---------------------------------------------------------------------
 * flite
 *---------------------------------------------------------------------*/
static t_class *flite_class;

typedef enum _thrd_request
{
  IDLE = 0,
  LIST = 1,
  TEXT = 2,
  TEXTFILE = 3,
  SYNTH = 4,
  VOXFILE = 5,
  QUIT = 6, 
} t_thrd_request;

typedef enum _thrd_error
{
  NONE = 0,
  ARRAY = 1,
  BUFFER = 2,
  FAIL = 3,
  INPROGRESS = 4,
} t_thrd_error;


typedef struct _flite
{
  t_object x_obj;                    /* black magic (probably inheritance-related) */
  t_canvas  *x_canvas;
  t_symbol *x_arrayname;             /* arrayname (from '_tabwrite' code in $PD_SRC/d_array.c) */
  char     *x_textbuf;                 /* text buffer (hack) */  
  char x_reqfile[MAXPDSTRING];
  char x_inprogress;
  t_outlet *x_bangout;
  t_clock *x_clock;
  t_thrd_request x_requestcode;
  t_thrd_error x_threaderrormsg;
  pthread_mutex_t x_mutex;
  pthread_cond_t x_requestcondition;
  pthread_t x_tid;
  int x_argc;
  t_atom *x_argv;
  cst_voice *x_voice;
  cst_wave *x_wave;
  int x_vecsize;
  t_garray *x_a;
  t_word *x_vec;
} t_flite;


static void flite_clock_tick(t_flite *x);

/*--------------------------------------------------------------------
 * flite_synth : synthesize current text-buffer
 *--------------------------------------------------------------------*/
static void flite_synth(t_flite *x) {
  
  if (x->x_inprogress) {
    pthread_mutex_lock(&x->x_mutex);
    // do not lock Pd if flite_free() is currently trying to join the thread!
    if (x->x_requestcode != QUIT)
    {
      x->x_requestcode != IDLE && x->x_requestcode != SYNTH  ? sys_lock() : "";
      x->x_threaderrormsg = INPROGRESS;
      x->x_inprogress = 0;
      clock_delay(x->x_clock, 0);
      sys_unlock();
    }
    pthread_mutex_unlock(&x->x_mutex);
    return;
  }

# ifdef FLITE_DEBUG
  debug("flite: got message 'synth'\n");
# endif

  x->x_inprogress = 1;

  // -- sanity checks
  if (!(x->x_a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class))) {
      
    pthread_mutex_lock(&x->x_mutex);
    // do not lock Pd if flite_free() is currently trying to join the thread!
    if (x->x_requestcode != QUIT)
    {
      x->x_requestcode != IDLE ? sys_lock() : "";
      x->x_threaderrormsg = ARRAY;
      x->x_inprogress = 0;
      clock_delay(x->x_clock, 0);
      sys_unlock();
    }
    pthread_mutex_unlock(&x->x_mutex);
    return;
  }
  if (!x->x_textbuf) {

    pthread_mutex_lock(&x->x_mutex);
    // do not lock Pd if flite_free() is currently trying to join the thread!
    if (x->x_requestcode != QUIT)
    {
      x->x_requestcode != IDLE ? sys_lock() : "";
      x->x_threaderrormsg = BUFFER;
      x->x_inprogress = 0;
      clock_delay(x->x_clock, 0);
      sys_unlock();
    }
    pthread_mutex_unlock(&x->x_mutex);
    return;
  }

# ifdef FLITE_DEBUG
  debug("flite: flite_text_to_wave()\n");
# endif
  x->x_wave = flite_text_to_wave(x->x_textbuf, x->x_voice);

  if (!x->x_wave) {

    pthread_mutex_lock(&x->x_mutex);
    // do not lock Pd if flite_free() is currently trying to join the thread!
    if (x->x_requestcode != QUIT)
    {
      x->x_requestcode != IDLE ? sys_lock() : "";
      x->x_threaderrormsg = FAIL;
      x->x_inprogress = 0;
      clock_delay(x->x_clock, 0);
      sys_unlock();
    }
    pthread_mutex_unlock(&x->x_mutex);
    return;
  }

  // -- resample
# ifdef FLITE_DEBUG
  debug("flite: cst_wave_resample()\n");
# endif
  cst_wave_resample(x->x_wave, sys_getsr());
  

  
  // -- sanity checks again for the thread if the patch has been closed
  if (!(x->x_a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class))) {
    x->x_inprogress = 0;
    x->x_threaderrormsg = FAIL; // this is need but why?
    return;
  }
  
  
  pthread_mutex_lock(&x->x_mutex);
  // do not lock Pd if flite_free() is currently trying to join the thread!
  if (x->x_requestcode != QUIT)
# ifdef FLITE_DEBUG
  debug("what\n");
# endif
  {
    x->x_requestcode != IDLE ? sys_lock() : "";
    x->x_threaderrormsg = NONE;
    clock_delay(x->x_clock, 0);
    sys_unlock();
  }
  pthread_mutex_unlock(&x->x_mutex);
 
  return;
}

/*--------------------------------------------------------------------
 * flite_clock_tick : clock
 *--------------------------------------------------------------------*/

static void flite_clock_tick(t_flite *x)
{
   
  if (x->x_threaderrormsg == NONE) {
  
   /*----------- <fooblock> : if I place all this "fooblock" in the clock function 
    *------------------------------I get audio pops even in not graphical arrays */
    int i;
 
    // -- resize & write to our array
# ifdef FLITE_DEBUG
  debug("flite: garray_resize(%d)\n", x->x_wave->num_samples);
# endif 
    garray_resize_long(x->x_a, (long) x->x_wave->num_samples);  
    if (!garray_getfloatwords(x->x_a, &x->x_vecsize, &x->x_vec))
      pd_error(x,"flite: bad template for write to array '%s'", x->x_arrayname->s_name);
# ifdef FLITE_DEBUG
  debug("flite: ->write to garray loop<-\n");
# endif
    for (i = 0; i < x->x_wave->num_samples; i++) {
      x->x_vec->w_float = x->x_wave->samples[i]/32767.0;
      x->x_vec++;
    }   
    // -- cleanup
    delete_wave(x->x_wave);
    // -- redraw
    garray_redraw(x->x_a);
    x->x_inprogress = 0;  
    
    /*------------- </ fooblock> */
    
    outlet_bang(x->x_bangout);
    
  } else if (x->x_threaderrormsg == ARRAY) { 
  
    pd_error(x,"flite: no such array '%s'", x->x_arrayname->s_name);  
    
  } else if (x->x_threaderrormsg == BUFFER) {
      
    pd_error(x,"flite: attempt to synthesize empty text-buffer!");
    
  } else if (x->x_threaderrormsg == FAIL) {
      
    pd_error(x,"flite: synthesis failed for text '%s'", x->x_textbuf);

  } else if (x->x_threaderrormsg == INPROGRESS) {
      
    pd_error(x,"%s", thread_waiting);
  }
  return;
}

/*--------------------------------------------------------------------
 * flite_do_textbuffer : text-buffer
 *--------------------------------------------------------------------*/
static void flite_do_textbuffer(t_flite *x) {
    
  char *buf;
  int length;
  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }
  x->x_inprogress = 1;  
  t_binbuf*bbuf = binbuf_new();
  binbuf_add(bbuf, x->x_argc, x->x_argv);
  binbuf_gettext(bbuf, &buf, &length);
  binbuf_free(bbuf);
  x->x_textbuf = (char *) calloc(length+1, sizeof(char)); 
  memcpy(x->x_textbuf, buf, length);  
  freebytes(buf, length+1);
  x->x_inprogress = 0;
  
#ifdef FLITE_DEBUG
  debug("flite_debug: got text='%s'\n", x->x_textbuf);
#endif
  return;
    
}


/*--------------------------------------------------------------------
 * flite_text : set text-buffer
 *--------------------------------------------------------------------*/
static void flite_text(t_flite *x, MOO_UNUSED t_symbol *s, int argc, t_atom *argv) {

 if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }   
  x->x_argc = argc;
  x->x_argv = argv;
  flite_do_textbuffer(x);
  return;
}


/*--------------------------------------------------------------------
 * flite_list : parse & synthesize text in one swell foop
 *--------------------------------------------------------------------*/
static void flite_list(t_flite *x, t_symbol *s, int argc, t_atom *argv) {
 
 if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  } 
  flite_text(x,s,argc,argv);
  flite_synth(x);
  return;
}


/*--------------------------------------------------------------------
 * flite_set : set arrayname
 *--------------------------------------------------------------------*/
static void flite_set(t_flite *x, t_symbol *ary) {

  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  } 
#ifdef FLITE_DEBUG
  debug("flite_set: called with arg='%s'\n", ary->s_name);
#endif
  x->x_arrayname = ary;
  return;
}


/*--------------------------------------------------------------------
 * flite_filex : get the full path of the file if it exist 
 * and place it's full path on the struct.
 *--------------------------------------------------------------------*/
static int flite_filex(t_flite *x, t_symbol *name) {
    
  char completefilename[MAXPDSTRING];

  const char* filename = name->s_name;
  char realdir[MAXPDSTRING], *realname = NULL;
  int fd;
  fd = canvas_open(x->x_canvas, filename, "", realdir, &realname, MAXPDSTRING, 0);
      if(fd < 0){
          pd_error(x, "[flite]: can't find file: %s", filename);
          x->x_inprogress = 0;
          return 0;
        }

  strcpy(completefilename, realdir);
  strcat(completefilename, "/");
  strcat(completefilename, realname);
  strcpy(x->x_reqfile, completefilename);
  return 1;
}

/*--------------------------------------------------------------------
 * flite_voice_file : check for the voice file and signal the thread to open it
 *--------------------------------------------------------------------*/
static void flite_voice_file(t_flite *x, t_symbol *filename) {
    
  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  } 
    
  if(!flite_filex(x, filename)) {
    return;
  }
  
  pthread_mutex_lock(&x->x_mutex);
  x->x_requestcode = VOXFILE;
  pthread_mutex_unlock(&x->x_mutex);
  pthread_cond_signal(&x->x_requestcondition);
  return;
}

/*--------------------------------------------------------------------
 * flite_thrd_voice_file : open the voice file in the thread
 *--------------------------------------------------------------------*/
static void flite_thrd_voice_file(t_flite *x) {

  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }    
  
#ifdef FLITE_DEBUG
  debug("flite_voice: called with arg='%s'\n", x->x_reqfile);
#endif
  x->x_inprogress = 1;
  flite_add_lang("eng",usenglish_init,cmulex_init);
  flite_add_lang("usenglish",usenglish_init,cmulex_init);
  x->x_voice = flite_voice_load(x->x_reqfile);
  x->x_inprogress = 0;
}

/*--------------------------------------------------------------------
 * flite_voice : set the voice for the synthesizer
 *--------------------------------------------------------------------*/
static void flite_voice(t_flite *x, t_symbol *vox) {

  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }

#ifdef FLITE_DEBUG
  debug("flite_voice: called with arg='%s'\n", vox->s_name);
#endif

  const char *voxstring;
  voxstring = vox->s_name;
  
  if (!strcmp(voxstring, "awb")) {
    x->x_voice = register_cmu_us_awb();  
  } 
  else if (!strcmp(voxstring, "kal")) {
    x->x_voice = register_cmu_us_kal();     
  }
  else if (!strcmp(voxstring, "kal16")) {
    x->x_voice = register_cmu_us_kal16();   
  }
  else if (!strcmp(voxstring, "rms")) {
    x->x_voice = register_cmu_us_rms();     
  }
  else if (!strcmp(voxstring, "slt")) {
    x->x_voice = register_cmu_us_slt();     
  } else {
    pd_error(x,"flite: unknown voice '%s'. Possible voices are: 'awb', 'kal', 'kal16', 'rms' or 'slt'.", voxstring );
    return; 
  }
  return;  
}

/*--------------------------------------------------------------------
 * flite_do_textfile : read the textfile and synthesize it.
 *--------------------------------------------------------------------*/
static void flite_do_textfile(t_flite *x) {
    
  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }
  x->x_inprogress = 1;
  FILE *fp;
  fp = fopen(x->x_reqfile, "r");
  fseek(fp, 0, SEEK_END);
  int len;
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);  
  x->x_textbuf = (char *) calloc(len+1, sizeof(char));
  fread(x->x_textbuf, 1, len, fp);
  fclose(fp);
  x->x_inprogress = 0;
  flite_synth(x);    
  return;  
}

/*--------------------------------------------------------------------
 * flite_textfile : read textfile
 *--------------------------------------------------------------------*/
static void flite_textfile(t_flite *x, t_symbol *filename) {

  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }
  if(!flite_filex(x, filename)) {
    return;
  }
  flite_do_textfile(x);
  return;
}


/*--------------------------------------------------------------------
 * flite_thrd_textfile : threaded read textfile
 *--------------------------------------------------------------------*/
static void flite_thrd_textfile(t_flite *x, t_symbol *filename) {
 
  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }

  if(!flite_filex(x, filename)) {
    //pthread_mutex_unlock(&x->x_mutex);
    return;
  }
  
  pthread_mutex_lock(&x->x_mutex);
  x->x_requestcode = TEXTFILE;
  pthread_mutex_unlock(&x->x_mutex);
  pthread_cond_signal(&x->x_requestcondition);
  return;  
}

/*--------------------------------------------------------------------
 * flite_thrd_synth : threaded flite_synth
 *--------------------------------------------------------------------*/
static void flite_thrd_synth(t_flite *x) {
  
  if (x->x_inprogress) {
    pd_error(x,"%s", thread_waiting);
    return;
  }
  
  pthread_mutex_lock(&x->x_mutex);
  x->x_requestcode = SYNTH;
  pthread_mutex_unlock(&x->x_mutex);
  pthread_cond_signal(&x->x_requestcondition);
  return;  
}

/*--------------------------------------------------------------------
 * flite_thread : thread
 *--------------------------------------------------------------------*/
static void flite_thread(t_flite *x) {
    
  while (1) {
    pthread_mutex_lock(&x->x_mutex);
    while (x->x_requestcode == IDLE) {
# ifdef FLITE_DEBUG
  debug("pthread_cond_wait(\n");
# endif 
      pthread_cond_wait(&x->x_requestcondition, &x->x_mutex);  
    } 
    if (x->x_requestcode == SYNTH)
    {
      pthread_mutex_unlock(&x->x_mutex);    
# ifdef FLITE_DEBUG
  debug("thread synth\n");
# endif
      flite_synth(x);
      pthread_mutex_lock(&x->x_mutex);
      if (x->x_requestcode == SYNTH)
          x->x_requestcode = IDLE;
      pthread_mutex_unlock(&x->x_mutex);
    }
    else if (x->x_requestcode == TEXTFILE)
    {
      pthread_mutex_unlock(&x->x_mutex);
      flite_do_textfile(x);
      pthread_mutex_lock(&x->x_mutex);
      if (x->x_requestcode == TEXTFILE)
          x->x_requestcode = IDLE;
      pthread_mutex_unlock(&x->x_mutex);
    }
    else if (x->x_requestcode == VOXFILE)
    {
      pthread_mutex_unlock(&x->x_mutex);
      flite_thrd_voice_file(x);
      pthread_mutex_lock(&x->x_mutex);
      if (x->x_requestcode == VOXFILE)
          x->x_requestcode = IDLE;
      pthread_mutex_unlock(&x->x_mutex);
    }
    else if (x->x_requestcode == QUIT)
    {
      pthread_mutex_unlock(&x->x_mutex);
      break;
    }
  }
# ifdef FLITE_DEBUG
  debug("thread quit\n");
# endif
  return;
}

/*--------------------------------------------------------------------
 * constructor / destructor
 *--------------------------------------------------------------------*/
static void *flite_new(t_symbol *ary)
{
  t_flite *x;

  x = (t_flite *)pd_new(flite_class);
  
  x->x_clock = clock_new(x, (t_method)flite_clock_tick);
  
  // set initial arrayname
  x->x_arrayname = ary;

  // init x_textbuf
  x->x_textbuf = NULL;

  // create bang-on-done outlet
  x->x_bangout = outlet_new(&x->x_obj, &s_bang);
  
  // default voice  
  x->x_voice = register_cmu_us_kal16();
  
  x->x_canvas = canvas_getcurrent();  
  x->x_inprogress = 0;  
  x->x_requestcode = IDLE;
  pthread_mutex_init(&x->x_mutex, 0);
  pthread_cond_init(&x->x_requestcondition, 0);
  pthread_create(&x->x_tid, 0, flite_thread, x);
  
  return (void *)x;
}

static void flite_free(t_flite *x) {
    
  while(x->x_inprogress) {
  sleep(1);
  }
    
# ifdef FLITE_DEBUG
  debug("free\n");
# endif
    
  pthread_mutex_lock(&x->x_mutex);
  x->x_requestcode = QUIT;
  pthread_mutex_unlock(&x->x_mutex);
  pthread_cond_signal(&x->x_requestcondition);
  pthread_join(x->x_tid, NULL);
  pthread_cond_destroy(&x->x_requestcondition);
  pthread_mutex_destroy(&x->x_mutex);
  free(x->x_textbuf);
  clock_free(x->x_clock);
}

/*--------------------------------------------------------------------
 * setup
 *--------------------------------------------------------------------*/
void flite_setup(void) {
  post("");
  post(flite_description);
  post("");

  // --- setup synth
  flite_init();

  
  // --- register class
  flite_class = class_new(gensym("flite"),
              (t_newmethod)flite_new,  // newmethod
              (t_method)flite_free,    // freemethod
              sizeof(t_flite),         // size
              CLASS_DEFAULT,           // flags
              A_DEFSYM,                // arg1: table-name
              0);

  // --- class methods
  class_addlist(flite_class, flite_list);
  class_addmethod(flite_class, (t_method)flite_set,   gensym("set"),   A_DEFSYM, 0);
  class_addmethod(flite_class, (t_method)flite_text,  gensym("text"),  A_GIMME, 0);
  class_addmethod(flite_class, (t_method)flite_synth, gensym("synth"), 0);
  class_addmethod(flite_class, (t_method)flite_voice,   gensym("voice"),   A_DEFSYM, 0);
  class_addmethod(flite_class, (t_method)flite_voice_file,   gensym("voice_file"),   A_DEFSYM, 0);
  class_addmethod(flite_class, (t_method)flite_textfile,   gensym("textfile"),   A_DEFSYM, 0);
  class_addmethod(flite_class, (t_method)flite_thrd_synth,   gensym("thrd_synth"), 0);
  class_addmethod(flite_class, (t_method)flite_thrd_textfile,   gensym("thrd_textfile"),   A_DEFSYM, 0);
  
  // --- help patch
  //class_sethelpsymbol(flite_class, gensym("flite-help.pd")); /* breaks pd-extended help lookup */
}
