/* lslsend.c
*
*
*
*/

#include "m_pd.h"      //pd header file
#include "lsl_c.h"     //LSL header file
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_EVENT_LENGTH 4096
#define DEFAULT_STREAM_NAME "pd_send"
#define DEFAULT_STREAM_TYPE "EEG"
#define DEFAULT_DATA_TYPE "string"
#define DEFAULT_NCHAN 1
#define MAX_ARG_LENGTH 50
#define MAX_DATA_TYPE_LENGTH 32


//TODO: any need to expose the lsl timestamp of event?
static t_class *lslsend_class;

typedef struct _lslsend{
	t_object x_obj;
    
    char eventcode[MAX_EVENT_LENGTH];            /* current event code */
	lsl_streaminfo lsl_info;     //the streaminfo returned by the resolve call 
	char  lsl_stream_name[MAX_ARG_LENGTH]; /* Stream Name */
	char lsl_stream_type[MAX_ARG_LENGTH];
    int lsl_nchan;              /* number of channels in the stream (speacified when creating object) */
	lsl_channel_format_t lsl_channel_format;
    char data_type[MAX_ARG_LENGTH]; /* ui specified data type */

	lsl_outlet lsl_outlet;		/* a stream outlet to push events to */
 	int lsl_errcode;			/* error code (lsl_lost_error or timeouts) */
    double lsl_timestamp;		/* time stamp of the current sample (in sender time) */

} t_lslsend;



void* lslsend_new(t_symbol* s, long argc, t_atom* argv);
void  lslsend_free(t_lslsend* x);
void  lslsend_assist(t_lslsend* x, void* b, long m, long a, char* s);
void  lslsend_bang(t_lslsend *x);
void  lslsend_push(t_lslsend *x, t_symbol *s, t_int argc, t_atom *argv);

void* lslsend_new(t_symbol* s, long argc, t_atom* argv){
    
	t_lslsend *x = (t_lslsend *)pd_new(lslsend_class);

    // get event stream name if specified, else use default
    if (argc>=1 && argv[0].a_type==A_SYMBOL) {
        strncpy(x->lsl_stream_name, atom_getsymbol(&argv[0])->s_name, MAX_ARG_LENGTH);
    } else {
        strncpy(x->lsl_stream_name, DEFAULT_STREAM_NAME, MAX_ARG_LENGTH);
        post(" Using default stream name '%s'",x->lsl_stream_name);
    }
    /* Stream type */
	if (argc>=2 && argv[1].a_type==A_SYMBOL){
	    strncpy(x->lsl_stream_type,atom_getsymbol(&argv[1])->s_name, MAX_ARG_LENGTH);
	} else {
	    strncpy(x->lsl_stream_type, DEFAULT_STREAM_TYPE, MAX_ARG_LENGTH);
	    post(" Using default stream type (%s)",x->lsl_stream_type);
	}
	/* Number of Channels */
	if (argc>=3 && argv[2].a_type==A_FLOAT) {
	    x->lsl_nchan = atom_getint(&argv[2]);
	    if (x->lsl_nchan < 0) {
	        x->lsl_nchan = 1;
	        post("Warning: Must specify at least one channel. Defaulting to one channel.");
	    }
	} else {
	    post(" Using default number of channels (%d).",DEFAULT_NCHAN);
	    x->lsl_nchan = DEFAULT_NCHAN;
	}
	/* Channel format */
	if (argc>=4 && argv[3].a_type==A_SYMBOL) {
	    strncpy(x->data_type, atom_getsymbol(&argv[3])->s_name, MAX_DATA_TYPE_LENGTH);
	} else {
	    strncpy(x->data_type, DEFAULT_DATA_TYPE, MAX_DATA_TYPE_LENGTH);
	    post(" Using default data type (%s)",x->data_type);
	}

	// handle data-type specifics
	if (!strcmp(x->data_type, "string") || !strcmp(x->data_type, "string32")) {
	    x->lsl_channel_format = cft_string;
	} else if (!strcmp(x->data_type, "float") || !strcmp(x->data_type, "float32")) {
	    x->lsl_channel_format = cft_float32;
	} else {
	    post("ERROR: Unsupported data type (%s)",x->data_type);
	    return NULL;
	}

    
    x->eventcode[0]=0; //probably unnecessary--ensure event code is empty to start
			
	post("Creating a stream named '%s'.",x->lsl_stream_name);
	x->lsl_info = lsl_create_streaminfo(x->lsl_stream_name,x->lsl_stream_type,x->lsl_nchan,0,x->lsl_channel_format,"uniqueid12345");
    x->lsl_outlet = lsl_create_outlet(x->lsl_info, 0, 300);
   
    if (x->lsl_outlet) {
        post("Stream created.\n");
    } else {
        post("Problem creating stream. Events won't be sent.");
    }

    inlet_new(&x->x_obj,&x->x_obj.ob_pd,&s_symbol,gensym("push"));
	return x;
}

void lslsend_setup(void) {  
  lslsend_class = class_new(gensym("lslsend"),  
							    (t_newmethod)lslsend_new,  
							    (t_method)lslsend_free,
							    sizeof(t_lslsend),  
							    CLASS_DEFAULT,
							    A_GIMME,
							   	0);   
	class_addbang(lslsend_class, (t_method)lslsend_push);
	class_addlist(lslsend_class, (t_method)lslsend_push);
	// class_addmethod(lslsend_class, (t_method)lslsend_push, gensym("push"), A_GIMME, 0);
}



void lslsend_free(t_lslsend* x){
	/* Do any deallocation needed here. */
    lsl_destroy_outlet(x->lsl_outlet);
}

// send the event code on a bang
void  lslsend_bang(t_lslsend *x) {
	post("niceeeee");
    
    // post("eventcode length= %d",strlen(x->eventcode));
    

}

// store then send first symbol in input out as event string
void  lslsend_push(t_lslsend *x, t_symbol *s, t_int argc, t_atom *argv) {
	int size = argc;
	char *output[size];
	unsigned int lengths[size];
	for (int i=0;i<size;++i){
		output[i] = atom_getsymbol(&argv[i])->s_name;
		lengths[i] = strlen(output[i]);

		// post("%s",output[i]);
	}
	for (int i=0;i<size;++i){
		post("%s", output[i]);
		post("%d", lengths[i]);
	}

	// test it out from here
	lsl_push_sample_str(x->lsl_outlet,&output);
}

