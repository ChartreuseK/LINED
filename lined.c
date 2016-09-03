#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h> 
#include "filebuf.h"

#define LINESIZE 	512
#define BUFINC		1024


/**************************
 * Global State/Variables */
int running  = 1;
long curline = 1; 
char inputbuf[LINESIZE];
enum { NONE, CURLINE, RELLINE, ENDLINE, NUMBER } sltype, eltype;
long startl  = 0;		
long endl    = 0;
char cmd     = 0;
char *srch   = NULL;
char *repl   = NULL;

char *filename = NULL;

struct filebuf fb;





void usage(char *pname, char *msg);
void prompt(void);
void getinput(void);
void parse(void);
void docmd(void);
char *fixesc(char *start);
void fixlines(void);
void insertmode(int append);
void listlines(void);
void listline(long pos);

int main(int argc, char **argv)
{
	long l;
	if (argc < 2)
	{
		usage(argv[0], "Too few arguments\n");
		return 1;
	}
	
	filebuf_init(&fb);
	
	filename = argv[1];
	
	if ( (l = filebuf_load(&fb, filename)) < 0)
	{
		printf("Creating new file '%s'\n",filename);
		curline = 1;
	}
	else
		printf("Read %d lines\n",l);
	
	while (running)
	{
		prompt();
		getinput();
		parse();
		
		printf("DEBUG: sltype = %d eltype = %d startl = %d endl = %d cmd = %c srch = %s repl = %s\n",
			sltype, eltype, startl, endl, cmd, srch, repl);
		docmd();
	}
	
	filebuf_free(&fb);
	
	
	return 0;
}




void usage(char *pname, char *msg)
{
	fputs(msg, stderr);
	fprintf(stderr, "usage: %s <fname>\n", pname);
}


void prompt(void)
{
	printf("%d> ", curline);
}

void getinput(void)
{
	fgets(&inputbuf[0], LINESIZE, stdin);
}


void parse(void)
{
	char *endptr;
	char *cur;
	enum { R_START=0, R_END, CMD, SRCH_STR, REPL_STR, END } state;
	sltype = NONE; eltype = NONE;
	startl = 0; endl = 0;
	cmd = '\0';
	
	cur = &inputbuf[0];
	state = R_START;

	while (state != END)
	{
		switch (state)
		{
		case R_START:
			// Skip over whitespace
			while (isspace(*cur)) 
				cur++;
			
			startl = strtol(cur, &endptr, 10);
			printf("startl = %d\n", startl);
			// Did we parse any number
			if ( endptr != cur )
			{
				// Check if a relative number was specified
				sltype =  (*cur == '+' || *cur == '-') ? RELLINE : NUMBER;
				cur = endptr;
			} // Check if it was just a + or - by themself
			else if (*cur == '+' || *cur == '-')
			{
				sltype = RELLINE;
				startl = (*cur == '+') ? 1 : -1;
				cur++;
			} // End line
			else if (*cur == '$')
			{
				sltype = ENDLINE;
				cur++;
			} // Explicit current line
			else if (*cur == '.')
			{
				sltype = CURLINE;
				cur++;
			} // Current through end
			else if (*cur == ';')
			{
				sltype = CURLINE;
				eltype = ENDLINE;
				
				
				cur++; 
				state = CMD; 
				break;
			} // Implicit current line
			else
				sltype = CURLINE;
			
			// Check if a range is specified
			if (*cur == ',')
			{
				cur++;
				state = R_END;
			}
			else
			{
				state = CMD;
			}
			
			break;
			
			
		case R_END:
			endl = strtol(cur, &endptr, 10);
			// Did we parse any number
			if ( endptr != cur )
			{
				// Check if a relative number was specified
				eltype =  (*cur == '+' || *cur == '-') ? RELLINE : NUMBER;
				cur = endptr;
			} // Check if it was just a + or - by itself
			else if (*cur == '+' || *cur == '-')
			{
				eltype = RELLINE;
				endl = (*cur == '+') ? 1 : -1;
				cur++;
			} // Explicit end of file
			else if (*cur == '$')
			{
				eltype = ENDLINE;
				cur++;
			} // Explicit current line
			else if (*cur == '.')
			{
				eltype = CURLINE;
				cur++;
			} // Implicit end of file
			else
			{	
				eltype = ENDLINE;
			}
			state = CMD;
			break;
			
			
		case CMD:
			cmd = *cur;
			switch (cmd)
			{
			case 'R':
			case 'S':
				state = SRCH_STR;
				cur++;
				break;
			case '\0':
			case '\n':
			case ' ':
			case '\t':
				cmd = '\0';
			default:
				state = END;
				break;
			}
			break;
			
			
		case SRCH_STR:
			srch = cur;
			for (; *cur != '/' && *cur != '\n' && *cur != '\0'; cur++)
			{
				if(*cur != '\\') 
					continue;
				
				cur = fixesc(cur);
			}
			if (*cur == '\n')
			{
				*cur = '\0';
				state = END;
			}
			else if (*cur == '/')
			{
				state = REPL_STR;
				*cur = '\0';
				cur++;
			}
			else
				state = END;
				
			break;
			
		case REPL_STR:
			repl = cur;
			for (; *cur != '\n' && *cur != '\0'; cur++)
			{
				if(*cur != '\\') 
					continue;
				
				cur = fixesc(cur);
			}
			if (*cur == '\n')
				*cur = '\0';
			
			state = END;
				
			break;		
		}
	}
	fixlines();
}


int numlength(long num)
{
	if (num >= 1000000000) return 10;
	if (num >= 100000000 ) return 9;
	if (num >= 10000000  ) return 8;
	if (num >= 1000000   ) return 7;
	if (num >= 100000    ) return 6;
	if (num >= 10000     ) return 5;
	if (num >= 1000      ) return 4;
	if (num >= 100       ) return 3;
	if (num >= 10        ) return 2;
	if (num >= 0         ) return 1;
	if (num > -10        ) return 2; // Negative include - sign
	if (num > -100       ) return 3;
	if (num > -1000      ) return 4;
	if (num > -10000     ) return 5;
	if (num > -100000    ) return 6;
	if (num > -1000000   ) return 7;
	if (num > -10000000  ) return 8;
	if (num > -100000000 ) return 9;
	if (num > -1000000000) return 10;
	return 11;
}
void docmd(void)
{
	int n;
	cmd = toupper(cmd);
	
	
	
	switch(cmd)
	{
	case '\0': // Change curline (or list if range given)
		if (eltype != NONE)
			listlines();
		else
		{
			curline = (startl <= 0) ? 1 : startl;
			listline(curline);
		}
		break;
	case 'S': // Search
		break;
	case 'R': // Replace
		break;
	case 'N': // Next match for search
		break;
	case 'I': // Insert at current line (push line forward)
		insertmode(0);
		break;
	case 'A': // Append after current line
		insertmode(1);
		break;
	case 'D': // Delete line
		if (endl <= startl && startl <= fb.numlines)
			filebuf_delete(&fb, startl);
		else
		{ 
			for (int i = startl; i <= endl; i++)
			{
				printf("Deleting %d\n", i);
				filebuf_delete(&fb, startl);
			}
		}
		break;
	case 'L': // List line(s)
		listlines();
				
		break;
	case 'P': // List line(s) moving curline to last line listed 
		listlines();
		curline = (endl > fb.numlines) ? fb.numlines : endl;
		
		break;
	case 'W': // Write
		n = filebuf_save(&fb, filename);
		if (n < 0)
		{
			fprintf(stderr, "Failed to save to file '%s'\n", filename);
		}
		else
		{
			fprintf(stderr, "Wrote %d lines to '%s'\n", n, filename);
		}
		break;
	case 'Q': // Quit
		running = 0;
		break;
	}
}


char *fixesc(char *start)
{
	char buf[3];
	char *end = start+1;
	char *cur, *nxt;
	switch (*end)
	{
	case 'a':  *start = '\a'; break;
	case 'b':  *start = '\b'; break;
	case 'f':  *start = '\f'; break;
	case 'n':  *start = '\n'; break;
	case 'r':  *start = '\r'; break;
	case 't':  *start = '\t'; break;
	case 'v':  *start = '\v'; break;
	case '\\': *start = '\\'; break;
	case '/':  *start = '/';  break;
	case 'x': // hexadecimal escape
		buf[0] = *++end; buf[1] = *++end; buf[2] = '\0';
		*start = (char)strtol(&buf[0], NULL, 16);
		break;
	}
	// Now fixup the rest of the string
	for(cur = start+1, nxt = end+1; *cur++ = *nxt++;)
		;
		
	return end;
}




void fixlines(void)
{
	switch (sltype)
	{
	case NONE:    startl = curline; break;
	case CURLINE: startl = curline; break;
	case RELLINE: startl += curline; break;
	case ENDLINE: startl = fb.numlines; break;
	case NUMBER:  break;
	}
	
	switch (eltype)
	{
	case NONE:    endl = 0; break;
	case CURLINE: endl = curline; break;
	case RELLINE: endl += curline; break;
	case ENDLINE: endl = fb.numlines; break;
	case NUMBER:  break;
	}
	
	if (startl <= 0) startl = 1;
	if (endl   <= 0) endl   = 1;
	if (startl > fb.numlines) startl = fb.numlines;
	if (endl   > fb.numlines) endl = fb.numlines;
}



void insertmode(int append)
{
	char linebuf[LINESIZE];
	
	if(append && curline <= fb.numlines)
		curline++;
	
	for(;;)
	{
		printf(" %*d: ", numlength(curline)+1, curline);

		fgets(&linebuf[0], LINESIZE, stdin);
		linebuf[ strcspn(linebuf, "\n") ] = '\0';
		
		if (strncmp(linebuf, ".", 2) == 0)
			break;
			
		filebuf_insert(&fb, linebuf, curline-1);
		curline++;
	}		
}


void listlines(void)
{

	if (startl <= 0 || fb.numlines == 0)
		return;
		
	if (endl <= startl && startl <= fb.numlines)
		listline(startl);
	else
	{ 
		for (int i = startl; i <= endl && i <= fb.numlines; i++)
			listline(i);
	}
}


void listline(long pos)
{
	printf(" %c%*d: %s\n", (pos == curline)?'*':' ', numlength(pos), pos, fb.lines[pos-1]);
}
