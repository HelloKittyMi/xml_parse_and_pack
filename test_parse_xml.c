#include "util_xml.h"
#include "stdlib.h"
#include "stdio.h"

#define DST_XML_FILE "./test.xml"

#define DATA1   2
#define DATA2   3

/*从目标字符串中解析属性值*/
static int parse_attr(char *dst_str, char *key, char *vaule)
{
	int ret = -1;
	char *str_start = NULL;
	char *str_end	= NULL;
	do
	{
		if((str_start = strstr(dst_str, key)) == NULL)
		{
			break;
		}
		if((str_start = strstr(str_start, "=")) == NULL)
		{
			break;
		}
        str_start += 1;
		if(((str_end = strstr(str_start, " ")) == NULL))
			strncpy(vaule, str_start, strlen(str_start));
        else
            strncpy(vaule, str_start, str_end - str_start);
		printf("key: %s, vaule: %s\n", key, vaule);
	}while(0);
	return ret;
}

int parse_xml_str(char *xml)
{
    pUTIL_XML_REQ util_xml_req = NULL;
	pUTIL_XML_TAG root_tag = NULL;
    char name[100];
    char id[10];
   	util_xml_req = calloc(1, sizeof(UTIL_XML_REQ));
	util_xml_init(util_xml_req);
	util_xml_req->query_str = xml;
	if(util_xml_validate(util_xml_req, xml, strlen(xml)) < 0)
	{
		printf("无效的报文\n");
		return -1;
	}
   	root_tag = util_xml_req->root_tag;
	if(!root_tag->name || strcmp(root_tag->name, "root"))
	{
		printf("报文格式不合法,找不到跟节点");
		return -1;
	}
    printf("root_tag->attr: %s\n", root_tag->attr);
    printf("root_tag->attr_len: %d\n", root_tag->attr_len);
    
    parse_attr(root_tag->attr, "name", name);
    parse_attr(root_tag->attr, "id", id);
    printf("ID:  %d\n",atoi(id));
	int role = atoi(id)?DATA1:DATA2;
	printf("DATA:%d\n",role);
    return 0;
}

int main()
{
    FILE *fp = NULL;
    char *xml_str = NULL;
    int file_len = 0;
    
    if((fp = fopen(DST_XML_FILE, "a+")) == NULL)
    {
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);
    xml_str = calloc(file_len+1, 1);
    
    fseek(fp, 0, SEEK_SET);
    fread(xml_str, file_len, 1, fp);
    parse_xml_str(xml_str);
    fclose(fp);
    return 0;
}
