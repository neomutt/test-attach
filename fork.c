#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

struct Child
{
  int pid;
  const char *file;
};

int num_children = 0;
int open_children = 0;
struct Child *children = NULL;

void close_pid(int pid)
{
  for (int i = 0; i < num_children; i++)
  {
    if (children[i].pid == pid)
    {
      printf("Removing file %s for child %d (pid=%d)\n", children[i].file, i, pid);
      if (unlink(children[i].file) < 0)
        printf("Error unlinking '%s' -- %s\n", children[i].file, strerror(errno));
      free((void*) children[i].file);
      children[i].file = NULL;
      children[i].pid = 0;
      open_children--;
      return;
    }
  }
  printf("Can't find child pid %d\n", pid);
}

void parent(void)
{
  // printf("\033[1;33mparent (%d)\033[0m\n", getpid());

  int wstatus;
  pid_t w;

  do
  {
    // printf("\033[1;33mparent waiting\033[0m\n");
    w = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
    // printf("\033[1;33mpid = %d, wstatus = %d (E%d,S%d)\033[0m\n", w, wstatus, WIFEXITED(wstatus), WIFSIGNALED(wstatus));
    if (w == -1)
    {
      printf("\033[1;31mpid = %d, wstatus = %d\033[0m\n", w, wstatus);
      break;
    }

    if (w == 0)
    {
      // printf("\033[1;31mZERO pid = %d, wstatus = %d\033[0m\n", w, wstatus);
    }
    else if (WIFEXITED(wstatus))
    {
      printf("\033[1;33mchild %d exited, status=%d\033[0m\n", w, WEXITSTATUS(wstatus));
      close_pid(w);
    }
    else if (WIFSIGNALED(wstatus))
    {
      printf("\033[1;33mchild %d killed by signal %d\033[0m\n", w, WTERMSIG(wstatus));
    }
    else if (WIFSTOPPED(wstatus))
    {
      printf("\033[1;33mchild %d stopped by signal %d\033[0m\n", w, WSTOPSIG(wstatus));
    }
    else if (WIFCONTINUED(wstatus))
    {
      printf("\033[1;33mchild %d continued\033[0m\n", w);
    }
    else
    {
      printf("\033[1;31mUNKNOWN pid = %d, wstatus = %d\033[0m\n", w, wstatus);
    }

    // printf("\033[1;33mparent sleeping %d, %d (E%d,S%d)\033[0m\n", w, wstatus, WIFEXITED(wstatus), WIFSIGNALED(wstatus));
    sleep(3);

  } while ((w == 0) || (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus)));

  // printf("\033[1;33mparent done\033[0m\n");
}

void logmsg(FILE *fp, const char *fmt, ...)
{
  char buffer[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);
  printf("%s", buffer);
  fprintf(fp, "%s", buffer);
}

int child(int num, int count, FILE *fp)
{
  logmsg(fp, "child %d (pid=%d) (count=%d)\n", num, getpid(), count);

  for (; count > 0; count--)
  {
    // logmsg(fp, "child %d : %d\n", num, count);
    sleep(1);
  }

  logmsg(fp, "child %d (pid=%d) finished\n", num, getpid());

  fclose(fp);

  for (int i = 0; i < num_children; i++)
    free((void*) children[i].file);

  free(children);

  return (100 + num);
}

int main()
{
  srand(time(NULL));

  char name[32];
  int count = 10;
  int num;
  FILE *fp = NULL;

  children = calloc(count, sizeof(struct Child));
  num_children = count;

  printf("\033[1;32mcreating %d children\033[0m\n", count);
  for (int i = 0; i < count; i++)
  {
    snprintf(name, sizeof(name), "file-%d", i);
    fp = fopen(name, "a+");

    num = (rand() % 10) + 3;

    int pid = fork();
    if (pid == -1)
    {
      fclose(fp);
    }
    else if (pid == 0)
    {
      return child(i, num, fp);
    }
    else
    {
      children[i].pid = pid;
      children[i].file = strdup(name);
      open_children++;
      fclose(fp);
    }
  }

  printf("\033[1;32mwaiting for %d children\033[0m\n", open_children);
  while (open_children > 0)
  {
    parent();
  }

  printf("\033[1;32mall children closed\033[0m\n");
  free(children);
  return 0;
}
