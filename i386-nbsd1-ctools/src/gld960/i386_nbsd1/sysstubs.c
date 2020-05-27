
#include <stdio.h>

execv() {}

static char *asciize(unsigned long k) {
    static char buff[5];
    int i;

    for (i=0;i < 5;i++) {
	buff[i] = 'A' + (k % 26);
	k = k / 26;
    }
    buff[4] = 0;
    return buff;
}

#if 0
char *mktemp(char *s) {
    static char *buff;
    static int buff_length = 0;
    static int key = 0;
    char *p;

    if (strlen(s) > buff_length) {
	if (buff)
		buff = ldrealloc(buff,buff_length = strlen(s) + 1);
	else
		buff = ldmalloc(buff_length = strlen(s) + 1);
    }
    strcpy(buff,s);
    for (p=buff;p && *p;p++)
	    if (strcmp("XXXXXX",p) == 0)
		    break;
    if (p) {
	*p = 0;
	strcat(buff,asciize(key++));
    }
    strcpy(s,buff);
    return buff;
}
#endif

chmod() {}

system(char *command)
{
    printf("\n\n\nsystem(%s)\n\n\n",command);
    printf("Please enter <CR> when ready:");
    getchar();
    return 0;
}
