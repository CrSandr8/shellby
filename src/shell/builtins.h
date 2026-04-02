#ifndef _BUILTIN_H
#define _BUILTIN_H

int format(const char * name, int size);
int mkdir();
int cd();
int touch();
int cat();
int ls();
int append();
int rm();
int close();

int do_shell(const char* prompt);

#endif
