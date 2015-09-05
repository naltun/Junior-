#include "libs/mpc.h"

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

/* Create enumeration of possible error types */
enum { LISP_ERROR_DIV_ZERO, LISP_ERROR_BAD_OP, LISP_ERROR_BAD_NUM };

/* Create enumeration of possible lisp_value types */
enum { LISP_VALUE_NUMBER, LISP_VALUE_ERROR };

/* Declare new lisp_value struct */
typedef struct{
  int type;
  long number;
  int error;
} lisp_value;

/* Create a new number type for lisp_value */
lisp_value lisp_value_number(long x) {
  lisp_value v;
  v.type = LISP_VALUE_NUMBER;
  v.number = x;

  return v;
}

lisp_value lisp_value_error(int x) {
  lisp_value v;
  v.type = LISP_VALUE_ERROR;
  v.error = x;

  return v;
}

/* Prints a "lisp_value" */
void lisp_value_print(lisp_value v) {
  switch (v.type) {
    
    /* If the type is a number, it gets printed */
    /* After print, it 'breaks' out of the switch */
    case LISP_VALUE_NUMBER: printf("%li", v.number); break;

    /* If the type is an error */
    case LISP_VALUE_ERROR:
    
      /* Checks the type of the error and prints it */
      if (v.error == LISP_ERROR_DIV_ZERO) {
        printf("Error: cannot divide by zero!");
      }

      if (v.error == LISP_ERROR_BAD_OP)   {
        printf("Error: input contains invalid Operator!");
      }

      if (v.error == LISP_ERROR_BAD_NUM)  {
        printf("Error: input contains invalid Number!");
      }

      break;
  }
}

/* Prints a "lisp_value" with a newline */
void lisp_value_print_line(lisp_value v) { lisp_value_print(v); putchar('\n'); }

lisp_value eval_op(lisp_value x, char* op, lisp_value y) {

  /* If either the x or y value is an error, return value */
  if (x.type == LISP_VALUE_ERROR) { return x; }
  if (y.type == LISP_VALUE_ERROR) { return y; }

  /* Otherwise, perform mathematical operations on values x and y */
  /* Use the operator string to compare which operation to perform */
  if (strcmp(op, "+") == 0) { return lisp_value_number(x.number + y.number); }
  if (strcmp(op, "-") == 0) { return lisp_value_number(x.number - y.number); }
  if (strcmp(op, "*") == 0) { return lisp_value_number(x.number * y.number); }
  if (strcmp(op, "/") == 0) {

    /* If y is zero return error */
    return y.number == 0
      ? lisp_value_error(LISP_ERROR_DIV_ZERO)
      : lisp_value_number(x.number / y.number);
  }

  if (strcmp(op, "%") == 0) { 

    /* If y is zero return error */
    return y.number == 0
      ? lisp_value_error(LISP_ERROR_DIV_ZERO)
      : lisp_value_number(x.number % y.number);
  }

  return lisp_value_error(LISP_ERROR_BAD_OP);
}

/* eval function evaluates an expression */
/* For understanding the evaluation structure, look at line 77 */
lisp_value eval(mpc_ast_t* t) {

  /* If a number is tagged, then it returns directly */
  if (strstr(t->tag, "number")) {
    /* checks if there is a conversion error */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lisp_value_number(x) : lisp_value_error(LISP_ERROR_BAD_NUM);
  }

  /* Based on the regex tree for Junior-, the operator is always the second child */
  char* op = t->children[1]->contents;
  
  /* The third child is always a number, so it gets stored in 'x' */
  lisp_value x = eval(t->children[2]);

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
  mpc_parser_t* Number       = mpc_new("number");
  mpc_parser_t* Symbol       = mpc_new("symbol");
  mpc_parser_t* Sexpression = mpc_new("sexpression");
  mpc_parser_t* Expression   = mpc_new("expression");
  mpc_parser_t* Junior       = mpc_new("junior");

  /* Language definition */
  mpca_lang(MPCA_LANG_DEFAULT, 
    "                                                             \
      number      : /-?[0-9]+/ ;                                   \
      symbol      : '+' | '-' | '*' | '/' | '%' ;                  \
      sexpression : '(' <expression>* ')' ;                        \
      expression  : <number> | <symbol> | <sexpression> ;          \
      junior      : /^/ <operator> <expression>+ /$/ ;             \
    ",
    Number, Symbol, Sexpression, Expression, Junior);

  puts("\n\tJunior- Version 0.0.1\nDeveloped by Noah Altunian (github.com/naltun/)\n");
  puts("Press ctrl+C to Exit\n");

  while (1) {

    char* input = readline(">> ");

    add_history(input);

    /* Parse user input */
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Junior, &r)) {

      /* If evaluation is successful, print result and delete the output regex tree */
      lisp_value result = eval(r.output);
      lisp_value_print_line(result);
      mpc_ast_delete(r.output);
    } else {

      /* If not successful, print and delete error  */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpression, Expression, Junior);

  return 0;
}
