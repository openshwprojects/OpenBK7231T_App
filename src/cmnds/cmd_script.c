
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../obk_config.h"
#include "../driver/drv_public.h"
#include <ctype.h>
#include "cmd_local.h"

/*
Example 1:


again:
	echo "Step 1"
	setChannel 1 0
	echo "Step 2"
	delay_s 2
	echo "Step 3"
	setChannel 1 1
	echo "Step 4"
	delay_s 2
	goto again

Example 2:


addEventHandler OnClick 8 startScript myScript2.txt::label1
addEventHandler OnClick 9 startScript myScript2.txt::label2

label1:
	setChannel 1 0
	delay_s 2
	setChannel 1 1
	delay_s 2
	exit;
label2:
	setChannel 1 0
	delay_s 2
	setChannel 1 1
	delay_s 2
	exit;



*/

typedef struct scriptFile_s {
	char *fname;
	char *data;

	struct scriptFile_s *next;
} scriptFile_t;

typedef struct scriptInstance_s {
	scriptFile_t *curFile;
	const char *curLine;
	int currentDelayMS;

	struct scriptInstance_s *next;
} scriptInstance_t;

#define MAX_SCRIPT_LINE 512

char *g_scrBuffer = 0;
int svm_deltaMS;
scriptFile_t *g_scriptFiles = 0;
scriptInstance_t *g_scriptThreads = 0;
scriptInstance_t *g_activeThread = 0;

scriptInstance_t *SVM_RegisterThread() {
	scriptInstance_t *r;

	r = g_scriptThreads;

	while(r) {
		if(r->curLine == 0)
			return r;
		r = r->next;
	}
	r = malloc(sizeof(scriptInstance_t));
	memset(r,0,sizeof(scriptInstance_t));
	r->next = g_scriptThreads;
	g_scriptThreads = r;
	return r;
}

scriptFile_t *SVM_RegisterFile(const char *fname) {
	scriptFile_t *r;

	r = g_scriptFiles;

	while(r) {
		if(!stricmp(fname,r->fname))
			return r;
		r = r->next;
	}
	r = malloc(sizeof(scriptFile_t));
	memset(r,0,sizeof(scriptFile_t));
	r->fname = test_strdup(fname);
	r->data = LFS_ReadFile(fname);
	r->next = g_scriptFiles;
	g_scriptFiles = r;
	return r;
}
const char *SVM_SkipWS(const char *p) {
	// skip also whitespaces
	while(*p == ' ' || *p == '\r' || *p == '\t') {
		p++;
	}
	return p;
}
const char *SVM_SkipLine(const char *p) {
	while(*p) {
		if(*p == '\n') {
			p++;
			return p;
		}
		p++;
	}
	return p;
}

const char *SVM_FindLabel(const char *text, const char *label) {
	int labLen;

	if(label == 0)
		return text;

	labLen = strlen(label);

	while(*text) {
		text = SVM_SkipWS(text); 
		if(strncmp(text,label,labLen) == 0 && text[labLen] == ':') {
			return text;
		}
		if(text[0] == '/' && text[1] == '/') {
			text = SVM_SkipLine(text); 
			text = SVM_SkipWS(text); 
		} else {
			text = SVM_SkipLine(text); 
			text = SVM_SkipWS(text); 
		}
	}
	return text;
}
void SVM_RunThread(scriptInstance_t *t) {
	int maxLoops = 10;
	int loop = 0;
	const char *start, *end;
	int len;

	while(loop++ < maxLoops) {
		if(t->curLine == 0) {
			t->curLine = 0;
			t->curFile = 0;
			return;
		}
		if(t->curLine[0] == 0) {
			t->curLine = 0;
			t->curFile = 0;
			return;
		}
		if(t->curLine[0] == '/' && t->curLine[1] == '/') {
			t->curLine = SVM_SkipLine(t->curLine); 
			t->curLine = SVM_SkipWS(t->curLine); 
		} else {
			start = t->curLine;
			end = SVM_SkipLine(start);
			t->curLine = SVM_SkipWS(end); 

			len = end - start;
			if(len >= MAX_SCRIPT_LINE) {
				len = MAX_SCRIPT_LINE-1;
			}
			strncpy(g_scrBuffer,start,len);

			CMD_ExecuteCommand(g_scrBuffer,0);
		}
	}
}

void SVM_RunThreads(int deltaMS) {
	svm_deltaMS = deltaMS;

	if(g_scrBuffer == 0) {
		g_scrBuffer = malloc(MAX_SCRIPT_LINE);
	}

	g_activeThread = g_scriptThreads;
	while(g_activeThread) {
		SVM_RunThread(g_activeThread);
		g_activeThread = g_activeThread->next;
	}
}
void SVM_GoTo(scriptInstance_t *th, const char *fname, const char *label) {
	scriptFile_t *f;

	f = SVM_RegisterFile(fname);
	if(f == 0) {

		return;
	}
	if(f->data == 0) {

		return;
	}
	if(th == 0) {

		return;
	}
	th->curFile = f;
	th->curLine = SVM_FindLabel(f->data,label);

	return;
}
void SVM_GoToLocal(scriptInstance_t *th, const char *label) {

	if(th == 0) {

		return;
	}
	th->curLine = SVM_FindLabel(th->curFile->data,label);

	return;
}
void SVM_StartScript(const char *fname, const char *label) {
	scriptFile_t *f;
	scriptInstance_t *th;

	f = SVM_RegisterFile(fname);
	if(f == 0) {

		return;
	}
	if(f->data == 0) {

		return;
	}
	th = SVM_RegisterThread();
	if(th == 0) {

		return;
	}
	th->curFile = f;
	th->curLine = SVM_FindLabel(f->data,label);

	return;
}

static int CMD_GoTo(const void *context, const char *cmd, const char *args, int cmdFlags){
	const char *fname;
	const char *label;
	scriptFile_t *scr;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_GoTo: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_GoTo: command requires 1 argument");
		return 1;
	}
	if(g_activeThread == 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_GoTo: this can be only used from a script");
		return 1;
	}

	if(Tokenizer_GetArgsCount() == 1) {
		label = Tokenizer_GetArg(0);
		SVM_GoToLocal(g_activeThread,label);
	} else {
		fname = Tokenizer_GetArg(0);
		label = Tokenizer_GetArg(1);
		SVM_GoTo(g_activeThread,fname,label);
	}

	return 1;
}

static int CMD_StartScript(const void *context, const char *cmd, const char *args, int cmdFlags){
	const char *fname;
	const char *label;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_StartScript: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_StartScript: command requires 1 argument");
		return 1;
	}

	fname = Tokenizer_GetArg(0);
	label = Tokenizer_GetArg(1);


	SVM_StartScript(fname,label);


	return 1;
}
static int CMD_Delay_s(const void *context, const char *cmd, const char *args, int cmdFlags){
	int del;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Delay_s: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Delay_s: command requires 1 argument");
		return 1;
	}
	if(g_activeThread == 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Delay_s: this can be only used from a script");
		return 1;
	}

	del = Tokenizer_GetArgInteger(0);

	g_activeThread->currentDelayMS += del;


	return 1;
}
static int CMD_Return(const void *context, const char *cmd, const char *args, int cmdFlags){

	if(g_activeThread == 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Return: this can be only used from a script");
		return 1;
	}

	g_activeThread->curFile = 0;
	g_activeThread->curLine = 0;


	return 1;
}

void CMD_InitScripting(){
    CMD_RegisterCommand("startScript", "", CMD_StartScript, "qqqqq0", NULL);
    CMD_RegisterCommand("goto", "", CMD_GoTo, "qqqqq0", NULL);
    CMD_RegisterCommand("delay_s", "", CMD_Delay_s, "qqqqq0", NULL);
    CMD_RegisterCommand("return", "", CMD_Return, "qqqqq0", NULL);

}


