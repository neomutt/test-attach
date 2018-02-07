#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void create_children(int count)
{
  children = calloc(count, sizeof(struct Child));
  num_children = count;
}

void add_child(int pid, const char *name)
{
  for (int i = 0; i < num_children; i++)
  {
    if (children[i].pid == 0)
    {
      children[i].pid = pid;
      children[i].file = strdup(name);
      open_children++;
      return;
    }
  }
}

int get_child(int pid)
{
  for (int i = 0; i < num_children; i++)
    if (children[i].pid == pid)
      return i;
  return -1;
}

void remove_child(int i)
{
  if ((i < 0) || (i >= num_children))
    return;

  if (unlink(children[i].file) < 0)
    printf("Error unlinking '%s' -- %s\n", children[i].file, strerror(errno));
  free((void *) children[i].file);
  children[i].file = NULL;
  children[i].pid = 0;
  open_children--;
}

void free_children()
{
  for (int i = 0; i < num_children; i++)
    free((void *) children[i].file);
  free(children);
}

void close_pid(int pid)
{
  int i = get_child(pid);
  if (i < 0)
  {
    printf("Can't find child pid %d\n", pid);
    return;
  }

  printf("Removing file %s for child %d (pid=%d)\n", children[i].file, i, pid);
  remove_child(i);
}

void wait_for_children(void)
{
  int wstatus = 0;

  pid_t w = waitpid(-1, &wstatus, WNOHANG);
  if (w == -1) /* error */
    return;

  if (w == 0) /* nothing waiting */
    return;

  if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus))
  {
    printf("\033[1;33mchild %d exited\033[0m\n", w);
    close_pid(w);
  }
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
    logmsg(fp, "child %d : %d\n", num, count);
    sleep(1);
  }

  logmsg(fp, "child %d (pid=%d) finished\n", num, getpid());

  fclose(fp);
  free_children();
  return (100 + num);
}

int main()
{
  srand(time(NULL));

  char name[32];
  int count = 10;
  int num;
  FILE *fp = NULL;

  create_children(count);

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
      add_child(pid, name);
      fclose(fp);
    }
  }

  sleep(2);
  printf("\033[1;32mwaiting for %d children\033[0m\n", open_children);
  while (open_children > 0)
  {
    wait_for_children();
    sleep(1);
  }

  printf("\033[1;32mall children closed\033[0m\n");
  free_children();
  return 0;
}
