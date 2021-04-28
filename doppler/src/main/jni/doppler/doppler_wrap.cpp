#include <jni.h>
#include <stdlib.h> // for NULL
#include <assert.h>

#include <android/log.h>


extern "C" {
#include "dopplerpro.h"


jmethodID nativeCallBackGetLen = NULL; //初始化时获取buf长度的接口
jmethodID nativeCallBackGetData = NULL; //获取数据的接口
const char* const gClassName = "com/jasperlu/doppler/DopplerCVer";
jclass gClass = NULL;
JavaVM* gVm = NULL;


int callBackFuncGetLen(int needDetach) {
	JNIEnv *env = NULL;
    int len = 0;
    //__android_log_print(ANDROID_LOG_INFO, "zj", "callBackFuncGetLen1 is %d", needDetach);
    int status = gVm->GetEnv((void **)&env, JNI_VERSION_1_4);
    if(status < 0) {
        status = gVm->AttachCurrentThread(&env, NULL);
        if(status < 0) {
            return 0;
        }
    }
    //__android_log_print(ANDROID_LOG_INFO, "zj", "callBackFuncGetLen2 is %d", needDetach);

    if (env != NULL && gClass !=NULL && nativeCallBackGetLen!=NULL){
    	len = env->CallStaticIntMethod(gClass, nativeCallBackGetLen);
		//__android_log_print(ANDROID_LOG_INFO, "zj", "callBackFuncGetLen3 is %d", needDetach);
    }

    if (needDetach)
	    gVm->DetachCurrentThread();

    return len;
}

int callBackFuncGetData(int needDetach, short *buf, int bufsize) {
	JNIEnv *env = NULL;
    int len = 0;
    //__android_log_print(ANDROID_LOG_INFO, "zj", "callBackFuncGetData1 is %d", needDetach);
    int status = gVm->GetEnv((void **)&env, JNI_VERSION_1_4);
    if(status < 0) {
        status = gVm->AttachCurrentThread(&env, NULL);
        if(status < 0) {
            return 0;
        }
    }
    //__android_log_print(ANDROID_LOG_INFO, "zj", "callBackFuncGetData2 is %d", needDetach);

    if (env != NULL && gClass !=NULL && nativeCallBackGetData!=NULL){
    	jobject obj_shortbuf = env->CallStaticObjectMethod(gClass, nativeCallBackGetData, bufsize);

        if (!obj_shortbuf) {
    	    if (needDetach) {
                gVm->DetachCurrentThread();
    	    }
	    
        	return 0;
        }

        jclass cls_shortbuf = env->GetObjectClass(obj_shortbuf);
        jmethodID mtd_array = env->GetMethodID(cls_shortbuf, "array", "()[S");
        jshortArray array_ = (jshortArray) env->CallObjectMethod(obj_shortbuf, mtd_array);
        jshort *array = env->GetShortArrayElements(array_, NULL);

        jmethodID mtd_capacity = env->GetMethodID(cls_shortbuf, "capacity", "()I");
        jint capacity = env->CallIntMethod(obj_shortbuf, mtd_capacity);

        if (capacity > 0) {
        	len = capacity;
        	if (capacity < bufsize) {
        		memcpy(buf, array, capacity*sizeof(short));
        	} else {
        		memcpy(buf, array, bufsize*sizeof(short));
        	}
        }
        env->ReleaseShortArrayElements(array_, array, 0);
		//__android_log_print(ANDROID_LOG_INFO, "zj", "callBackFuncGetData3 is %d, %d, %d", needDetach, bufsize, capacity);
    }

    if (needDetach)
	    gVm->DetachCurrentThread();

    return len;
}

jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	// Only called once.
	gVm = vm;
	jclass clazz;
	JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }

	clazz = env->FindClass(gClassName);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "JNI_OnLoad is %d", clazz);
	if (clazz == NULL) {
		return -1;
	}
	gClass = clazz;
	
	if (nativeCallBackGetLen == NULL) {
		nativeCallBackGetLen = env->GetStaticMethodID(clazz, "GetBufLenFromNative","()I");
	}

	if (nativeCallBackGetData == NULL) {
		nativeCallBackGetData = env->GetStaticMethodID(clazz, "getBufDataFromNative","(I)Ljava/nio/ShortBuffer;"); //getBufDataFromNative
	}

	return JNI_VERSION_1_4;
}


JNIEXPORT jlong JNICALL
Java_com_jasperlu_doppler_DopplerCVer_nativeCreateInstance(JNIEnv *env,
		jclass jclazz) {
	DopplerPro *InstHandler = NULL;
	__android_log_print(ANDROID_LOG_INFO, "zj", "Java_com_jasperlu_doppler_DopplerCVer_nativeCreateInstance");

	InstHandler = createDopplerPro();
	if (InstHandler == NULL) {
		return -1;
	} else {
        SetCallback(InstHandler, callBackFuncGetLen);
        SetCallbackGetData(InstHandler, callBackFuncGetData);
	    return ((long) InstHandler); //returns the pointer which points to created AECM instance to JAVA layer.
	}
}

JNIEXPORT jint JNICALL 
Java_com_jasperlu_doppler_DopplerCVer_nativeStart(JNIEnv *env, 
	jclass jclazz, jlong Handler) {
	DopplerPro *Inst = (DopplerPro *) Handler;

	__android_log_print(ANDROID_LOG_INFO, "zj", "Java_com_jasperlu_doppler_DopplerCVer_nativeStart  %d", Inst);
	if (Inst == NULL)
		return -1;
	int ret = startPro(Inst);
    return 0;
}

JNIEXPORT jint JNICALL 
Java_com_jasperlu_doppler_DopplerCVer_nativeStop(
		JNIEnv *env, jclass jclazz, jlong Handler) {
	DopplerPro *Inst = (DopplerPro *) Handler;
	if (Inst == NULL)
		return -1;
	int ret = stopPro(Inst);
    return 0;
}


}