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

/*************************************************
** The names for the enumerations and structs   **
** below can seem intuitive enough; however,    **
** included in docs/names.txt is a list of the  **
** expounded meanings just in case :)           **
*************************************************/

/* Create enumeration of possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create enumeration of possible lisp_value types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

/* Declare new lisp_value struct */
typedef struct{
  int type;
  long number;
  /* Both Error and Symbol types have string data, therefore they are char* */
  char* err;
  char* sym;
  struct lval** cell;
} lval;

/* Create a pointer to a new Number type for lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;

  return v;
}

/* Create a pointer to a new Error lval */
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);

  return v;
}


/* Create a pointer to a new Symbol lval */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcopy(v->sym, s);

  return v;
}

/* Create a pointer to a new, empty Sexpr lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

void lval_del(lval* v) {
  switch (v->type) {

    /* Break if LVAL_NUM */
    case LVAL_NUM: break;

    /* Free string data from LVAL_ERR and LVAL_SYM */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    /* If LVAL_SEXPR then delete all elements within */
    CASE LVAL_SEXPR:
      for LVAL(int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }

      /* Free the memory allocated to the pointers */
      free(v->cell);
    break;
  }
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;

  return v;
}

/* Prints a "lisp_value" */
void lval_print(lval v) {
  switch (v.type) {

    /* If the type is a number, it gets printed */
    /* After print, it 'breaks' out of the switch */
    case LVAL_NUM: printf("%li", v.number); break;

    /* If the type is an error */
    case LVAL_ERR:

      /* Checks the type of the error and prints it */
      if (v.error == LERR_DIV_ZERO) {
        printf("Error: cannot divide by zero!");
      }

      if (v.error == LERR_BAD_OP)   {
        printf("Error: input contains invalid Operator!");
      }

      if (v.error == LERR_BAD_NUM)  {
        printf("Error: input contains invalid Number!");
      }

      break;
  }

  /* Free the memory allocated to the lval struct */
  free(v);
}

/* Prints a "lisp_value" with a newline */
void Lval_printlin(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

  /* If either the x or y value is an error, return value */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* Otherwise, perform mathematical operations on values x and y */
  /* Use the operator string to compare which operation to perform */
  if (strcmp(op, "+") == 0) { return lval_num(x.number + y.number); }
  if (strcmp(op, "-") == 0) { return lval_num(x.number - y.number); }
  if (strcmp(op, "*") == 0) { return lval_num(x.number * y.number); }
  if (strcmp(op, "/") == 0) {

    /* If y is zero return error */
    return y.number == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.number / y.number);
  }

  if (strcmp(op, "%") == 0) {

    /* If y is zero return error */
    return y.number == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.number % y.number);
  }

  return lval_err(LERR_BAD_OP);
}

/* eval function evaluates an expression */
/* For understanding the evaluation structure, look at line 77 */
lval eval(mpc_ast_t* t) {

  /* If a number is tagged, then it returns directly */
  if (strstr(t->tag, "number")) {
    /* checks if there is a conversion error */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  /* Based on the regex tree for Junior-, the operator is always the second child */
  char* op = t->children[1]->contents;

  /* The third child is always a number, so it gets stored in 'x' */
  lval x = eval(t->children[2]);

  /* Interates over the remaining children and combines them into the expression */
  int i = 3;
  while (strstr(t->children[i]->tag, "expression")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ?
    lval_num(x) : lval_err("invailid number!");
}

lval* lval_read(mpc_ast_t* t) {

  /* If Symbol or a Number, returns a conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If the root is '>' or sexpr then creates empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strcmp(t->tag, "sexpr"))  { x = lval_sexpr(); }

  /* Fill the list with any valid expression passed through */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }

    x = lval_add(x, lval_read(t->children[i]));

    return x;
  }
}

int main(int argc, char** argv) {

  /* Create Parsers */
  mpc_parser_t* Number       = mpc_new("number");
  mpc_parser_t* Symbol       = mpc_new("symbol");
  mpc_parser_t* Sexpr        = mpc_new("sexpr");
  mpc_parser_t* Expression   = mpc_new("expression");
  mpc_parser_t* Junior       = mpc_new("junior");

  /* Language definition */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                              \
      number      : /-?[0-9]+/ ;                                   \
      symbol      : '+' | '-' | '*' | '/' | '%' ;                  \
      sexpr       : '(' <expression>* ')' ;                        \
      expression  : <number> | <symbol> | <sexpr> ;                \
      junior      : /^/ <operator> <expression>+ /$/ ;             \
    ",
    Number, Symbol, Sexpr, Expression, Junior);

  puts("\n\tJunior- Version 0.0.1\nDeveloped by Noah Altunian (github.com/naltun/)\n");
  puts("Press ctrl+C to Exit\n");

  while (1) {

    char* input = readline(">> ");

    add_history(input);

    /* Parse user input */
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Junior, &r)) {

      /* If evaluation is successful, print result and delete the output regex tree */
      lval result = eval(r.output);
      Lval_printlin(result);
      mpc_ast_delete(r.output);
    } else {

      /* If not successful, print and delete error  */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpr, Expression, Junior);

  return 0;
}
