#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>

int main(int arg, char** argv) {

  puts("\n\tJunior- Version 0.0.1\nDeveloped by Noah Altunian (github.com/naltun/)\n");
  puts("Press ctrl+C to Exit\n");

  while (1) {

    char* input = readline(">> ");

    add_history(input);

    printf("Now take notice of: %s\n\n", input);
    free(input);
  }

  return 0;
}
