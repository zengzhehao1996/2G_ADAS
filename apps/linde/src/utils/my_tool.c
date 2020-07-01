#include "my_misc.h"
#include <string.h>

/**
 * @brief search string
 * @param pdata, pointer to source buffer.
 * @param find, pointer to find string,
 * @param size, source buffer size,
 * @return a pointer to the beginning of the located substring, or NULL if the substring is not found.
 */
uint8_t *memstr(const uint8_t *pdata, const uint8_t *find, int size)
{
    uint8_t  c, sc;
    size_t len;
    int i=0;

    c = *find++;
    if (c != 0) 
    {
        len = strlen(find);
        do 
        {
            do 
            {
                sc = pdata[i];
                i++;
                if (i>=size)
                return NULL;
            } while (sc != c);
        } while (memcmp(&pdata[i], find, len) != 0);
        i--;
    }
    return (uint8_t *)&pdata[i];
}

double atod(const uint8_t *str)
{
  double s=0.0;
  double d=10.0;
  uint32_t i = 0;

  if(!(str[i]>='0'&&str[i]<='9'))
    return s;

  while(str[i]>='0'&&str[i]<='9'&&str[i]!='.'){
    s=s*10.0+str[i]-'0';
    i++;
  }

  if(str[i]=='.')
    i++;

  while(str[i]>='0'&&str[i]<='9')	{
    s=s+(str[i]-'0')/d;
    d*=10.0;
    i++;
  }

  return s;
}

void turnEndian(uint8_t *data, int len)
{
    if(data == NULL || len % 2 != 0)
    {
        err_log("Parameter ERROR.\n");
        return ;
    }
    uint8_t tmp;
    for(int i=0; i<len/2;i++)
    {
        tmp = data[i];
        data[i] = data[len-i-1];
        data[len - i - 1] = tmp;
    }
}

uint32_t toBigEndian(uint32_t val)
{
    uint8_t *p = &val;
    uint8_t tmp;
    tmp  = p[0];
    p[0] = p[3];
    p[3] = tmp;
    tmp  = p[1];
    p[1] = p[2];
    p[2] = tmp;
    return val;
}

void bit32Set(uint32_t *pval, int pos)
{
    if(NULL==pval || pos < 0 || pos > 31)
    {
        err_log("Para Error.\n");
        return;
    }
   
    *pval = *pval | (1<<pos);
}

void bit32Clear(uint32_t *pval, int pos)
{
    if(NULL==pval || pos < 0 || pos > 31)
    {
        err_log("Para Error.\n");
        return;
    }

    *pval = *pval & (~(1<<pos));
}