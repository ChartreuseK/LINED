#ifndef FILEBUF_H
#define FILEBUF_H

#include <stdio.h>


struct filebuf
{
	char **lines;   // Pointer to the start of each line
	long numlines;  // Number of used lines in the buffer
	long linessiz;	// Number of lines allocated in buffer
};


int  filebuf_init(struct filebuf *fb);
long filebuf_insert(struct filebuf *fb, const char *line, long pos);
long filebuf_delete(struct filebuf *fb, long pos);
long filebuf_load(struct filebuf *fb, const char *filename);
long filebuf_save(struct filebuf *fb, const char *filename);
int  filebuf_free(struct filebuf *fb);


#endif
