#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "util_xml.h"




//#define UTIL_XML_ADD_DBG
#ifndef UTIL_XML_ADD_DBG
#define UXMLA_INFO(...)  \
    //UTIL_DEBUG(DBG_INFO, MODULE_UTIL, MASK_UTIL_XML, __VA_ARGS__)
#define UXMLA_ERR(...)   \
   // UTIL_DEBUG(DBG_ERR, MODULE_UTIL, MASK_UTIL_XML, __VA_ARGS__)
#define UXMLA_HINT(...)  \
   // UTIL_DEBUG(DBG_HINT, MODULE_UTIL, MASK_UTIL_XML, __VA_ARGS__)
#else
#define UXMLA_INFO(...)  \
    //UTIL_DEBUG(DBG_INFO, MODULE_DBG_FILE, MASK_DBG_FILE, __VA_ARGS__)
#define UXMLA_ERR(...)   \
   // UTIL_DEBUG(DBG_ERR, MODULE_DBG_FILE, MASK_DBG_FILE, __VA_ARGS__)
#define UXMLA_HINT(...)  \
    //UTIL_DEBUG(DBG_HINT, MODULE_DBG_FILE, MASK_DBG_FILE, __VA_ARGS__)
#endif


/* append formatted string to the response buffer */

void util_xml_append(pUTIL_XML_REQ dstbuf, const char *fmt, ...)
{
    va_list ap;
    char text[512];
    int len;
    char *buf;

    va_start(ap, fmt);
    len = vsnprintf(text, 512, fmt, ap);
    if(len == 512)
    {
        UXMLA_ERR("CATION!!!!!!!!!XML Text length maybe larger than 512!!!!!!!\n");
    }
    va_end(ap);

    len = strlen(text);

    if((dstbuf->currlen + len) >= dstbuf->buflen)
    {
        UXMLA_ERR("CATION!!!!!!!!!XML BUFFER NOT ENOUGH!!!!!!!\n");
        return ;
    }   

    /* append new string to xml buffer */
    buf = &(dstbuf->buf[dstbuf->currlen]);
    memcpy(buf, text, len);
    dstbuf->currlen += len;
    buf = &dstbuf->buf[dstbuf->currlen];
    *buf = 0x00;
}

void util_xml_append_ex(pUTIL_XML_REQ dstbuf, const char *fmt, ...)
{
    va_list ap;
    char text[1024*3];
    int len;
    char *buf;

    va_start(ap, fmt);
    len = vsnprintf(text, 1024*3, fmt, ap);
    if(len == 1024*3)
    {
        UXMLA_ERR("CATION!!!!!!!!!XML Text length maybe larger than 1024*3!!!!!!!\n");
    }
    va_end(ap);

    len = strlen(text);

    if((dstbuf->currlen + len) >= dstbuf->buflen)
    {
        UXMLA_ERR("CATION!!!!!!!!!XML BUFFER NOT ENOUGH!!!!!!!\n");
        return ;
    }   

    /* append new string to xml buffer */
    buf = &(dstbuf->buf[dstbuf->currlen]);
    memcpy(buf, text, len);
    dstbuf->currlen += len;
    buf = &dstbuf->buf[dstbuf->currlen];
    *buf = 0x00;
}



/* add <starttag attributes> to the response buffer */
void util_xadd_stag_attr(pUTIL_XML_REQ dstbuf, const char *tag, const char *fmt, ...)
{
    va_list ap;
    int len;
    char attr[512];
    va_start(ap, fmt);
    len = vsnprintf(attr, 512, fmt, ap);
    if(len == 512)
    {
        UXMLA_ERR("CATION!!!!!!!!!XML Text length maybe larger than 512!!!!!!!\n");
    }
    va_end(ap);
    util_xml_append(dstbuf, "<%s %s>\n", tag, attr);
}

/* add <starttag> to the response buffer */
void util_xadd_stag(pUTIL_XML_REQ dstbuf, const char *tag)
{
    util_xml_append(dstbuf, "<%s>\n", tag);
}

/* add <endtag> to the response buffer */
void util_xadd_etag(pUTIL_XML_REQ dstbuf, const char *tag)
{
    util_xml_append(dstbuf, "</%s>\n", tag);
}

/* add <starttag>value<endtag> to the response buffer */
void util_xadd_elem(pUTIL_XML_REQ dstbuf, const char *tag, const char *val)
{
    if(val == NULL)
    {
        util_xml_append(dstbuf, "<%s/>\n", tag);
    }
    else
    {
        util_xml_append(dstbuf, "<%s>%s</%s>\n", tag, val, tag);
    }
}

void util_xadd_elem_ex(pUTIL_XML_REQ dstbuf, const char *tag, const char *val)
{
    if(val == NULL)
    {
        util_xml_append_ex(dstbuf, "%s\n", tag);
    }
    else
    {
        util_xml_append(dstbuf, "<%s>%s</%s>\n", tag, val, tag);
    }
}


/* add <starttag>value<endtag> to the response buffer */
void util_xadd_elem_attr(pUTIL_XML_REQ dstbuf, const char *tag, const char *val, const char *fmt, ...)
{
    int len;
    char attr[512];
    va_list ap;

    if(fmt != NULL)
    {
        va_start(ap, fmt);
        len = vsnprintf(attr, 512, fmt, ap);
        if(len == 512)
        {
            UXMLA_ERR("CATION!!!!!!!!!XML Text length maybe larger than 512!!!!!!!\n");
        }
        va_end(ap);
        
        if(val)
        {
            util_xml_append(dstbuf, "<%s %s>%s</%s>\n", tag, attr, val, tag);
        }
        else
        {
            util_xml_append(dstbuf, "<%s %s/>\n", tag, attr);
        }
        return;
    }

    if(val == NULL)
    {
        util_xml_append(dstbuf, "<%s/>\n", tag);
    }
    else
    {
        util_xml_append(dstbuf, "<%s>%s</%s>\n", tag, val, tag);
    }

    return;
}

/**@fn     void util_xadd_int_elem(pUTIL_XML_REQ pstruDstBuf,const char *szTag,int iVal)
 * @brief  add int value element
 * @brief  Author/Date hcqiu/2010-10-09
 * @param  [in]pstruDstBuf:pointer to pUTIL_XML_REQ structure
 * @param  [in]szTag:tag
 * @param  [in]iVal:int type value
 * @param  [out]
 * @return 
 */
void util_xadd_int_elem(pUTIL_XML_REQ pstruDstBuf,const char *szTag,int iVal)
{
    char szVal[32] = {0};
    if((pstruDstBuf != NULL) && (szTag != NULL))
    {
        memset(szVal, 0, sizeof(szVal));
        sprintf(szVal, "%d", iVal);
        util_xadd_elem(pstruDstBuf, szTag, szVal);
    }
    return ;
}

/**@fn     void util_xadd_float_elem(pUTIL_XML_REQ pstruDstBuf,const char *szTag,float fVal)
 * @brief  add float value element
 * @brief  Author/Date hcqiu/2010-10-09
 * @param  [in]pstruDstBuf:pointer to pUTIL_XML_REQ structure
 * @param  [in]szTag:tag
 * @param  [in]fVal:float type value
 * @param  [out]
 * @return 
 */
void util_xadd_float_elem(pUTIL_XML_REQ pstruDstBuf,const char *szTag,float fVal)
{
    char szVal[32] = {0};
    if((pstruDstBuf != NULL) && (szTag != NULL))
    {
        memset(szVal, 0, sizeof(szVal));
        sprintf(szVal, "%f", fVal);
        util_xadd_elem(pstruDstBuf, szTag, szVal);
    }
    return ;
}


