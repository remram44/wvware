#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "wv.h"
/*
Released under GPL, written by Caolan.McNamara@ul.ie.

Copyright (C) 1998,1999 
	Caolan McNamara

Real Life: Caolan McNamara           *  Doing: MSc in HCI
Work: Caolan.McNamara@ul.ie          *  Phone: +353-86-8790257
URL: http://skynet.csn.ul.ie/~caolan *  Sig: an oblique strategy
How would you have done it?
*/

/*
returns 1 for not an ole doc
2 ole but not word doc
-1 for an error of some unknown kind
0 on success
*/

int myelehandler(wvParseStruct *ps,wvTag tag, void *props, int dirty);
int mydochandler(wvParseStruct *ps,wvTag tag);
int myCharProc(wvParseStruct *ps,U16 eachchar,U8 chartype);
int mySpecCharProc(wvParseStruct *ps,U16 eachchar,CHP *achp);

FILE *wvOpenConfig(char *config);

void usage( void )
	{
	printf("Usage: wvHtml [--config config.xml] [--charset charset] [--password password] filename.doc\n");
	exit(-1);
	}

static U16 charset=0xffff;

int main(int argc,char **argv)
	{
	FILE *input;
	char *password=NULL;
	char *config=NULL;
	int ret;
	state_data myhandle;
	expand_data expandhandle;
	wvParseStruct ps;
	int c,index=0;
	static struct option long_options[] =
        {
        {"charset",1,0,'c'},
        {"config",1,0,'x'},
        {"password",1,0,'p'},
        {0,0,0,0}
        };

	wvInitError();

	if (argc < 2) 
		usage();

	 while (1)
        { 
        c = getopt_long (argc, argv, "c:x:p:", long_options, &index);
        if (c == -1)
            break;
		switch(c)
			{
			case 'c':
				if (optarg)
					charset = wvLookupCharset(optarg);
				else
					wvError(("No argument given to charset"));
				break;
			case 'x':
				if (optarg)
					config = optarg;
				else
					wvError(("No config file given to config option"));
				break;
			case 'p':
				if (optarg)
					password = optarg;
				else
					wvError(("No password given to password option"));
				break;
			default:
                usage();
                break;
			}
		}

	input = fopen(argv[optind],"rb");
	if (!input)
		{
		fprintf(stderr,"Failed to open %s: %s\n",argv[1],strerror(errno));
		return(-1);
		}


	ret = wvInitParser(&ps,input);
	ps.filename = argv[optind];

	if (ret & 0x8000)
		{
		if ( (ret & 0x7fff) == WORD8)
			{
			ret = 0;
			if (password == NULL)
				{
				fprintf(stderr,"Password required, this is an encrypted document\n");
				return(-1);
				}
			else
				{
				wvSetPassword(password,&ps);
				if (wvDecrypt97(&ps))
					{
					wvError(("Incorrect Password\n"));
					return(-1);
					}
				}
			}
		else if ( ( (ret & 0x7fff) == WORD7) || ( (ret & 0x7fff) == WORD6))
			{
			ret=0;
			if (password == NULL)
				{
				fprintf(stderr,"Password required, this is an encrypted document\n");
				return(-1);
				}
			else
				{
				wvSetPassword(password,&ps);
				if (wvDecrypt95(&ps))
					{
					wvError(("Incorrect Password\n"));
					return(-1);
					}
				}
			}
		}

	if (ret)
		{
		wvError(("startup error\n"));
		wvOLEFree();
		return(-1);
		}

	wvSetElementHandler(myelehandler);
	wvSetDocumentHandler(mydochandler);
	wvSetCharHandler(myCharProc);
	wvSetSpecialCharHandler(mySpecCharProc);

	wvInitStateData(&myhandle);
    myhandle.fp = wvOpenConfig(config);
    if (myhandle.fp == NULL)
		{
        wvError(("config file not found\n"));
		return(-1);
		}
    else
		{
		wvTrace(("x for FILE is %x\n",myhandle.fp));
        ret = wvParseConfig(&myhandle);
		}

	if (!ret)	
		{
		expandhandle.sd = &myhandle;
		ps.userData = &expandhandle;
		ret = wvHtml(&ps);
		}
	wvReleaseStateData(&myhandle);

	if (ret == 2)
		return(2);
	else if (ret != 0)
		ret = -1;
	wvOLEFree();
	return(ret);
	}

int myelehandler(wvParseStruct *ps,wvTag tag, void *props, int dirty)
    {
	static PAP *ppap;
    expand_data *data = (expand_data *)ps->userData;
    data->anSttbfAssoc = &ps->anSttbfAssoc;
	data->lfo = &ps->lfo;
	data->lfolvl = ps->lfolvl;
	data->lvl = ps->lvl;
	data->nolfo = &ps->nolfo;
	data->nooflvl = &ps->nooflvl;
	data->stsh = &ps->stsh;
	data->lst = &ps->lst;
	data->noofLST = &ps->noofLST;
	data->liststartnos = &ps->liststartnos;
	data->finallvl = &ps->finallvl;
	data->fib = &ps->fib;
	data->intable = &ps->intable;
	data->cellbounds = &ps->cellbounds;
	data->nocellbounds = &ps->nocellbounds;
	data->endcell = &ps->endcell;
	data->vmerges = &ps->vmerges;
	data->norows = &ps->norows;
	data->nextpap = &ps->nextpap;
	if (charset == 0xffff)
    	data->charset = wvAutoCharset(&ps->clx);
	else
		data->charset = charset;
    data->props = props;

    switch (tag)
        {
        case PARABEGIN:
			ppap = (PAP *)data->props;
			wvTrace(("fore back is %d %d\n",((PAP *)(data->props))->shd.icoFore,((PAP *)(data->props))->shd.icoBack));
            wvBeginPara(data);
            break;
        case PARAEND:
            wvEndCharProp(data);	/* danger will break in the future */
            wvEndPara(data);
			wvCopyPAP(&data->lastpap,(PAP*)(data->props));
            break;
        case CHARPROPBEGIN:
            wvBeginCharProp(data,ppap);
            break;
        case CHARPROPEND:
            wvEndCharProp(data);
            break;
		case SECTIONBEGIN:
			wvBeginSection(data);
			break;
		case SECTIONEND:
			wvEndSection(data);
			break;
		case COMMENTBEGIN:
			wvBeginComment(data);
			break;
		case COMMENTEND:
			wvEndComment(data);
			break;
        default:
            break;
        }
    return(0);
    }

int mydochandler(wvParseStruct *ps,wvTag tag)
    {
	static int i;
    expand_data *data = (expand_data *)ps->userData;
    data->anSttbfAssoc = &ps->anSttbfAssoc;
	data->lfo = &ps->lfo;
	data->lfolvl = ps->lfolvl;
	data->lvl = ps->lvl;
	data->nolfo = &ps->nolfo;
	data->nooflvl = &ps->nooflvl;
	data->stsh = &ps->stsh;
	data->lst = &ps->lst;
	data->noofLST = &ps->noofLST;
	data->liststartnos = &ps->liststartnos;
	data->finallvl = &ps->finallvl;
	data->fib = &ps->fib;
	data->intable = &ps->intable;
	data->cellbounds = &ps->cellbounds;
	data->nocellbounds = &ps->nocellbounds;
	data->endcell = &ps->endcell;
	data->vmerges = &ps->vmerges;
	data->norows = &ps->norows;
	if (i==0)
		{
		data->filename = ps->filename;
		data->whichcell=0;
		data->whichrow=0;
		data->asep = NULL;
		i++;
		wvInitPAP(&data->lastpap);
		data->nextpap=NULL;
		}

	if (charset == 0xffff)
	    data->charset = wvAutoCharset(&ps->clx);
	else
		data->charset = charset;

    switch (tag)
        {
        case DOCBEGIN:
            wvBeginDocument(data);
            break;
        case DOCEND:
            wvEndDocument(data);
            break;
        default:
            break;
        }
    return(0);
    }


int mySpecCharProc(wvParseStruct *ps,U16 eachchar,CHP *achp)
	{
	static int message,state;
	/*PICF picf;*/
	FSPA *fspa;
    expand_data *data = (expand_data *)ps->userData;

	switch(eachchar)
		{
		case 19:
			state=1;
			return(0);
			break;
		case 20:
		case 21:
			state=0;
			return(0);
			break;
		}
	if (state) 
		return(0);
	
	switch(eachchar)
		{
		case 0x05:
			/* this should be handled by the COMMENTBEGIN and COMMENTEND events */
			return(0);
			break;
		case 0x01:
			wvError(("picture 0x01 here\n"));
			/*
			fseek(ps->data,achp->fcPic_fcObj_lTagObj,SEEK_SET);
			wvGetPICF(&picf,ps->data);
			*/
			printf("<img src=\"placeholder.png\"><br>");
			return(0);
		case 0x08:
			fspa = wvGetFSPAFromCP(ps->currentcp,ps->fspa,ps->fspapos,ps->nooffspa);
			data->props = fspa;
			if ( (fspa) && (data->sd != NULL) && (data->sd->elements[TT_PICTURE].str) && (data->sd->elements[TT_PICTURE].str[0] != NULL) )
				{
				wvExpand(data,data->sd->elements[TT_PICTURE].str[0],
					strlen(data->sd->elements[TT_PICTURE].str[0]));
				if (data->retstring)
					{
					wvTrace(("picture string is now %s",data->retstring));
					printf("%s",data->retstring);
					wvFree(data->retstring);
					}
				}
			return(0);
		case 0x28:
			{
			U16 symbol[6] = {'S','y','m','b','o','l'};
			U16 wingdings[9] = {'W','i','n','g','d','i','n','g','s'};
			wvTrace(("no of strings %d %d\n",ps->fonts.nostrings,achp->ftcSym));
			if (0 == memcmp(symbol,ps->fonts.ffn[achp->ftcSym].xszFfn,12))
				{
				if ( (!message) && (UTF8 != charset) )
					{
					wvWarning("Symbol font detected (too late sorry!), rerun wvHtml with option --charset utf-8\n\
option to support correct symbol font conversion to a viewable format.\n");
					message++;
					}
				wvTrace(("symbol char %d %x %c, using font %d %s\n",achp->xchSym,achp->xchSym,achp->xchSym,achp->ftcSym,wvWideStrToMB(ps->fonts.ffn[achp->ftcSym].xszFfn) ));
				wvTrace(("symbol char ends up as a unicode %x\n",wvConvertSymbolToUnicode(achp->xchSym-61440)));
				return(myCharProc(ps,wvConvertSymbolToUnicode(achp->xchSym-61440),UTF8));
				}
			else if (0 == memcmp(wingdings,ps->fonts.ffn[achp->ftcSym].xszFfn,18))
				{
				if (!message)
					{
					wvError(("I have yet to do a wingdings to unicode mapping table, if you know of one tell me\n"));
					message++;
					}
				}
			else
				{
				if (!message)
					{
					char *fontname = wvWideStrToMB(ps->fonts.ffn[achp->ftcSym].xszFfn);
					wvError(("Special font %s, i need a mapping table to unicode for this\n",fontname));
					wvFree(fontname);
					printf("*");
					}
				return(0);
				}
			}
		default:
			return(0);
		}
	return(0);
	}

	
int myCharProc(wvParseStruct *ps,U16 eachchar,U8 chartype)
	{
	static int state,i;
	switch(eachchar)
		{
		case 19:
			state=1;
			i=0;
			return(0);
			break;
		case 20:
		case 21:
			state=0;
			return(0);
			break;
		case 0x08:
			wvError(("hmm did we loose the fSpec flag ?, this is possibly a bug\n"));
			break;
		}
	if (state) 
		{
		/*fieldstring[i++] = eachchar;*/
		fieldCharProc(ps,eachchar,chartype);
		return(0);
		}
	if (charset != 0xffff)
		wvOutputHtmlChar(eachchar,chartype,charset);
	else
		wvOutputHtmlChar(eachchar,chartype,wvAutoCharset(&ps->clx));
	return(0);
	}


FILE *wvOpenConfig(char *config)
	{
	FILE *tmp;
	int i=0;
	if (config == NULL)
		config = "wvHtml.xml";
	else
		i=1;
    tmp = fopen(config,"rb");
	if (tmp == NULL)
		{
		if (i) wvError(("Attempt to open %s failed, using %s\n",config,HTMLCONFIG));
		config = HTMLCONFIG;
	    tmp = fopen(config,"rb");
		}
	return(tmp);
	}
