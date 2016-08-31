#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define LINESIZE 512

/**************************
 * Global State/Variables */
int running = 1;
long curline = 0; 
char linebuf[LINESIZE];
enum { NONE, CURLINE, RELLINE, ENDLINE, NUMBER } sltype, eltype;
long startl  = 0;		
long endl    = 0;
char cmd    = 0;
char *srch  = NULL;
char *repl  = NULL;


void usage(char *pname, char *msg);
void prompt(void);
void getinput(void);
void parse(void);
void docmd(void);
char *fixesc(char *start);



int main(int argc, char **argv)
{
	
	if (argc < 2)
	{
		usage(argv[0], "Too few arguments");
		return 1;
	}
	
	while (running)
	{
		prompt();
		getinput();
		parse();
		
		printf("DEBUG: sltype = %d eltype = %d startl = %d endl = %d cmd = %c srch = %s repl = %s\n",
			sltype, eltype, startl, endl, cmd, srch, repl);
		docmd();
	}
	
	
	
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
	fgets(&linebuf[0], LINESIZE, stdin);
}


void parse(void)
{
	char *endptr;
	char *cur;
	enum { R_START=0, R_END, CMD, SRCH_STR, REPL_STR, END } state;
	sltype = NONE; eltype = NONE;
	startl = 0; endl = 0;
	cmd = '\0';
	srch = NULL; repl = NULL;
	
	cur = &linebuf[0];
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
				state = CMD;
			
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
}


void docmd(void)
{
	
	
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
