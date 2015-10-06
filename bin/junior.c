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

/* Create enumeration of possible lisp_value types  */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

/* Declare new lisp_value struct */
typedef struct lval {
  int type;
  long num;
  /* Both Error and Symbol types have string data, therefore they are char* */
  char* err;
  char* sym;
  // Count to a list of lval*
  int count;
  // Pointer to a list of lval*
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
  strcpy(v->sym, s);

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
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }

      /* Free the memory allocated to the pointers */
      free(v->cell);
    break;
  }

  /* Free the allocated memory for lval struct */
  free(v);
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;

  return v;
}

lval* lval_pop(lval* v, int i) {
  // Find the item at child 'i'
  lval* x = v->cell[i];

  // Shifts the memory after child 'i' to the top
  memmove(&v->cell[i], &v->cell[i+1],
  sizeof(lval*) * (v->count-i-1));

  // Decrease the count of items in the list
  v->count--;

  // Reallocate the used memory
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

/* Forward declaration of lval_print function on line */
/* This is done in order to call lval_print in the lval_expr_print function before the arguments get defined */
void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {

    /* Prints value */
    lval_print(v->cell[i]);

    /* If the last element has trailing space, doesn't print */
    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch(v->type) {

    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_ERR: printf("Error! %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* builtin_op(lval* a, char* op) {

  // Checks that all arguments are numbers
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on a non-number!");
    }
  }

  // Pops the first element
  lval* x = lval_pop(a, 0);

  // If there are no arguments and sub then perform a unary negation
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  // While further elements remain
  while (a->count > 0) {

    // Pops the next element as before
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }

    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Error: you can't divide by Zero!"); break;
      }
      x->num /= y->num;
    }

    if (strcmp(op, "%") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Error: cannot perform modulus with Zero!"); break;
      }
      x->num = x->num % y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {

  /* Evaluates the S-Expression's children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  // Checks for Errors
  for (int i= 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  // Returns empty Expression
  if (v->count == 0) { return v; }

  // Returns single Expression
  if (v->count == 1) { return lval_take(v, 0); }

  // Checks first element is a Symbol
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);

    return lval_err("S-expression does not start with a symbol!");
  }

  // Call builtin function with the operator
  lval* result = builtin_op(v, f->sym);
  lval_del(f);

  return result;
}

lval* lval_eval(lval* v) {
  // Evaluate S-expressions
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

  // For all other lval types
  return v;
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
  }

  return x;
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
    "                                                \
      number      : /-?[0-9]+/ ;                     \
      symbol      : '+' | '-' | '*' | '/' | '%' ;    \
      sexpr       : '(' <expression>* ')' ;          \
      expression  : <number> | <symbol> | <sexpr> ;  \
      junior      : /^/ <expression>* /$/ ;          \
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
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
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
