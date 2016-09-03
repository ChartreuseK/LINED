#include "filebuf.h"
#include <string.h>
#include <stdlib.h>

#define BUFALLOCSIZ		256			// Number of lines to allocate at a time




/* filebuf_init -- Get filebuf ready for use 
 */
int filebuf_init(struct filebuf *fb)
{
	if (fb == NULL)
		return -1;
	
	fb->numlines = 0;
	fb->lines = malloc(BUFALLOCSIZ * sizeof(char *));
	
	if (fb->lines == NULL)
		return -2;
	
	fb->linessiz = BUFALLOCSIZ;
	
	return 0;
}


/* filebuf_insert -- Inserts a line at the specified position
 *   Allocates a copy of the line to store in the buffer
 *   Returns
 */
long filebuf_insert(struct filebuf *fb, const char *line, long pos)
{
	char *newline;
	char **tmp;
	
	if (fb == NULL)
		return -1;
	
	// Make space for the line
	if (fb->numlines >= fb->linessiz)
	{
		tmp = realloc(fb->lines, (fb->linessiz + BUFALLOCSIZ) * sizeof(char *));
		if (tmp == NULL)
			return -2;
		fb->lines = tmp;
			
		fb->linessiz += BUFALLOCSIZ;
	}
	
	if ((newline = strdup(line)) == NULL)
		return -3;
	
	// If the position is at the end or past it, cap it to the end
	if (pos >= fb->numlines)
		fb->lines[fb->numlines++] = newline;
	else
	{
		// Push lines forward to insert
		for (int i = fb->numlines-1; i >= pos; i--)
			fb->lines[i+1] = fb->lines[i];
		fb->numlines++;
		fb->lines[pos] = newline;
	}
	
	return pos >= fb->numlines ? fb->numlines-1 : pos;
}


/* filebuf_delete -- Removes a specified line from the buffer
 *    Frees allocated copy of line
 *    Returns number of remaining lines on success, negative on error
 */
long filebuf_delete(struct filebuf *fb, long pos)
{
	if (fb == NULL || fb->lines == NULL)
		return -1;
		
	if (pos > fb->numlines)
		return -2;
		
	for (int i = pos-1; i < fb->numlines; i++)
		fb->lines[i] = fb->lines[i+1];
	
	// Remove the last line
	fb->lines[fb->numlines--] = NULL;
		
	return fb->numlines;
}


/* filebuf_load -- Reads in a file into a buffer
 */
long filebuf_load(struct filebuf *fb, const char *filename)
{
	char buffer[8192];
	FILE *file;
	
	if (fb == NULL || fb->lines == NULL)
		return -1;
	
	if ((file = fopen(filename, "r")) == NULL)
	{
		return -2;
	}
	
	rewind(file);
	
	while ( !feof(file) && !ferror(file) )
	{
		// TODO: Handle lines larger than buffer size
		if (fgets(&buffer[0], 8192, file) == NULL)
			break;
		
		// Remove trailing newline
		buffer[ strcspn(buffer, "\n") ] = '\0';
		
		filebuf_insert(fb, buffer, fb->numlines);
	}
	
	fclose(file);
	return fb->numlines;
}


/* filebuf_save -- Writes buffer out into file
 */
long filebuf_save(struct filebuf *fb, const char *filename)
{
	FILE *file;
	
	if (fb == NULL || fb->lines == NULL)
		return -1;
		
	if ((file = fopen(filename, "w")) == NULL)
	{
		return -2;
	}
	
	for (int i = 0; i < fb->numlines; i++)
	{
		fprintf(file, "%s\n", fb->lines[i]);
	}
	
	fclose(file);
	
	return fb->numlines;
}



/* filebuf_free -- Cleans up a filebuf, freeing all allocated memory
 */
int filebuf_free(struct filebuf *fb)
{
	if (fb == NULL || fb->lines == NULL)
		return -1;
		
	for (int i = 0; i < fb->numlines; i++)
	{
		if (fb->lines[i] != NULL)
			free(fb->lines[i]);
	}
	free(fb->lines);
	
	return 0;
}
