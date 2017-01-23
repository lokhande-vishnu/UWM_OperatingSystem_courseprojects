#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/time.h>
#define MXCHAR 513

typedef struct JOB {
  char** myargv;
  int narg;
  int jid;
  int pid;
  struct JOB* next;
}JOB;

int n_args(char* str) {
  int n = 0;
  char* token;
  token = strtok(str, " \t\n");
  while (token != NULL) {
    n++;
    token = NULL;
    token = strtok(NULL, " \t\n");
  }
  return n;
}

void free_mm(char** myargv, int narg) {
  narg++;
  int p;
  if (myargv) {
    for (p = 0; p < narg; p++) {
      free(myargv[p]);
    }
  }
  free(myargv);
}

void free_m(JOB *start) {
  JOB *temp = start;
  while (temp) {
    free_mm(temp->myargv, temp->narg);
    start = temp->next;
    free(temp);
    temp = start;
  }
}

int main(int argc, char *argv[]) {
  FILE *file = NULL;
  char *str_b;
  ssize_t read;
  char str[MXCHAR];
  char copy[MXCHAR];
  size_t len = 0;
  char *args;
  char** myargv;
  int narg;
  int jid = 0;
  int backg = 0;
  int exec_start = 0;
  JOB *job, *start, *jtemp;
  job = (JOB*)malloc(1*sizeof(JOB));
  job->jid = -1;
  job->myargv = NULL;
  job->narg = -1;
  job->pid = -1;
  job->next = NULL;
  start = job;

  if (argc == 1) {
    file = stdin;

  } else if (argc == 2) {
    args = strdup(argv[1]);
    file = fopen(args, "r");
    if (file == NULL) {
      char err[] = "Error: Cannot open file ";
      write(STDERR_FILENO, err, strlen(err));
      write(STDERR_FILENO, argv[1], strlen(argv[1]));
      write(STDERR_FILENO, "\n", 1);

      // Clearing the memory before exit
      free(args);
      free(job);
      exit(1);
    }
  } else {
    char err[] = "Usage: mysh [batchFile]\n";
    write(STDERR_FILENO, err, strlen(err));

    // Clearing the memory before exit
    free(job);
    exit(1);
  }

  while (1) {
    if (argc == 1) {
      write(STDOUT_FILENO, "mysh> ", strlen("mysh> "));
      if (fgets(str, MXCHAR, file) == NULL) {
          if (feof(file)) {
          // Clearing the memory;
          free_m(start);
          // when ctr-d encountered
          exit(0);
          } else {
          write(STDERR_FILENO, "Error in fgets\n", strlen("Error in fgets\n"));
          continue;
          }
      }
      jid++;
      backg = 0;
      exec_start = 0;
    } else {
      read = getline(&str_b, &len, file);
      if (read == -1) {
      // Clearing the memory before exit
        free(args);
        fclose(file);
        free(str_b);
        free_m(start);
        // EOF encountered
        exit(0);
      }

      if (strlen(str_b) > MXCHAR) {  // one extra for \n
        char err[] = "An error has occurred\n";
        write(STDERR_FILENO, err, strlen(err));
      } else {
        strcpy(str, str_b);
      }
      write(STDOUT_FILENO, str_b, strlen(str_b));
      jid++;
      backg = 0;
      exec_start = 0;
    }

    // Counting the number of strings
    strcpy(copy, str);
    narg =  n_args(str);  // counts the number of arguments in the string

    // Detecting empty command line
    if (narg < 1) {
      jid--;
      continue;
    }

    // Acquiring the argument to run
    myargv = (char**)malloc((narg+1)*sizeof(char*));
    int i = 0;
    myargv[i] = strdup(strtok(copy, " \t\n"));
    while (i < narg-1) {
      i++;
      myargv[i] = strdup(strtok(NULL, " \t\n"));
    }
    myargv[i+1] = NULL;

    // Identifying Background processes
    if (myargv[i][strlen(myargv[i])-1] == '&') {
      // Handling white spaces before and after '&'
      backg = 1;
      if (strlen(myargv[i]) > 1) {
         myargv[i][strlen(myargv[i])-1] = '\0';
      } else {
        myargv[i] = NULL;
      }

      if (myargv[0] == NULL) {
        // Clearing memory
        free_mm(myargv, narg);
        continue;
      }
    }

    // Built-in shell command "exit"
    if (strcmp(myargv[0], "exit\0") == 0) {
      if (myargv[1] != NULL) {
        exec_start = 1;
      } else {
        // Clearing the memory
          if (argc == 2) {
          free(args);
          fclose(file);
          free(str_b);
          }
          free_mm(myargv, narg);
          free_m(start);
          exit(0);
      }
}

    // Built-in shell command "j"
    if (strcmp(myargv[0], "j\0") == 0) {
      if (myargv[1] != NULL) {
        exec_start = 1;
      } else {
        jid--;
        jtemp = start;
        while (jtemp->jid != -1) {
          if (waitpid(jtemp->pid, NULL, WNOHANG) == 0) {
            printf("%d", jtemp->jid);
            fflush(stdout);
            write(STDOUT_FILENO, " : ", 3);
            int store = strlen(jtemp->myargv[0]);
            write(STDOUT_FILENO, jtemp->myargv[0], store);
            int k = 1;
            while (((jtemp->myargv)[k]) != NULL) {
              write(STDOUT_FILENO, " ", 1);
              int store = strlen(jtemp->myargv[k]);
              write(STDOUT_FILENO, jtemp->myargv[k], store);
              k++;
            }
            write(STDOUT_FILENO, "\n", 1);
            jtemp = jtemp->next;
            } else {
            // Deleting the finished job
            JOB* temp = jtemp->next;
            jtemp->jid = temp->jid;
            jtemp->pid = temp->pid;
            free(jtemp->myargv);
            jtemp->myargv = (char**)malloc(sizeof(temp->myargv));
            jtemp->myargv = temp->myargv;
            jtemp->next = temp->next;
            if (temp == job) {
              job = jtemp;  // Avoiding job to be deleted
            }
            free(temp);
          }
        }
        jtemp = NULL;
        free_mm(myargv, narg);
        continue;
      }
    }

    // Built-in shell command "myw"
    if (strcmp(myargv[0], "myw\0") == 0) {
      if (myargv[2] != NULL) {
        exec_start = 2;
      } else {
        jid--;
        int found = 0;
        char *ptr;
        int jid_req = strtol(myargv[1], &ptr, 10);
        if (jid_req > jid) {
          write(STDERR_FILENO, "Invalid jid ", strlen("Invalid jid "));
          write(STDERR_FILENO, myargv[1], strlen(myargv[1]));
          write(STDERR_FILENO, "\n", 1);
          free_mm(myargv, narg);
          continue;
        }
        jtemp = start;
        while (jtemp->jid != -1) {
          if (jtemp->jid == jid_req) {
            found = 1;
            break;
          }
          jtemp = jtemp->next;
        }
          if (found) {
          // Is it still running?
            if (waitpid(jtemp->pid, NULL, WNOHANG) == 0) {
            struct timeval begin, end;
            gettimeofday(&begin, NULL);
            (void) waitpid(jtemp->pid, NULL, WUNTRACED);
            gettimeofday(&end, NULL);
            long ini = (end.tv_sec * 1000000 + end.tv_usec);
            long fina = (begin.tv_sec * 1000000 + begin.tv_usec);
            long range = ini - fina;
            printf("%ld : Job %d terminated\n", range, jtemp->jid);
            fflush(stdout);
            } else {
            // It was already completed, hence delete it
            long range = 0;
            printf("%ld : Job %d terminated\n", range, jtemp->jid);
            fflush(stdout);
            JOB* temp = jtemp->next;
            jtemp->jid = temp->jid;
            jtemp->pid = temp->pid;
            free(jtemp->myargv);
            jtemp->myargv = (char**)malloc(sizeof(temp->myargv));
            jtemp->myargv = temp->myargv;
            jtemp->next = temp->next;
            if (temp == job) {
              job = jtemp;  // Avoiding job to be deleted
            }
            free(temp);
            }
            } else {
              long range = 0;
              printf("%ld : Job %d terminated\n", range, jid_req);
              fflush(stdout);
        }
        jtemp = NULL;
        free_mm(myargv, narg);
        continue;
      }
    }

    // Executing the argument via forking
    int rval = fork();
    // Child Process
    if (rval == 0) {
      execvp(myargv[exec_start], myargv);
      write(STDERR_FILENO, myargv[0], strlen(myargv[0]));
      write(STDERR_FILENO, ": ", 2);
      write(STDERR_FILENO, "Command not found", strlen("Command not found"));
      write(STDERR_FILENO, "\n", 1);

      // Clearing the memory before exit
      if (argc == 2) {
        free(args);
        fclose(file);
        free(str_b);
      }
      free_m(start);
      exit(1);
    } else if (!backg && rval > 0) {
      (void) waitpid(rval, NULL, WUNTRACED);
      free_mm(myargv, narg);
    } else if (backg && rval > 0) {
        job->pid = rval;
        job->jid = jid;
        job->myargv = myargv;
        job->narg = narg;
        job->next = (JOB*)malloc(1*sizeof(JOB));
        job = job->next;
        job->jid = -1;
        job->myargv = NULL;
        job->narg = -1;
        job->pid = -1;
        job->next = NULL;
      } else {
      char err[] = "An error has occurred\n";
        write(STDOUT_FILENO, err, strlen(err));
    }
    fflush(stdin);
    fflush(stdout);
  }
  // Clearing the memory;
  if (argc == 2) {
    free(args);
    fclose(file);
  }
  free(str_b);
  free_m(start);
  return 0;
}
