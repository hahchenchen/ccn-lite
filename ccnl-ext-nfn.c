/*
 * @f ccn-lite-relay.c
 * @b user space CCN relay
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2014-02-06 <christopher.scherb@unibas.ch>created 
 */

#ifndef CCNL_EXT_NFN_H
#define CCNL_EXT_NFN_H

#include "ccnl-core.h"
#include "krivine.c"
#include "krivine-common.c"


static int
ccnl_nfn_count_required_thunks(char *str)
{
    int num = 0;
    char *tok;
    tok = str;
    while((tok = strstr(tok, "call")) != NULL ){
        tok += 4;
        ++num;
    }
    DEBUGMSG(99, "Number of required Thunks is: %d\n", num);
    return num;
}

static void
ccnl_nfn_remove_thunk_from_prefix(struct ccnl_prefix_s *prefix){
    prefix->comp[prefix->compcnt-2] =  prefix->comp[prefix->compcnt-1];
    prefix->complen[prefix->compcnt-2] =  prefix->complen[prefix->compcnt-1];
    --prefix->compcnt;
}


void * 
ccnl_nfn_thread(void *arg)
{
    DEBUGMSG(49, "ccnl_nfn_thread()\n");
    struct thread_parameter_s *t = ((struct thread_parameter_s*)arg);
    
    struct ccnl_relay_s *ccnl = t->ccnl;
    struct ccnl_buf_s *orig = t->orig;
    struct ccnl_prefix_s *prefix = t->prefix;
    struct ccnl_face_s *from = t->from;
    struct thread_s *thread = t->thread;
    int thunk_request = 0;
    int num_of_thunks = 0;
   
    if(!memcmp(prefix->comp[prefix->compcnt-2], "THUNK", 5))
    {
        thunk_request = 1;
    }
    char str[CCNL_MAX_PACKET_SIZE];
    int i, len = 0;
        
    
    //put packet together
    len = sprintf(str, "%s", prefix->comp[prefix->compcnt-2-thunk_request]);
    if(prefix->compcnt > 2 + thunk_request){
        len += sprintf(str + len, " ");
    }
    for(i = 0; i < prefix->compcnt-2-thunk_request; ++i){
        len += sprintf(str + len, "/%s", prefix->comp[i]);
    }
    
    DEBUGMSG(99, "%s\n", str);
    //search for result here... if found return...
    if(thunk_request){
        num_of_thunks = ccnl_nfn_count_required_thunks(str);
    }
    char *res = Krivine_reduction(ccnl, str, thunk_request, 
            &num_of_thunks, thread);
    //stores result if computed      
    if(res){
        DEBUGMSG(2,"Computation finshed: %s\n", res);
        if(thunk_request){      
            ccnl_nfn_remove_thunk_from_prefix(thread->prefix);
        }
        struct ccnl_content_s *c = create_content_object(ccnl, thread->prefix, res, strlen(res));
            
        c->flags = CCNL_CONTENT_FLAGS_STATIC;
        if(!thunk_request)ccnl_content_serve_pending(ccnl,c);
        ccnl_content_add2cache(ccnl, c);
        
    }
    
    //TODO: check if really necessary
    /*if(thunk_request)
    {
       ccnl_nfn_delete_prefix(prefix);
    }*/
    struct thread_s *thread1 = main_thread;
    pthread_cond_broadcast(&thread1->cond);
    pthread_exit ((void *) 0);
    return 0;
}

void 
ccnl_nfn_send_signal(int threadid){
    DEBUGMSG(49, "ccnl_nfn_send_signal()\n");
    struct thread_s *thread = threads[threadid];
    pthread_cond_broadcast(&thread->cond);
    struct thread_s *thread1 = main_thread;
    pthread_cond_wait(&(thread1->cond), &thread1->mutex);
}

int
ccnl_nfn_thunk_already_computing(struct ccnl_prefix_s *prefix)
{
    DEBUGMSG(49, "ccnl_nfn_thunk_already_computing()\n");
    int i = 0;
    for(i = 0; i < -threadid; ++i){
        struct ccnl_prefix_s *copy;
        struct thread_s *thread = threads[i];
        if(!thread) continue;
        ccnl_nfn_copy_prefix(thread->prefix ,&copy);
        ccnl_nfn_remove_thunk_from_prefix(copy);
        if(!ccnl_prefix_cmp(copy, NULL, prefix, CMP_EXACT)){
             return 1;
        }
        
    }
    return 0;
}

int 
ccnl_nfn(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	  struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    DEBUGMSG(49, "ccnl_nfn(%p, %p, %p, %p)\n", ccnl, orig, prefix, from); 
    
    if(ccnl_nfn_thunk_already_computing(prefix)){
        DEBUGMSG(9, "Computation for this interest is already running\n");
        return -1;
    }
    struct ccnl_prefix_s *original_prefix;
    ccnl_nfn_copy_prefix(prefix, &original_prefix);
    struct thread_s *thread = new_thread(threadid, original_prefix);
    struct thread_parameter_s *arg = malloc(sizeof(struct thread_parameter_s *));
    char *h = malloc(10);
    arg->ccnl = ccnl;
    arg->orig = orig;
    arg->prefix = prefix;
    arg->from = from;
    arg->thread = thread;
    
    int threadpos = -threadid;
    threads[threadpos] = thread;
    --threadid;
    DEBUGMSG(99, "New Thread with threadid %d\n", -arg->thread->id);
    pthread_create(&thread->thread, NULL, ccnl_nfn_thread, (void *)arg);
    struct thread_s *thread1 = main_thread;
    pthread_cond_wait(&(thread1->cond), &thread1->mutex);
    return 0;
}

#endif //CCNL_EXT_NFN_H