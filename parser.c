#include "mpc.h"

/* Code to be compiled on Windows */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include editline libraries */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

int main(int arg, char** argv) {

  /* Create Parsers */
  mpc_parser_t* Number     = mpc_new("number");
  mpc_parser_t* Operator   = mpc_new("operator");
  mpc_parser_t* Expression = mpc_new("expression");
  mpc_parser_t* Junior     = mpc_new("junior");

  /* Language definition */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                       \
      number     : /-?[0-9]+/ ;                             \
      operator   : '+' | '-' | '*' | '/' ;                  \
      expression : <number> | '(' <operator> <expr>+ ')' ;  \
      junior     : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expression, Junior);

  puts("\n\tJunior- Version 0.0.1\nDeveloped by Noah Altunian (github.com/naltun/)\n");
  puts("Press ctrl+C to Exit\n");

  while (1) {

    char* input = readline(">> ");

    add_history(input);

    printf("Now take notice of: %s\n\n", input);
    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expression, Junior);
  return 0;
}
