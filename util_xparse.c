#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util_xml.h"




//#define UTIL_XML_PARSE_DBG
#ifndef UTIL_XML_PARSE_DBG
#define UXMLP_INFO(...)  \
   // UTIL_DEBUG(DBG_INFO, MODULE_UTIL, MASK_UTIL_XML, __VA_ARGS__)
#define UXMLP_ERR(...)   \
    //UTIL_DEBUG(DBG_ERR, MODULE_UTIL, MASK_UTIL_XML, __VA_ARGS__)
#define UXMLP_HINT(...)  \
   // UTIL_DEBUG(DBG_HINT, MODULE_UTIL, MASK_UTIL_XML, __VA_ARGS__)
#else
#define UXMLP_INFO(...)  \
   // UTIL_DEBUG(DBG_INFO, MODULE_DBG_FILE, MASK_DBG_FILE, __VA_ARGS__)
#define UXMLP_ERR(...)   \
    //UTIL_DEBUG(DBG_ERR, MODULE_DBG_FILE, MASK_DBG_FILE, __VA_ARGS__)
#define UXMLP_HINT(...)  \
   // UTIL_DEBUG(DBG_HINT, MODULE_DBG_FILE, MASK_DBG_FILE, __VA_ARGS__)
#endif




/* Element state */
#define XST_GET_START_TAG   1
#define XST_GET_VALUE       2
#define XST_GET_END_TAG     3
#define XST_VALIDATE        4
#define XST_DONE        5

/* Start tag state */
#define ST_GET_TAG      0
#define ST_GET_ATTR_NAME    1
#define ST_GET_QUOTE        2
#define ST_GET_ATTR_VALUE   3
#define ST_GET_SPACE        4
#define ST_GET_SLASH        5
#define ST_DONE         6

#define MAX_ERR_STRINGS 11

static void dlelete_tag_t_xml(pUTIL_XML_TAG tag);

static char *ParserErrorString[MAX_ERR_STRINGS+1] =
{
    "OK",
    "invalid start tag",
    "invalid attributes",
    "invalid value",
    "invalid end tag",
    "start and end tags don't match",
    "no end tag",
    "failed processing attributes",
    "failed processing tag",
    "failed processing value",
    "two root tags",
    "unknown",
};

static int validate_start_tag(char *str, int len)
{
    if((str[0] != '<') || (str[len-1] != '>'))
        return 1;
    return 0;
}

static int validate_attributes(char *str, int len)
{
    return 0;
}

static int validate_value(char *str, int len)
{
    return 0;
}

static int validate_end_tag(char *str, int len)
{
    if(len) {  // could be empty
        if((str[0] != '<') || (str[len-1] != '>'))
            return 1;
        if(str[1] != '/')
            return 1;
    }
    return 0;
}

static int replace_inbound_value_references(pUTIL_XML_TAG tag)
{
    char *outbuf, *inbuf;
    int inlen;
    int i, o;

    inbuf = tag->value_name;
    inlen = tag->value_len;
    outbuf = malloc(inlen+1);
    if(!outbuf)
        return -1;

    i = o = 0;
    while(i < inlen) { // input is not null terminated
        if(inbuf[i] == '&') {
            if((inbuf[i+1] == 'a') &&
               (inbuf[i+2] == 'm') &&
               (inbuf[i+3] == 'p') &&
               (inbuf[i+4] == ';')) {
                outbuf[o++] = 0x26;  // &
                i += 5;
                continue;
            }
            if((inbuf[i+1] == '#') &&
               ((inbuf[i+2] == 'x') || (inbuf[i+2] == 'X')) &&
               ((inbuf[i+3] == 'a') || (inbuf[i+3] == 'A')) &&
               (inbuf[i+4] == ';')) {
                i += 5;    // skip \n
                continue;
            }
            if((inbuf[i+1] == 'l') &&
               (inbuf[i+2] == 't') &&
               (inbuf[i+3] == ';')) {
                outbuf[o++] = 0x3c;  // <
                i += 4;
                continue;
            }
            if((inbuf[i+1] == 'g') &&
               (inbuf[i+2] == 't') &&
               (inbuf[i+3] == ';')) {
                outbuf[o++] = 0x3e;  // >
                i += 4;
                continue;
            }
            if((inbuf[i+1] == 'a') &&
               (inbuf[i+2] == 'p') &&
               (inbuf[i+3] == 'o') &&
               (inbuf[i+4] == 's') &&
               (inbuf[i+5] == ';')) {
                outbuf[o++] = 0x27;  // '
                i += 6;
                continue;
            }
            if((inbuf[i+1] == 'q') &&
               (inbuf[i+2] == 'u') &&
               (inbuf[i+3] == 'o') &&
               (inbuf[i+4] == 't') &&
               (inbuf[i+5] == ';')) {
                outbuf[o++] = 0x22;  // "
                i += 6;
                continue;
            }
            #if 0 /*unkown input, skip it*/
            /* input is bad */
            free(outbuf);
            return -2;
            #endif
        }
        outbuf[o++] = inbuf[i++];
    }
    outbuf[o] = 0x00;
    tag->value_len = o; // adjust strlen
    tag->value = outbuf;
    return 0;
}

static int validate_tag(pUTIL_XML_TAG tag)
{
    int i;
    int start_len, end_len;
    char *str;
    char *p = NULL;
    char start_name[128];
    char end_name[128];

    start_len = tag->start_len-2; // ignore open and closing braces
    if(tag->attr_len) // ignore space + attributes
        start_len = start_len - (tag->attr_len+1);
    if((start_len<1) || (start_len>sizeof(start_name)))
    {
        return -1;
    }
    strncpy(start_name, tag->start_name+1, start_len);
    start_name[start_len] = 0x00;
    for(i=(start_len-1); i>1; i--)
    {
        if(start_name[i]==' ' || start_name[i]=='\t'
            || start_name[i] == '\r' || start_name[i] == '\n')
        {
            start_name[i] = 0;
        }
        else
        {
            break;
        }
    }

    /* start tag must be valid here */
    if(validate_start_tag(tag->start_name, tag->start_len))
    {
        UXMLP_ERR("tag->start_name %s\n",tag->start_name);
        return -1;
    }
    if(!tag->empty) 
    {
        end_len = tag->end_len-3; // ignore braces and /
        if(tag->end_name[1] != '/') {
            if(tag->value_len == 0)
            {
                return 1;   /* no match, treat as a child */
            }
            else 
            {
                for(i=0; i<tag->value_len; i++)
                {
                    if((tag->value_name[i]=='\n')||(tag->value_name[i]=='\r')
                        ||(tag->value_name[i]=='\t') || (tag->value_name[i]==' '))
                    {
                        continue;
                    }
                    return -6;
                }
                return 1;
            }
            return -6;      /* value with no end tag is incorrect */
        }

        if((end_len<1) || (end_len>sizeof(end_name)))
        {
            return -1;
        }
        strncpy(end_name, tag->end_name+2, end_len);
        end_name[end_len] = 0x00;


        for(i=(end_len-1); i>1; i--)
        {
            if(end_name[i]==' ' || end_name[i]=='\t'
                || end_name[i] == '\r' || end_name[i] == '\n')
            {
                end_name[i] = 0;
            }
            else
            {
                break;
            }
        }
        
        if(strcmp(start_name, end_name))
        {
            UXMLP_ERR("tags don't match [%s] : [%s]\n", start_name, end_name);
            return -5;   /* start and end tag don't match */
        }
    }
    else
    {
        if(start_name[start_len-1] == '/')
        {
            start_name[start_len-1] = 0x00;  
        }
    }

    if(validate_attributes(tag->attr_name, tag->attr_len))
        return -2;

    if(validate_value(tag->value_name, tag->value_len))
        return -3;

    if(validate_end_tag(tag->end_name, tag->end_len))
        return -4;

    if(start_len) {
        str = malloc(start_len+1);
        if(!str) return -8;
        p=strstr(start_name, " ");
        if(p)
        {
            memcpy(str, start_name, p-start_name);
            str[p-start_name] = 0x00;
        }
        else
        {
            memcpy(str, start_name, start_len);
            str[start_len] = 0x00;
        }
        tag->name = str;
    }

    if(tag->value_len) {
        if(replace_inbound_value_references(tag))
            return -9;
    }

    if(tag->attr_len) {
        str = malloc(tag->attr_len+1);
        if(!str) return -7;
        memcpy(str, tag->attr_name, tag->attr_len);
        str[tag->attr_len] = 0x00;
        tag->attr = str;
    }

    return 0;  /* match */
}

static pUTIL_XML_TAG create_tag(pUTIL_XML_REQ req,  pUTIL_XML_TAG parent)
{
    pUTIL_XML_TAG tag, empty;

    tag = malloc(sizeof(UTIL_XML_TAG));
    if(!tag) {
        return NULL;
    }
    memset(tag, 0, sizeof(UTIL_XML_TAG));
    tag->marker = TAG_MARKER;
    tag->count = req->tag_count;
    tag->parent = parent;

    /* add tag to parent's child list */
    if(parent) {
        if(!parent->first_child) {
            parent->first_child = tag;
        }
        else {
            empty = parent->first_child;
            while(empty->next)
                empty = empty->next;
            empty->next = tag;
        }
    }
    req->tag_count++;
    return tag;
}

static int set_xstate(int state)
{
    return state;
}

/* basic xml parser */
static int util_xml_parse(pUTIL_XML_REQ req, pUTIL_XML_TAG parent, char *xstr, int len)
{
    int skip = 0;
    int i;
    int next_ch = -1;
    int last_ch = -1;
    int this_ch = -1;
    int rval;
    int state;
    int err_ind;
    char *p1;
    int len1 = 0;
    pUTIL_XML_TAG tag = NULL;

    state = set_xstate(XST_DONE);
    i = 0;
    while(i < len) {
        last_ch = xstr[i-1];
        this_ch = xstr[i];
        next_ch = xstr[i+1];
        //UXMLP_INFO("last_ch %c this_ch %c next_ch %c\n",last_ch,this_ch,next_ch);
        if(state == XST_DONE) {

            if(req->root_tag && !parent)
            {
                rval = -10;
                err_ind = -rval;
                sprintf(req->error_string, "Tag %d is invalid (%s)", req->tag_count, ParserErrorString[err_ind]);
                return 0;
            }

            /* done processing at this level, return processed count to previous level */
            if(this_ch == '<' && next_ch == '/')
            {
                return i;
            }

            #if 1  /*menghong 2009-12-17 防止标签后有回车解析不正确*/
            while((this_ch == '\n')||(this_ch == '\r') ||(this_ch == ' ') || (this_ch == '\t'))   /*每个'<' 前跳过\n */
            {
                i++;
                last_ch = xstr[i-1];
                this_ch = xstr[i];
                next_ch = xstr[i+1];
                if(this_ch == '<' && next_ch == '/')
                {
                    return i;    
                }
            }
            #endif
            state = set_xstate(XST_GET_START_TAG);
            tag = create_tag(req, parent);
        }

        switch (state) {
            case XST_GET_START_TAG:
                if(!tag->start_name) 
                {
                    p1 = NULL;
                    len1 = 0;
                    if(!strncmp(&xstr[i], "<!--", 4))   /*跳过xml  注释*/
                    {
                        p1 = strstr(&xstr[i], "-->");
                        if(p1)
                        {
                            len1 = p1- &xstr[i] + 3;
                            i += len1;
                            return i;
                        }                       
                    }
#if 1 
                    while((xstr[i] == '\n')||(xstr[i] == '\r') ||(xstr[i] == ' ')||(xstr[i] == '\t'))
                    {
                        i++;
                        next_ch = xstr[i];
                        if((next_ch != '\n')&&(next_ch!= '\r')&&(next_ch!= ' ')&&(next_ch != '\t'))
                        {
                            break;
                        }
                    }
#endif

                    tag->start_name = &xstr[i];
                }
                tag->start_len++;
                /* process any tag attributes */
                if(!tag->attr_name) {
                    if(((this_ch==' ') || (this_ch=='\n') || (this_ch=='\r')) 
                            && (next_ch != '/'))
                    {
                        tag->attr_name = &xstr[i+1];
                    }
                }
                else {
                    if(this_ch != '>')
                    {
                        tag->attr_len++;
                    }
                }

                if (this_ch == '>') {
                    if(last_ch == '?') {
                        if(!req->prolog_tag) {
                            req->prolog_tag = tag;
                            state = set_xstate(XST_DONE);                           
                            goto NextChar;
                        }
                        else {
                            state = set_xstate(XST_DONE);    
                            dlelete_tag_t_xml(tag);
                            goto NextChar;
                            //sprintf(req->error_string, "Received multiple ?>");
                            //return -1;
                        }
                    }
                    else if(!req->root_tag) {
                        req->root_tag = tag;
                    }

                    if(last_ch == '/')  
                    { // empty tag
                        state = set_xstate(XST_VALIDATE);
                        tag->empty = 1;
                    }
                    else
                    {                        
                        state = set_xstate(XST_GET_VALUE);
                    }
                }
                break;

            case XST_GET_VALUE:
                //UXMLP_INFO("XST_GET_VALUE %d \n",this_ch);
                skip = 0;
                #if 0  /*menghong 2009-12-17 防止标签后有回车解析不正确*/
                while((xstr[i]=='\n')||(xstr[i]=='\r')||(xstr[i]=='\t') || (xstr[i]==' '))
                {
                    skip++;
                    i++;
                }
                this_ch = xstr[i];
                #endif
                
                if (this_ch != '<') {
                    i -= skip;
                    if(!tag->value_name)
                    {
                        tag->value_name = &xstr[i];
                    }
                    
                    tag->value_len++;
                    break;
                }
                else
                {
                    state = set_xstate(XST_GET_END_TAG);
                }
                /* fall through is deliberate */

            case XST_GET_END_TAG:
                if(!tag->end_name)
                    tag->end_name = &xstr[i];
                tag->end_len++;
                if (this_ch == '>')
                {
                    state = set_xstate(XST_VALIDATE);
                }
                break;
        }

        if(state == XST_VALIDATE) 
        {
            state = set_xstate(XST_DONE);
            rval = validate_tag(tag);
            if (rval == 0) {
                tag = NULL;
                goto NextChar;
            }
            /* tag is invalid */
            else if (rval < 0) {
                err_ind = -rval;
                if(err_ind > MAX_ERR_STRINGS)
                {
                    err_ind = MAX_ERR_STRINGS;
                }
                sprintf(req->error_string, "Tag %d is invalid (%s)", req->tag_count, ParserErrorString[err_ind]);
                return -1;
            }

            // tag mismatch, adjust len and process child
            i = i - tag->end_len;
            rval = util_xml_parse(req, tag, tag->end_name, len-i);
            if(rval <= 0)
            {
                return rval;
            }
            tag->end_name = 0;  // clear end tag
            tag->end_len = 0;
            state = set_xstate(XST_GET_END_TAG);
            i = i+rval; // advance processed offset
        }
NextChar:
        i++;
    };

    return i;
}

int util_xml_validate(pUTIL_XML_REQ req, char *xstr, int len)
{
    int rval;
    pUTIL_XML_TAG parent = NULL;

    if(!req || !xstr)
    {
        return -1;
    }
    
    rval = util_xml_parse(req, parent, xstr, len);
    if((req->root_tag == NULL) || (req->root_tag->name==NULL))
    {
        return -1;
    }
    
    return rval;
}

#if 0
unsigned long util_xml_required_mask(int count)
{
    unsigned long mask = 0;
    int f;

    for(f = 1; f <= count; f++)
        mask |= RQFLD(f);
    return mask;
}
#endif

int xml_check_required_fields(unsigned long req, unsigned long mask)
{
    if((req & mask) == mask)
        return 1;
    return 0;
}

static void delete_tag_t_xmlree(pUTIL_XML_TAG tag)
{
    pUTIL_XML_TAG next;

    if((tag) && (tag->marker == TAG_MARKER)) {
        do {
            next = tag->next;
            if(tag->first_child)
            {
                delete_tag_t_xmlree(tag->first_child);
                tag->first_child = NULL;
            }

            /* delete node */
            if((tag) && (tag->marker == TAG_MARKER)) {
                if(tag->name){
                    free(tag->name);
                    tag->name = NULL;
                }
                if(tag->value){
                    free(tag->value);
                    tag->value = NULL;
                }
                if(tag->attr)
                {
                    free(tag->attr);
                    tag->attr = NULL;
                }
                free(tag);
            }
            tag->next = NULL;
            tag = next;
        } while(tag);
    }
}

static void dlelete_tag_t_xml(pUTIL_XML_TAG tag)
{
    if((tag) && (tag->marker == TAG_MARKER))
    {
        if(tag->first_child)
        {
            delete_tag_t_xmlree(tag->first_child);
        }

        /* delete node */
        if(tag->name)
        {
            free(tag->name);
        }
        if(tag->value)
        {
            free(tag->value);
        }
        if(tag->attr)
        {
            free(tag->attr);
        }
        free(tag);
    }
}

int util_xml_init(pUTIL_XML_REQ req)
{
    req->prolog_tag = NULL;
    req->root_tag = NULL;
    req->tag_count = 0;
    return 0;
}

void util_xml_cleanup(pUTIL_XML_REQ req)
{
    delete_tag_t_xmlree(req->prolog_tag);
    delete_tag_t_xmlree(req->root_tag);
    req->prolog_tag = NULL;
    req->root_tag = NULL;
}

