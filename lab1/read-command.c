// UCLA CS 111 Lab 1 command reading

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

#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

static int NUM_LINE = 1; //record the number of line currently in
static size_t BUFFER_SIZE = 0; //store the total length of the file string

//connext every comman with a linked list
struct command_node
{
	struct command* command;
	struct command_node* next;
};
//command stream used to record the head of the stream and the current position of the string
struct command_stream
{
	struct command_node* head;
	struct command_node* cursor;
};

//fuctions declarations
bool is_valid_char(char c); //determine if a charactor is valid
bool is_special_char(char c); //determine if a charactor is special token
char *get_next_word(char *start, size_t *index); //get next word in the buffer with index moving
char *read_next_token(char *start, size_t *index, size_t size); //check next word without index moving
char *read_next_token2(char *start, size_t *index, size_t size); //check next word with index moving
void read_comments(char *start, size_t *index); //ignore the comments (after #)
enum command_type get_command_type(char *start, size_t *index, size_t size); //get the next command type
command_stream_t initialize_command_stream(); //initialize command string
command_t initialize_command(enum command_type type); //initialize command corresponding to command type
command_t create_if_command(char *start, size_t *index); //create IF COMMAND
command_t create_pipe_command(char *start, size_t *index); //create PIPE COMMAND
command_t create_sequence_command(char *start, size_t *index); //create SEQUENCE COMMAND
command_t create_simple_command(char * start, size_t *index); //create SIMPLE COMMAND
command_t create_subshell_command(char *start, size_t *index); //create SUBSHELL COMMAND
command_t create_until_command(char *start, size_t *index); //create UNTIL COMMAND
command_t create_while_command(char *start, size_t *index); //create WHILE COMMAND

//functions definitions here:
bool
is_valid_char(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
    || c == '%' || c == '+' || c == ',' || c == '-' || c == '.' || c == '!'
    || c == '/' || c == ':' || c == '@' || c == '^' || c == '_' || c == '*';
}

bool
is_special_char(char c)
{
	return c == ';' || c == '|' || c == '(' || c == ')'
    || c == '<' || c == '>';
}

char *
get_next_word(char *start, size_t *index)
{
    //eliminate the space, tab and comment (which is after #)
    while ((*(start + *index) == ' ' || *(start + *index) == '\t' || *(start + *index) == '#'
            || *(start+ *index) == '\n' || *(start + *index) == '\\') && *index < BUFFER_SIZE)
    {
        if (*(start + *index) == '#' || *(start + *index) == '\\')
        {
            while (*(start + *index) != EOF)
            {
                if (*(start + *index) == '\n'){ NUM_LINE++; break; }
                (*index)++;
            }
        }
        if (*(start + *index) == '\n') NUM_LINE++;
        (*index)++;
    }
    //make the pointer temp point to the end of the word
    char *temp = start + *index;
    while (is_valid_char(*temp) && *temp != EOF) temp++;
    //declar next_token to store the word detected
    if (temp - start - *index ==0 ) {  return NULL; }
    char *next_word = (char*)checked_malloc(sizeof(char) * (temp - start - *index));
    char *word = next_word;
    while (start + *index != temp) *word++ = *(start + (*index)++);
    *word = '\0'; //end of the word
    if (strlen(next_word)==0) return NULL;
    return next_word;
}

//read next token without moving forward the global buffer pointer *index
char *
read_next_token(char *start, size_t *index, size_t size){
    if(*index>= size) return NULL;
    size_t tmp = *index;
    int count = 0;
    while ((start[tmp] == ' ' || start[tmp] == '\t' || start[tmp] == '\n') && tmp < size) {
        ++tmp;
    }
    while (start[tmp + count] != ' ' && start[tmp + count] != '\t' && start[tmp + count] != '\n' && start[tmp + count] != EOF && tmp + count < size) {
        ++count;
    }
    char *token = (char *)checked_malloc((count + 1)*sizeof(char));
    char *ret = token;
    while (count > 0) {
        *(token++) = start[tmp++];
        --count;
    }
    *token = '\0';
    return ret;
}


//read next word and move forward the global buffer pointer *index to ' ' or '\t' or '\n' after word
char *
read_next_token2(char *start, size_t *index, size_t size){
    int count = 0;
    while (start[*index] == ' ' || start[*index] == '\t' || start[*index] == '\n') {
        if(start[*index] == '\n'){
            return NULL;
        }
        ++(*index);
    }
    while (start[(*index)] != ' ' && start[(*index)] != '\t' && start[(*index)] != '\n' && start[(*index)] != EOF && (*index) < size) {
        ++count;
        ++(*index);
    }
    if(count==0) return NULL;
    char *token = (char *)checked_malloc((count + 1)*sizeof(char));
    char *ret = token;
    (*index)-=count;
    while (count > 0) {
        *(token++) = start[(*index)++];
        --count;
    }
    *token = '\0';
    return ret;
}

//parsing whole-line comments with a '#'
void
read_comments(char *start, size_t *index){
	char c = start[*index];
	while (c != '\n' && *index < BUFFER_SIZE) {
		(*index)++;
		c = start[*index];
	}
    ++NUM_LINE;
}


//judging the command type
enum command_type get_command_type(char *start, size_t *index,size_t size){
	char c = start[*index];
	enum command_type type = SIMPLE_COMMAND;
	for (; *index < size; (*index)++) {
        c = start[*index];
        while ((c == ' ' || c == '\t') && *index < size) {
			++(*index);
            c=start[*index];
		}
        if (c == '\n'){
			++NUM_LINE;
			continue;
		}
		if ((is_valid_char(c) != true) && (is_special_char(c) != true))
		{
			fprintf(stderr, "Line %d: Invalid syntax: %c.\n", NUM_LINE,c);
			exit(1);
		}
		else{
            char *word = read_next_token(start, index, size);
            if(word==NULL) continue;
            if (*word=='('){
                type = SUBSHELL_COMMAND;
                return type;
            }
            else if (strcmp(word, "if") == 0){
                type = IF_COMMAND;
                return type;
            }
            else if (strcmp(word, "while") == 0){
                type = WHILE_COMMAND;
                return type;
            }
            else if (strcmp(word, "until") == 0){
                type = UNTIL_COMMAND;
                return type;
            }
            else if (strcmp(word, "do") == 0){
                fprintf(stderr, "Line %d: Unexpected \'do\', missing \'until\'/\'while\'/\'for\'\n", NUM_LINE);
                exit(1);
            }
            else if (strcmp(word, "done") == 0){
                fprintf(stderr, "Line %d: Unexpected \'done\', missing \'until\'/\'while\'/\'for\'\n", NUM_LINE);
                exit(1);
            }
            else if (strcmp(word, "else") == 0){
                fprintf(stderr, "Line %d: Unexpected \'else\', missing \'if\'\n", NUM_LINE);
                exit(1);
            }
            else if (strcmp(word, "fi") == 0){
                fprintf(stderr, "Line %d: Unexpected \'fi\', missing \'if\'\n", NUM_LINE);
                exit(1);
            }
            else if (strcmp(word, "then") == 0){
                fprintf(stderr, "Line %d: Unexpected \'then\', missing \'if\'\n", NUM_LINE);
                exit(1);
            }
            else if (*word== ')'){
                fprintf(stderr, "Line %d: Unexpected \')\', missing \'(\'\n", NUM_LINE);
                exit(1);
            }
            else {
                //jugding for pipeline command
                if (strchr(word, '|')) {
                    char *pipeline=strchr(word, '|');
                    if(*(pipeline+1) != '|') return PIPE_COMMAND;
                }
                size_t tmp=*index;
                word=read_next_token2(start,&tmp,size);
                while(word!=NULL){
                    if (strcmp(word, "if") == 0 || strcmp(word, "else") == 0 || strcmp(word, "then") == 0 || strcmp(word, "while") == 0 || strcmp(word, "until") == 0 || strcmp(word, "do") == 0 || strcmp(word, "done") == 0 || strcmp(word, "fi") == 0 || word == NULL ) break;
                    if (strchr(word, '|')) {
                        char *pipeline=strchr(word, '|');
                        if(*(pipeline+1) != '|') return PIPE_COMMAND;
                        else word=read_next_token2(start,&tmp,size);
                    }
                    else word=read_next_token2(start,&tmp,size);
                }
                //jugding for sequence command
                word = read_next_token(start, index,size);
                if (strchr(word, ';')) return SEQUENCE_COMMAND;
                tmp=*index;
                word=read_next_token2(start,&tmp,size);
                while(word!=NULL){
                    if (strcmp(word, "if") == 0 || strcmp(word, "else") == 0 || strcmp(word, "then") == 0 || strcmp(word, "while") == 0 || strcmp(word, "until") == 0 || strcmp(word, "do") == 0 || strcmp(word, "done") == 0 || strcmp(word, "fi") == 0 || strcmp(word, "in") == 0 || strcmp(word, "esac") == 0 || word == NULL ) break;
                    if (strchr(word, ';')) return SEQUENCE_COMMAND;
                    else word=read_next_token2(start,&tmp,size);
                }
            }
        }
        return type;
	}
	return type;
}

command_stream_t
initialize_command_stream()
{
	command_stream_t stream = (command_stream_t)checked_malloc(sizeof(struct command_stream));
	stream->head = NULL;
	stream->cursor = NULL;
	return stream;
}

command_t
initialize_command(enum command_type type)
{
	command_t cmd = (command_t)checked_malloc(sizeof(struct command));
	cmd->type = type;
	cmd->status = -1;
	cmd->input = NULL;
	cmd->output = NULL;
    cmd->u.word = NULL;
	return cmd;
}


command_t
create_if_command(char *start, size_t *index)
{
	char *temp = get_next_word(start, index); //pass the "if"
	command_t cmd = initialize_command(IF_COMMAND);
	//store the command between "if" and "then"
	enum command_type type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[0] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[0] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[0] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[0] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[0] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[0] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[0] = create_while_command(start, index); break;
	}
	//if no "then", error
	if ((*index) == BUFFER_SIZE){
		fprintf(stderr, "Line %d: Incomplete if statement, missing \'then\'\n", NUM_LINE);
		exit(1);
	}
	temp = get_next_word(start, index);
    if (temp == NULL) {
		fprintf(stderr, "Line %d: Incomplete if statement, missing \'then\'\n", NUM_LINE);
		exit(1);
	}
	if (strcmp(temp, "then") != 0)
	{
		fprintf(stderr, "Line %d: Incomplete if statement, missing \'then\'\n", NUM_LINE);
		exit(1);
	}
	//store the command between "then" and ("fi" or "else")
	type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[1] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[1] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[1] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[1] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[1] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[1] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[1] = create_while_command(start, index); break;
	}
	//if neither "fi" nor "else", error
	if ((*index) == BUFFER_SIZE){
		fprintf(stderr, "Line %d: Incomplete if statement, missing \'else\' or \'fi\'\n", NUM_LINE);
		exit(1);
	}
	temp = get_next_word(start, index);
	if (temp == NULL) {
		fprintf(stderr, "Line %d: Incomplete if statement, missing \'fi\'\n", NUM_LINE);
		exit(1);
	}
	if (strcmp(temp, "else") != 0 && strcmp(temp, "fi") != 0)
	{
		fprintf(stderr, "Line %d: Incomplete conditional statement, missing \'fi\' or \'else\'\n", NUM_LINE);
		exit(1);
	}
	if (strcmp(temp, "fi") == 0) return cmd;
	//store the command between "else" and "fi"
	type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[2] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[2] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[2] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[2] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[2] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[2] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[2] = create_while_command(start, index); break;
	}
	temp = get_next_word(start, index);
	if (temp == NULL) {
		fprintf(stderr, "Line %d: Incomplete if statement, missing \'fi\'\n", NUM_LINE);
		exit(1);
	}
	if (strcmp(temp, "fi") != 0)
	{
		fprintf(stderr, "Line %d: Incomplete if statement, missing \'fi\'\n", NUM_LINE);
		exit(1);
	}
    if (start[*index] != '\n'){
        temp = read_next_token(start, index, BUFFER_SIZE);
        if (*temp == '<') {
            if (cmd->input)  //no more than one input allower
            {
                fprintf(stderr, "Line %d: More than one input.\n", NUM_LINE);
                exit(1);
            }
            temp = get_next_word(start, index);
            (*index)++;
            cmd->input = get_next_word(start, index);
            if (!cmd->input) // if input is still empty, error
            {
                fprintf(stderr, "Line %d: Invalid input filename: missing input file.\n", NUM_LINE);
                exit(1);
            }
        }
        else if (*temp == '>') {
            if (cmd->output) //no more than one output allowed
            {
                fprintf(stderr, "Line %d: More than one output.\n", NUM_LINE);
                exit(1);
            }
            temp = get_next_word(start, index);
            cmd->output = (char *)checked_malloc(sizeof(char));
            (*index)++;
            cmd->output = get_next_word(start, index);
            if (!cmd->output) // if output is still empty, error
            {
                fprintf(stderr, "Line %d: Invalid output filename: missing output file.\n", NUM_LINE);
                exit(1);
            }
        }
        /*else{
         fprintf(stderr, "Line %d: Redundancy after cmd \'done\'", NUM_LINE);
         exit(1);
         }*/
    }
	return cmd;
}

command_t
create_pipe_command(char *start, size_t *index)
{
    ////printf("*ppl*\n");
	command_t cmd = initialize_command(PIPE_COMMAND);
	int i = 0;
	while (!(*(start + *index + i) == '|' && *(start + *index + i + 1) != '|')) i++;
	enum command_type type = get_command_type(start, index, *index + i);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[0] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[0] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[0] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[0] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[0] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[0] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[0] = create_while_command(start, index); break;
	}
	while (*(start + *index) != EOF && *(start + *index) != '|')
	{
		if (*(start + *index) == '\n') NUM_LINE++;
		(*index)++;
	}
	(*index)++;
	type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[1] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[1] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[1] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[1] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[1] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[1] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[1] = create_while_command(start, index); break;
	}
	return cmd;
}

command_t
create_sequence_command(char *start, size_t *index)
{
	command_t cmd = initialize_command(SEQUENCE_COMMAND);
	int i = 0;
	while (*(start + *index + i) != ';') i++;
    if(i == 0) {
        fprintf(stderr, "Line %d: Missing command before \';\'\n", NUM_LINE);
        exit(1);
    }
	enum command_type type = get_command_type(start, index, *index + i);
	switch (type){
        case IF_COMMAND: cmd->u.command[0] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[0] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[0] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[0] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[0] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[0] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[0] = create_while_command(start, index); break;
	}
	while (*(start + *index) != EOF && *(start + *index) != ';')
	{
		if (*(start + *index) == '\n') NUM_LINE++;
		if (*(start + *index) == ')')
		{
			fprintf(stderr, "Line %d: Missing \'(\'\n", NUM_LINE);
			exit(1);
		}
		(*index)++;
	}
	(*index)++;
	char *temp = read_next_token(start, index, BUFFER_SIZE);
	if (strcmp(temp, "do") == 0 || strcmp(temp, "then") == 0 || strcmp(temp, "done") == 0 || strcmp(temp, "else") == 0 || strcmp(temp, "fi") == 0){
		return cmd->u.command[0];
	}
	type = get_command_type(start, index, BUFFER_SIZE);
	switch (type){
        case IF_COMMAND: cmd->u.command[1] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[1] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[1] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[1] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[1] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[1] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[1] = create_while_command(start, index); break;
	}
	return cmd;
}

command_t
create_simple_command(char *start, size_t *index)
{
	command_t cmd = initialize_command(SIMPLE_COMMAND);
	// defauted size of word is one char* now
	cmd->u.word = (char**)checked_malloc(sizeof(char*));
	cmd->u.word[0] = NULL;
	bool write = false;
	int num_words = 0, size_words = 1;
	while (*index < BUFFER_SIZE)
	{
        //printf("%zu %c simple\n", *index, start[*index]);
		//ignor the space or tab and move on
		if (*(start + *index) == ' ' || *(start + *index) == '\t') {
			(*index)++; continue;
		}
		//if #, eliminate the comments
		else if (*(start + *index) == '#')
		{
			while (*(start + *index) != EOF)
			{
				if (*(start + *index) == '\n'){ NUM_LINE++; break; }
				(*index)++;
			}
		}
		//number of lines increased by one and end the loop
		else if (*(start + *index) == '\n') { NUM_LINE++; (*index)++; break; }
		//if a single legal charactor detected, get the next word stored in u.word
		else if (is_valid_char(*(start + *index)))
		{
			write = true;
			char *temp = get_next_word(start, index);
			if (strcmp(temp, "if") == 0 || strcmp(temp, "then") == 0 || strcmp(temp, "else") == 0
				|| strcmp(temp, "fi") == 0 || strcmp(temp, "while") == 0 || strcmp(temp, "until") == 0
				|| strcmp(temp, "done") == 0 || strcmp(temp, "do") == 0 )
			{
				*index -= strlen(temp);
				break;
			}
			//resize the u.word when the number of words goes to up limit
			if (num_words == size_words)
			{
				size_words *= 2;
				cmd->u.word = checked_realloc(cmd->u.word, size_words * sizeof(char*));
			}
			cmd->u.word[num_words++] = temp;
			cmd->u.word[num_words] = NULL;
            //if (*(start + *index) == EOF) break;
			continue;
		}
		//if <, put the name into input of command
		else if (*(start + *index) == '<')
		{
			if (cmd->input)  //no more than one input allower
			{
				fprintf(stderr, "Line %d: More than one input.\n", NUM_LINE);
				exit(1);
			}
			(*index)++;
			cmd->input = get_next_word(start, index);
			if (!cmd->input) // if input is still empty, error
			{
				fprintf(stderr, "Line %d: Invalid input filename: missing input file.\n", NUM_LINE);
				exit(1);
			}
			continue;
		}
		//if <, put the name into output of command
		else if (*(start + *index) == '>')
		{
			if (cmd->output) //no more than one output allowed
			{
				fprintf(stderr, "Line %d: More than one output.\n", NUM_LINE);
				exit(1);
			}
			(*index)++;
			cmd->output = get_next_word(start, index);
			if (!cmd->output) // if output is still empty, error
			{
				fprintf(stderr, "Line %d: Invalid output filename: missing output file.\n", NUM_LINE);
				exit(1);
			}
			continue;
		}
		// if it is special character, end command
		else if (is_special_char(*(start + *index))) break;
		// otherwise the character might be invalid, error
		else
		{
			fprintf(stderr, "Line %d: Invalid syntax\n", NUM_LINE);
			exit(1);
		}
		(*index)++;
	}
	if (!write)
	{
		fprintf(stderr, "Line %d: NULL Command\n", NUM_LINE);
		exit(1);
	}
	return cmd;
}

command_t
create_subshell_command(char *start, size_t *index){
    command_t cmd = initialize_command(SUBSHELL_COMMAND);
    (*index)++;
    enum command_type type = get_command_type(start, index, BUFFER_SIZE);
    switch (type)
    {
        case IF_COMMAND: cmd->u.command[0] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[0] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[0] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[0] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[0] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[0] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[0] = create_while_command(start, index); break;
    }
    while (*(start + *index) != EOF && *(start + *index) != ')') {
        if (*(start + *index) == '\n') {
            ++NUM_LINE;
            fprintf(stderr, "Line %d: Missing \')\'\n", NUM_LINE);
            exit(1);
        }
        if (*(start + *index + 1) == EOF)
        {
            fprintf(stderr, "Line %d: Missing \')\'\n", NUM_LINE);
            exit(1);
        }
        (*index)++;
    }
    (*index)++;
    if (start[*index] != '\n'){
        char *temp = read_next_token(start, index, BUFFER_SIZE);
        if (*temp == '<') {
            if (cmd->input)  //no more than one input allower
            {
                fprintf(stderr, "Line %d: More than one input.\n", NUM_LINE);
                exit(1);
            }
            temp = get_next_word(start, index);
            (*index)++;
            cmd->input = get_next_word(start, index);
            if (!cmd->input) // if input is still empty, error
            {
                fprintf(stderr, "Line %d: Invalid input filename: missing input file.\n", NUM_LINE);
                exit(1);
            }
        }
        else if (*temp == '>') {
            if (cmd->output) //no more than one output allowed
            {
                fprintf(stderr, "Line %d: More than one output.\n", NUM_LINE);
                exit(1);
            }
            temp = get_next_word(start, index);
            cmd->output = (char *)checked_malloc(sizeof(char));
            (*index)++;
            cmd->output = get_next_word(start, index);
            if (!cmd->output) // if output is still empty, error
            {
                fprintf(stderr, "Line %d: Invalid output filename: missing output file.\n", NUM_LINE);
                exit(1);
            }
        }
        /*else{
         fprintf(stderr, "Line %d: Redundancy after cmd \'done\'", NUM_LINE);
         exit(1);
         }*/
    }
    return cmd;
}

command_t
create_until_command(char *start, size_t *index)
{
	char *temp = get_next_word(start, index); //pass the "until"
	command_t cmd = initialize_command(UNTIL_COMMAND);
	//store the command between "until" and "then"
	enum command_type type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[0] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[0] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[0] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[0] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[0] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[0] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[0] = create_while_command(start, index); break;
	}
	//if no "do", error
	if ((*index) == BUFFER_SIZE){
		fprintf(stderr, "Line %d: Incomplete until statement, missing \'do\'\n", NUM_LINE);
		exit(1);
	}
	temp = get_next_word(start, index);
	if (strcmp(temp, "do") != 0)
	{
		fprintf(stderr, "Line %d: Incomplete until statement, missing \'do\'\n", NUM_LINE);
		exit(1);
	}
	//store the command between "do" and "done"
	type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[1] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[1] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[1] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[1] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[1] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[1] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[1] = create_while_command(start, index); break;
	}
	//if no "done", error
	if ((*index) == BUFFER_SIZE){
		fprintf(stderr, "Line %d: Incomplete while statement, missing \'done\'\n", NUM_LINE);
		exit(1);
	}
	temp = get_next_word(start, index);
	if (strcmp(temp, "done") != 0)
	{
		fprintf(stderr, "ERROR at Line %d: Incomplete until statement, missing \'done\'\n", NUM_LINE);
		exit(1);
	}
    if (start[*index] != '\n'){
        temp = read_next_token(start, index, BUFFER_SIZE);
        if (*temp == '<') {
            if (cmd->input)  //no more than one input allower
            {
                fprintf(stderr, "Line %d: More than one input.\n", NUM_LINE);
                exit(1);
            }
            temp = get_next_word(start, index);
            (*index)++;
            cmd->input = get_next_word(start, index);
            if (!cmd->input) // if input is still empty, error
            {
                fprintf(stderr, "Line %d: Invalid input filename: missing input file.\n", NUM_LINE);
                exit(1);
            }
        }
        else if (*temp == '>') {
            if (cmd->output) //no more than one output allowed
            {
                fprintf(stderr, "Line %d: More than one output.\n", NUM_LINE);
                exit(1);
            }
            temp = get_next_word(start, index);
            cmd->output = (char *)checked_malloc(sizeof(char));
            (*index)++;
            cmd->output = get_next_word(start, index);
            if (!cmd->output) // if output is still empty, error
            {
                fprintf(stderr, "Line %d: Invalid output filename: missing output file.\n", NUM_LINE);
                exit(1);
            }
        }
        /*else{
         fprintf(stderr, "Line %d: Redundancy after cmd \'done\'", NUM_LINE);
         exit(1);
         }*/
    }
	return cmd;
}

command_t
create_while_command(char *start, size_t *index)
{
	char *temp = get_next_word(start, index); //pass the "while"
	command_t cmd = initialize_command(WHILE_COMMAND);
	//store the command between "while" and "do"
	enum command_type type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[0] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[0] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[0] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[0] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[0] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[0] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[0] = create_while_command(start, index); break;
	}
	//if no "do", error
	if ((*index) == BUFFER_SIZE){
		fprintf(stderr, "Line %d: Incomplete while statement, missing \'do\'\n", NUM_LINE);
		exit(1);
	}
	temp = get_next_word(start, index);
	if (strcmp(temp, "do") != 0)
	{
		(*index) -= strlen(temp);
		command_t condition = initialize_command(cmd->u.command[0]->type);
		condition = cmd->u.command[0];
		cmd->u.command[0] = initialize_command(SEQUENCE_COMMAND);
		cmd->u.command[0]->u.command[0] = condition;
		cmd->u.command[0]->u.command[1] = create_simple_command(start, index);
		temp = get_next_word(start, index);
	}
	if (strcmp(temp, "do") != 0)
	{
		fprintf(stderr, "Line %d: Incomplete while statement, missing \'do\'\n", NUM_LINE);
		exit(1);
	}
	//store the command between "do" and "done"
	type = get_command_type(start, index, BUFFER_SIZE);
	switch (type)
	{
        case IF_COMMAND: cmd->u.command[1] = create_if_command(start, index); break;
        case PIPE_COMMAND: cmd->u.command[1] = create_pipe_command(start, index); break;
        case SEQUENCE_COMMAND: cmd->u.command[1] = create_sequence_command(start, index); break;
        case SIMPLE_COMMAND: cmd->u.command[1] = create_simple_command(start, index); break;
        case SUBSHELL_COMMAND: cmd->u.command[1] = create_subshell_command(start, index); break;
        case UNTIL_COMMAND: cmd->u.command[1] = create_until_command(start, index); break;
        case WHILE_COMMAND: cmd->u.command[1] = create_while_command(start, index); break;
	}
	//if no "done", error
	if ((*index) == BUFFER_SIZE){
		fprintf(stderr, "Line %d: Incomplete while statement, missing \'done\'\n", NUM_LINE);
		exit(1);
	}
	temp = get_next_word(start, index);
	if (strcmp(temp, "done") != 0)
	{
		fprintf(stderr, "Line %d: Incomplete while statement, missing \'done\'\n", NUM_LINE);
		exit(1);
	}
	if (start[*index] != '\n'){
		temp = read_next_token(start, index, BUFFER_SIZE);
		if (*temp == '<') {
			temp = get_next_word(start, index);
			cmd->input = (char *)checked_malloc(sizeof(char));
			(*index)++;
			cmd->input = get_next_word(start, index);
		}
		else if (*temp == '>') {
			temp = get_next_word(start, index);
			cmd->output = (char *)checked_malloc(sizeof(char));
			(*index)++;
			cmd->output = get_next_word(start, index);
		}
		else{
			fprintf(stderr, "Line %d: Redundancy after cmd \'done\'", NUM_LINE);
			exit(1);
		}
	}
	return cmd;
}

command_stream_t
make_command_stream(int(*get_next_byte) (void *),
                    void *get_next_byte_argument)
{
	/* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
	size_t bufferLength = 1024;
	size_t ptr = 0;
	char* buffer = (char*)checked_malloc(bufferLength);
	int c = get_next_byte(get_next_byte_argument);
	while (c != EOF) {
		buffer[ptr++] = c;
		if (ptr == bufferLength){
			buffer = (char*)checked_grow_alloc(buffer, &bufferLength);
		}
		c = get_next_byte(get_next_byte_argument);
	}
    buffer[ptr]='\0';
	command_stream_t cst = (command_stream_t)checked_malloc(sizeof(struct command_stream));
	cst->head = NULL;
	cst->cursor = NULL;
	size_t start = 0;
	size_t *index = &start;
	BUFFER_SIZE = ptr;
	command_node_t cmdnode = (command_node_t)checked_malloc(sizeof(struct command_node));
	for (; *index < ptr; ) {
		//
        //printf ("index:%zu\n",*index);
        char c=buffer[*index];
        while ((c == ' ' || c == '\t') && *index < BUFFER_SIZE) {
			++(*index);
            c=buffer[*index];
		}
        if (c == '\n'){
			++NUM_LINE;
            (*index)++;
			continue;
		}
        if (c == '#'){ //comments
            read_comments(buffer, index);
            (*index)++;
            continue;
        }
		enum command_type type = get_command_type(buffer, index, BUFFER_SIZE);
		command_t cmd = NULL;
		switch (type) {
            case IF_COMMAND:
                cmd = create_if_command(buffer, index);
                break;
            case PIPE_COMMAND:
                cmd = create_pipe_command(buffer, index);
                break;
            case SEQUENCE_COMMAND:
                cmd = create_sequence_command(buffer, index);
                break;
            case SIMPLE_COMMAND:
                cmd = create_simple_command(buffer, index);
                break;
            case SUBSHELL_COMMAND:
                cmd = create_subshell_command(buffer, index);
                break;
            case UNTIL_COMMAND:
                cmd = create_until_command(buffer, index);
                break;
            case WHILE_COMMAND:
                cmd = create_while_command(buffer, index);
                break;
            default:
                fprintf(stderr, "Line %d: Wrong CMD Type.\n", NUM_LINE);
		}
		//
		//command_t cmd = checked_malloc(sizeof(struct command));
        
		cmdnode->command = cmd;
		command_node_t cmdnode2 = checked_malloc(sizeof(struct command_node));
		cmdnode->next = cmdnode2;
        
		if (cst->head == NULL){
			cst->head = cmdnode;
			cst->cursor = cmdnode;
		}
		else{
			cst->cursor = cmdnode;
		}
		cmdnode = cmdnode->next;
	}
    //fprintf(stderr,"Line Number: %d\n",NUM_LINE);
	free(buffer);
	return cst;
	//error (1, 0, "command reading not yet implemented");
	//return 0;
}

command_t
read_command_stream(command_stream_t s)
{
	/* FIXME: Replace this with your implementation too.  */
	if (s->head != NULL){
		command_t cmd = s->head->command;
		command_node_t tmp = s->head;
		s->head = s->head->next;
		free(tmp);
		return cmd;
	}
	else
		return NULL;
	//error (1, 0, "command reading not yet implemented");
	//return 0;
}
