#include "mpc.h"

/* Code to be compiled on Windows */
#ifdef _WIN32

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

/* Use the operator string to compare which operation to perform */
long eval_op(long x, char* op, long y){
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x % y; }

  return 0;
}

/* eval function evaluates an expression */
/* For understanding the evaluation structure, look at line 77 */
long eval(mpc_ast_t* t) {

  /* If a number is tagged, then it returns directly */
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /* Based on the regex tree for Junior-, the operator is always the second child */
  char* op = t->children[1]->contents;

  /* The third child is always a number, so it gets stored in 'x' */
  long x = eval(t->children[2]);

  /* Interates over the remaining children and combines them into the expression */
  int i = 3;
  while (strstr(t->children[i]->tag, "expression")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {

  /* Create Parsers */
  mpc_parser_t* Number      = mpc_new("number");
  mpc_parser_t* Operator    = mpc_new("operator");
  mpc_parser_t* Expression  = mpc_new("expression");
  mpc_parser_t* Junior      = mpc_new("junior");

  /* Language definition */
  mpca_lang(MPCA_LANG_DEFAULT, 
    "                                                             \
      number     : /-?[0-9]+/ ;                                   \
      operator   : '+' | '-' | '*' | '/' | '%' ;                  \
      expression : <number> | '(' <operator> <expression>+ ')' ;  \
      junior     : /^/ <operator> <expression>+ /$/ ;             \
    ",
    Number, Operator, Expression, Junior);

  puts("\n\tJunior- Version 0.0.1\nDeveloped by Noah Altunian (github.com/naltun/)\n");
  puts("Press ctrl+C to Exit\n");

  while (1) {

    char* input = readline(">> ");

    add_history(input);

    /* Parse user input */
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Junior, &r)) {

      /* If evaluation is successful, print result and delete the output regex tree */
      long result = eval(r.output);
      printf("%li\n", result);
      mpc_ast_delete(r.output);

    } else {

      /* If not successful, print and delete error  */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expression, Junior);
  return 0;
}
