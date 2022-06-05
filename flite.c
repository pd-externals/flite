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



#define DEFAULT_BUFSIZE 256
#define DEFAULT_BUFSTEP 256

/*=====================================================================
 * Structures and Types
 *=====================================================================*/

static const char *flite_description =
  "flite: Text-to-Speech external v" VERSION " \n"
  ;
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
  TEXTFILE = 1,
  SYNTH = 2,
  QUIT = 3, 
} t_thrd_request;

typedef struct _flite
{
  t_object x_obj;                    /* black magic (probably inheritance-related) */
  t_canvas  *x_canvas;
  t_symbol *x_arrayname;             /* arrayname (from '_tabwrite' code in $PD_SRC/d_array.c) */
  char     *textbuf;                 /* text buffer (hack) */
  int      bufsize;                  /* text buffer size */
  char completefilename[MAXPDSTRING];
  cst_voice *voice;
  t_thrd_request x_requestcode;
  pthread_mutex_t x_mutex;
  pthread_cond_t x_requestcondition;
  pthread_t x_tid;
} t_flite;


/*--------------------------------------------------------------------
 * flite_synth : synthesize current text-buffer
 *--------------------------------------------------------------------*/
static void flite_synth(t_flite *x) {
  cst_wave *wave;
  int i,vecsize;
  t_garray *a;
  t_word *vec;

# ifdef FLITE_DEBUG
  debug("flite: got message 'synth'\n");
# endif

  // -- sanity checks
  if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class))) {
    pd_error(x,"flite: no such array '%s'", x->x_arrayname->s_name);
    return;
  }
  if (!x->textbuf) {
    pd_error(x,"flite: attempt to synthesize empty text-buffer!");
    return;
  }

# ifdef FLITE_DEBUG
  debug("flite: flite_text_to_wave()\n");
# endif
  wave = flite_text_to_wave(x->textbuf,x->voice);

  if (!wave) {
    pd_error(x,"flite: synthesis failed for text '%s'", x->textbuf);
    return;
  }

  // -- resample
# ifdef FLITE_DEBUG
  debug("flite: cst_wave_resample()\n");
# endif
  cst_wave_resample(wave,sys_getsr());

  // -- resize & write to our array
# ifdef FLITE_DEBUG
  debug("flite: garray_resize(%d)\n", wave->num_samples);
# endif

  garray_resize(a, wave->num_samples);
  if (!garray_getfloatwords(a, &vecsize, &vec))
    pd_error(x,"flite: bad template for write to array '%s'", x->x_arrayname->s_name);

# ifdef FLITE_DEBUG
  debug("flite: ->write to garray loop<-\n");
# endif

  for (i = 0; i < wave->num_samples; i++) {
    vec->w_float = wave->samples[i]/32767.0;
    vec++;
  }

  // -- outlet synth-done-bang
  outlet_bang(x->x_obj.ob_outlet);

  // -- cleanup
  delete_wave(wave);

  // -- redraw
  garray_redraw(a);
  
  return;
}

/*--------------------------------------------------------------------
 * flite_text : set text-buffer
 *--------------------------------------------------------------------*/
static void flite_text(t_flite *x, MOO_UNUSED t_symbol *s, int argc, t_atom *argv) {
  int i, alen, buffered;
  t_symbol *asym;

// -- ugly: just for now
x->textbuf == NULL;
  
  
  // -- allocate initial text-buffer if required
  if (x->textbuf == NULL) {
    x->bufsize = DEFAULT_BUFSIZE;
    x->textbuf = getbytes(x->bufsize);
  }
  if (x->textbuf == NULL) {
    pd_error(x,"flite: allocation failed for text buffer");
    x->bufsize = 0;
    return;
  }

  // -- convert args to strings
  buffered = 0;
  for (i = 0; i < argc; i++) {
    asym = atom_gensym(argv);
    alen = 1+strlen(asym->s_name);

    // -- reallocate if necessary
    while (buffered + alen > x->bufsize) {
      x->textbuf = resizebytes(x->textbuf,x->bufsize,x->bufsize+DEFAULT_BUFSTEP);
      x->bufsize = x->bufsize+DEFAULT_BUFSTEP;
      if (x->textbuf == NULL) {
    pd_error(x,"flite: allocation failed for text buffer");
    x->bufsize = 0;
    return;
      }
    }
    
    // -- append arg-string to text-buf
    if (i == 0) {
      strcpy(x->textbuf+buffered, asym->s_name);
      buffered += alen-1;
    } else {
      *(x->textbuf+buffered) = ' ';
      strcpy(x->textbuf+buffered+1, asym->s_name);
      buffered += alen;
    }
    
    // -- increment/decrement
    argv++;
  }

#ifdef FLITE_DEBUG
  debug("flite_debug: got text='%s'\n", x->textbuf);
#endif
}


/*--------------------------------------------------------------------
 * flite_list : parse & synthesize text in one swell foop
 *--------------------------------------------------------------------*/
static void flite_list(t_flite *x, t_symbol *s, int argc, t_atom *argv) {
  flite_text(x,s,argc,argv);
  flite_synth(x);
}


/*--------------------------------------------------------------------
 * flite_set : set arrayname
 *--------------------------------------------------------------------*/
static void flite_set(t_flite *x, t_symbol *ary) {
#ifdef FLITE_DEBUG
  debug("flite_set: called with arg='%s'\n", ary->s_name);
#endif
  x->x_arrayname = ary;
}

/*--------------------------------------------------------------------
 * flite_opentextfile : full path of the text file.
 *--------------------------------------------------------------------*/
static void flite_opentextfile(t_flite *x, t_symbol *filename) {
    
  if(filename->s_name[0] == '/')/*make complete path + filename*/
  {
    strcpy(x->completefilename, filename->s_name);
  }
  else if( (((filename->s_name[0] >= 'A') && (filename->s_name[0] <= 'Z')) || \
  ((filename->s_name[0] >= 'a') && (filename->s_name[0] <= 'z'))) && \
  (filename->s_name[1] == ':') && (filename->s_name[2] == '/') )
  {
    strcpy(x->completefilename, filename->s_name);
  }
  else
  {
    strcpy(x->completefilename, canvas_getdir(x->x_canvas)->s_name);
    strcat(x->completefilename, "/");
    strcat(x->completefilename, filename->s_name);
  }
  return;  
}



/*--------------------------------------------------------------------
 * flite_do_textfile : read the textfile and synthesize it.
 *--------------------------------------------------------------------*/
static void flite_do_textfile(t_flite *x) {

  FILE *fp;
  fp = fopen(x->completefilename, "r");
  if(fp <= 0){
    pd_error(x, "[flite]: can't open file: %s", x->completefilename);
    return;
    }
  fseek(fp, 0, SEEK_END);
  int len;
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);  
  x->textbuf = (char *) calloc(len, sizeof(char));
  fread(x->textbuf, 1, len, fp);
  fclose(fp);
  flite_synth(x);  
  x->textbuf = NULL;
  
  return;  
}

/*--------------------------------------------------------------------
 * flite_voice : set the voice for the synthesizer
 *--------------------------------------------------------------------*/
static void flite_voice(t_flite *x, t_symbol *vox) {
#ifdef FLITE_DEBUG
  debug("flite_voice: called with arg='%s'\n", vox->s_name);
#endif

  const char *voxstring;
  voxstring = vox->s_name;
  
  if (!strcmp(voxstring, "awb")) {
    x->voice = register_cmu_us_awb();  
  } 
  else if (!strcmp(voxstring, "kal")) {
    x->voice = register_cmu_us_kal();     
  }
  else if (!strcmp(voxstring, "kal16")) {
    x->voice = register_cmu_us_kal16();   
  }
  else if (!strcmp(voxstring, "rms")) {
    x->voice = register_cmu_us_rms();     
  }
  else if (!strcmp(voxstring, "slt")) {
    x->voice = register_cmu_us_slt();     
  } else {
    pd_error(x,"flite: unknown voice '%s'. Possible voices are: 'awb', 'kal', 'kal16', 'rms' or 'slt'.", voxstring );
    return; 
  }
  return;  
}

/*--------------------------------------------------------------------
 * flite_textfile : read textfile
 *--------------------------------------------------------------------*/
static void flite_textfile(t_flite *x, t_symbol *filename) {

  flite_opentextfile(x, filename);
  flite_do_textfile(x);
}

/*--------------------------------------------------------------------
 * flite_thrd_textfile : threaded read textfile
 *--------------------------------------------------------------------*/
static void flite_thrd_textfile(t_flite *x, t_symbol *filename) {


  pthread_mutex_lock(&x->x_mutex);
  flite_opentextfile(x, filename);
  x->x_requestcode = TEXTFILE;
  pthread_mutex_unlock(&x->x_mutex);
  pthread_cond_signal(&x->x_requestcondition);
    
}

/*--------------------------------------------------------------------
 * flite_thrd_synth : threaded flite_synth
 *--------------------------------------------------------------------*/
static void flite_thrd_synth(t_flite *x) {
    
  pthread_mutex_lock(&x->x_mutex);
  x->x_requestcode = SYNTH;
  //pthread_cond_signal(&x->x_requestcondition);
  pthread_mutex_unlock(&x->x_mutex);
  pthread_cond_signal(&x->x_requestcondition);
    
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
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = IDLE;
# ifdef FLITE_DEBUG
  debug("thread synth\n");
# endif
    flite_synth(x);
    pthread_mutex_unlock(&x->x_mutex);  
    }
    else if (x->x_requestcode == TEXTFILE)
    {
    pthread_mutex_unlock(&x->x_mutex);
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = IDLE;
    flite_do_textfile(x);
    pthread_mutex_unlock(&x->x_mutex);
    }
    else if (x->x_requestcode == QUIT)
    {
    break;
    }
  }
# ifdef FLITE_DEBUG
  debug("thread quit\n");
# endif
  
}

/*--------------------------------------------------------------------
 * constructor / destructor
 *--------------------------------------------------------------------*/
static void *flite_new(t_symbol *ary)
{
  t_flite *x;

  x = (t_flite *)pd_new(flite_class);

  // set initial arrayname
  x->x_arrayname = ary;

  // init textbuf
  x->textbuf = NULL;
  x->bufsize = 0;

  // create bang-on-done outlet
  outlet_new(&x->x_obj, &s_bang);
  
  // default voice  
  x->voice = register_cmu_us_kal16();
  
  x->x_canvas = canvas_getcurrent();
  
  x->x_requestcode = IDLE;
  pthread_mutex_init(&x->x_mutex, 0);
  pthread_cond_init(&x->x_requestcondition, 0);
  pthread_create(&x->x_tid, 0, flite_thread, x);
  
  return (void *)x;
}

static void flite_free(t_flite *x) {
	
  pthread_mutex_lock(&x->x_mutex);
  x->x_requestcode = QUIT;
  pthread_cond_signal(&x->x_requestcondition);
  pthread_mutex_unlock(&x->x_mutex);
  pthread_join(x->x_tid, NULL);
  pthread_cond_destroy(&x->x_requestcondition);
  pthread_mutex_destroy(&x->x_mutex);
  if (x->bufsize && x->textbuf != NULL) {
    freebytes(x->textbuf, x->bufsize);
    x->bufsize = 0;
  }
  
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
  class_addmethod(flite_class, (t_method)flite_textfile,   gensym("textfile"),   A_DEFSYM, 0);
  class_addmethod(flite_class, (t_method)flite_thrd_synth,   gensym("thrd_synth"), 0);
  class_addmethod(flite_class, (t_method)flite_thrd_textfile,   gensym("thrd_textfile"),   A_DEFSYM, 0);
  
  
  // --- help patch
  //class_sethelpsymbol(flite_class, gensym("flite-help.pd")); /* breaks pd-extended help lookup */
}
