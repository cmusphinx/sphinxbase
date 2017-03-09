/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2014 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <ad.h>
#include <assert.h>
#include <pthread.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

static int8 g_IsInitEngine = 0;

struct ad_rec_s{
	SLObjectItf 						m_engineObject;
	SLEngineItf 						m_engineEngine;
	pthread_mutex_t 					m_audioEngineLock;
	SLObjectItf 						m_recorderObject;
	SLRecordItf 						m_recorderRecord;
	SLAndroidSimpleBufferQueueItf		m_recorderBufferQueue;

	short* 								m_pRecorderBuffer;
	unsigned short 						m_recorderSize;
	
	//spin lock is better
	pthread_mutex_t 					m_bufferOperLock;
	short*								m_pBufferTotal;
	unsigned int						m_nBufferLength;
	unsigned int						m_nAlreadySet;
};

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
    ad_rec_t* r = (ad_rec_t*)context;
    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
	pthread_mutex_lock(&r->m_bufferOperLock);
	
	if(r->m_nAlreadySet + r->m_recorderSize <= r->m_nBufferLength){
		memcpy(r->m_pBufferTotal + r->m_nAlreadySet * sizeof(short), r->m_pRecorderBuffer, sizeof(short) * r->m_recorderSize);
		r->m_nAlreadySet += r->m_recorderSize;
	}
	else{
		assert(0);
		fprintf(stderr, "full buffer when record, lost data\n");
	}
	
    SLresult result;
	result = (*r->m_recorderBufferQueue)->Enqueue(r->m_recorderBufferQueue, r->m_pRecorderBuffer, r->m_recorderSize * sizeof(short));
	if (SL_RESULT_SUCCESS != result) {
        fprintf(stderr, "error set record buffer\n");
    }
	pthread_mutex_unlock(&r->m_bufferOperLock);
}

int32 CreateEngine(ad_rec_t* r){
	if(g_IsInitEngine)
		return 0;
	g_IsInitEngine = 1;
	
    // create engine
    SLresult result = slCreateEngine(&r->m_engineObject, 0, NULL, 0, NULL, NULL);
    if(SL_RESULT_SUCCESS != result){
		return -1;
	}
	// realize the engine
    result = (*r->m_engineObject)->Realize(r->m_engineObject, SL_BOOLEAN_FALSE);
	if(SL_RESULT_SUCCESS != result){
		return -2;
	}
	// get the engine interface, which is needed in order to create other objects
	result = (*r->m_engineObject)->GetInterface(r->m_engineObject, SL_IID_ENGINE, &r->m_engineEngine);
	if(SL_RESULT_SUCCESS != result){
		return -3;
	}
	pthread_mutex_init (&r->m_audioEngineLock, NULL);
	return 0;
}

ad_rec_t *ad_open_dev(const char * dev, int32 samples_per_sec)
{
	ad_rec_t * handle = malloc(sizeof(ad_rec_t));

    if (handle == NULL) {
        fprintf(stderr, "%s\n", "failed to allocate memory");
        return NULL;
    }
	CreateEngine(handle);
	
	SLresult result;

    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
	SLDataSource audioSrc = {&loc_dev, NULL};
	
	// configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, samples_per_sec * 1000,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
	SLDataSink audioSnk = {&loc_bq, &format_pcm};
	
	 // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*handle->m_engineEngine)->CreateAudioRecorder(handle->m_engineEngine, &handle->m_recorderObject, &audioSrc, &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) {
        return NULL;
	}
	// realize the audio recorder
    result = (*handle->m_recorderObject)->Realize(handle->m_recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
            return NULL;
    }
	// get the record interface
    result = (*handle->m_recorderObject)->GetInterface(handle->m_recorderObject, SL_IID_RECORD, &handle->m_recorderRecord);
    if (SL_RESULT_SUCCESS != result) {
            return NULL;
    }
	// get the buffer queue interface
    result = (*handle->m_recorderObject)->GetInterface(handle->m_recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &handle->m_recorderBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
            return NULL;
    }
	// register callback on the buffer queue
    result = (*handle->m_recorderBufferQueue)->RegisterCallback(handle->m_recorderBufferQueue, bqRecorderCallback, handle);
    if (SL_RESULT_SUCCESS != result) {
            return NULL;
    }
	pthread_mutex_init(&handle->m_bufferOperLock, NULL);
	
	handle->m_pRecorderBuffer = (short*)malloc(sizeof(short)* samples_per_sec / 5) ;
	handle->m_recorderSize = samples_per_sec / 5;
	
	handle->m_nBufferLength = samples_per_sec * 10;
	handle->m_pBufferTotal = (short*)malloc(sizeof(short)* samples_per_sec * 10) ;
	handle->m_nAlreadySet = 0;
	return handle;
}


ad_rec_t *ad_open_sps(int32 samples_per_sec)
{
    return ad_open_dev(NULL, samples_per_sec);
}

ad_rec_t *ad_open(void)
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}


int32 ad_start_rec(ad_rec_t * r)
{
    if (pthread_mutex_trylock(&r->m_audioEngineLock)) {
        return -1;
    }
    // in case already recording, stop recording and clear buffer queue
    SLresult result = (*r->m_recorderRecord)->SetRecordState(r->m_recorderRecord, SL_RECORDSTATE_STOPPED);
    if(SL_RESULT_SUCCESS != result){
		return -2;
	}
    result = (*r->m_recorderBufferQueue)->Clear(r->m_recorderBufferQueue);
    if(SL_RESULT_SUCCESS != result){
		return -3;
	}

    // the buffer is not valid for playback yet
    r->m_recorderSize = 0;

    // enqueue an empty buffer to be filled by the recorder
    // (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
    result = (*r->m_recorderBufferQueue)->Enqueue(r->m_recorderBufferQueue, r->m_pRecorderBuffer, r->m_recorderSize * sizeof(short));
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
	if(SL_RESULT_SUCCESS != result){
		return -4;
	}

    // start recording
    result = (*r->m_recorderRecord)->SetRecordState(r->m_recorderRecord, SL_RECORDSTATE_RECORDING);
	if(SL_RESULT_SUCCESS != result){
		return -5;
	}
	return 0;
}


int32 ad_stop_rec(ad_rec_t * r)
{
	// in case already recording, stop recording and clear buffer queue
    SLresult result = (*r->m_recorderRecord)->SetRecordState(r->m_recorderRecord, SL_RECORDSTATE_STOPPED);
    if(SL_RESULT_SUCCESS != result){
		return -1;
	}
	result = (*r->m_recorderBufferQueue)->Clear(r->m_recorderBufferQueue);
    if(SL_RESULT_SUCCESS != result){
		return -2;
	}
	pthread_mutex_unlock(&r->m_audioEngineLock);
    return 0;
}


int32 ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
	if(pthread_mutex_trylock(&r->m_bufferOperLock)){
		if(r->m_nAlreadySet == 0){
			return 0;
		}
		pthread_mutex_lock(&r->m_bufferOperLock);
	}
	int32 nReadMaxSize = max > r->m_nAlreadySet ? r->m_nAlreadySet : max;
	memcpy(buf, r->m_pBufferTotal, sizeof(short) * nReadMaxSize);
	r->m_nAlreadySet -= nReadMaxSize;
	if(r->m_nAlreadySet > 0)
		memcpy(r->m_pBufferTotal, r->m_pBufferTotal + nReadMaxSize * sizeof(short), r->m_nAlreadySet * sizeof(short));
    return nReadMaxSize;
}


int32 ad_close(ad_rec_t * r)
{
	// destroy audio recorder object, and invalidate all associated interfaces
    if (r->m_recorderObject != NULL) {
        (*r->m_recorderObject)->Destroy(r->m_recorderObject);
        r->m_recorderObject = NULL;
        r->m_recorderRecord = NULL;
        r->m_recorderBufferQueue = NULL;
	}
	// destroy engine object, and invalidate all associated interfaces
    if (r->m_engineObject != NULL) {
        (*r->m_engineObject)->Destroy(r->m_engineObject);
        r->m_engineObject = NULL;
        r->m_engineEngine = NULL;
	}
	if(r->m_pRecorderBuffer){
		free(r->m_pRecorderBuffer);
	}
	if(r->m_pBufferTotal){
		free(r->m_pBufferTotal);
	}
	
	pthread_mutex_destroy(&r->m_bufferOperLock);
	pthread_mutex_destroy(&r->m_audioEngineLock);
	g_IsInitEngine = 0;
	return 0;
}
