/** \file ExtraSocketCatchcopy.cpp
\brief Define the socket of catchcopy
\author alpha_one_x86 */

#include "ExtraSocketCatchcopy.h"

#include <stdio.h>

const std::string ExtraSocketCatchcopy::pathSocket()
{
#ifdef Q_OS_UNIX
    return "advanced-copier-"+std::to_string(getuid());
#else
    QString userName;
    char uname[1024];
    DWORD len=1023;
    if(GetUserNameA(uname, &len)!=FALSE)
        userName=QString::fromLatin1(toHex(uname));
    return "advanced-copier-"+userName.toStdString();
#endif
}

char * ExtraSocketCatchcopy::toHex(const char *str)
{
    char *p, *sz;
    size_t len;
    if (str==NULL)
        return NULL;
    len= strlen(str);
    p = sz = (char *) malloc((len+1)*4);
    for (size_t i=0; i<len; i++)
    {
        sprintf(p, "%.2x00", str[i]);
        p+=4;
    }
    *p=0;
    return sz;
}
