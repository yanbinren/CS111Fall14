// UCLA CS 111 Lab 1 command printing, for debugging

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <stdlib.h>


static void
command_indented_print (int indent, command_t c)
{
    int index=0;
  switch (c->type)
    {
    case IF_COMMAND:
    case UNTIL_COMMAND:
    case WHILE_COMMAND:
      printf ("%*s%s\n", indent, "",
	      (c->type == IF_COMMAND ? "if"
	       : c->type == UNTIL_COMMAND ? "until" : "while"));
      command_indented_print (indent + 2, c->u.command[0]);
      printf ("\n%*s%s\n", indent, "", c->type == IF_COMMAND ? "then" : "do");
      command_indented_print (indent + 2, c->u.command[1]);
      if (c->type == IF_COMMAND && c->u.command[2])
	{
	  printf ("\n%*selse\n", indent, "");
	  command_indented_print (indent + 2, c->u.command[2]);
	}
      printf ("\n%*s%s", indent, "", c->type == IF_COMMAND ? "fi" : "done");
      break;
    
    case SEQUENCE_COMMAND:
    case PIPE_COMMAND:
      {
	command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				c->u.command[0]);
	char separator = c->type == SEQUENCE_COMMAND ? ';' : '|';
	printf (" \\\n%*s%c\n", indent, "", separator);
	command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				c->u.command[1]);
	break;
      }
    case SIMPLE_COMMAND:
      {
	char **w = c->u.word;
	printf ("%*s%s", indent, "", *w);
	while (*++w)
	  printf (" %s", *w);
	break;
      }

    case SUBSHELL_COMMAND:
      printf ("%*s(\n", indent, "");
      command_indented_print (indent + 1, c->u.command[0]);
      printf ("\n%*s)", indent, "");
      break;
    case AND_COMMAND:
    case OR_COMMAND:
    {
        command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
                                    c->u.command[0]);
        char separator = c->type == AND_COMMAND ? '&' : '|';
        printf (" \\\n%*s%c%c\n", indent, "", separator,separator);
        command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
                                    c->u.command[1]);
        break;
    }
    case NOT_COMMAND:
        printf ("%*s!\n", indent, "");
        command_indented_print (indent + 1, c->u.command[0]);
        break;
    case FOR_COMMAND:
            printf ("%*s%s\n", indent, "", "for");
            command_indented_print (indent + 2, c->u.command[0]);
            printf ("\n%*s%s\n", indent, "", "in");
            command_indented_print (indent + 2, c->u.command[1]);
            if (c->u.command[2])
        {
            printf ("\n%*sdo\n", indent, "");
            command_indented_print (indent + 2, c->u.command[2]);
        }
            printf ("\n%*s%s", indent, "", "done");
            break;
            
    case CASE_COMMAND:
            printf ("%*s%s\n", indent, "", "case");
            command_indented_print (indent + 2, c->u.casecmd[index++]);
            printf ("\n%*s%s\n", indent, "", "in");
            command_indented_print (indent + 2, c->u.casecmd[index++]);
            printf (")\n");
            //printf ("\n%*s)\n", indent, "");
            command_indented_print (indent + 4, c->u.casecmd[index++]);
            printf ("\n%*s%s\n", indent, "", ";;");
            while (1) {
                int next_condition=index++;
                int next_command=index++;
                if(c->u.casecmd[next_condition]){
                    command_indented_print (indent + 2, c->u.casecmd[next_condition]);
                    printf (")\n");
                    //printf ("\n%*s%s", indent, "", ")");
                    command_indented_print (indent + 4, c->u.casecmd[next_command]);
                    printf ("\n%*s%s\n", indent, "", ";;");
                }else
                    break;
            }
            printf ("%*s%s", indent, "", "esac");
            break;
            
    default:
      abort ();
    }

  if (c->input)
    printf ("<%s", c->input);
  if (c->output)
    printf (">%s", c->output);
}

void
print_command (command_t c)
{
  command_indented_print (2, c);
  putchar ('\n');
}
