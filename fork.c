#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void parent(int pid)
{
  printf("\033[1;33mparent (%d)\033[0m\n", getpid());

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

  printf("\033[1;33mparent done\033[0m\n");
}

int child(void)
{
  int num = 12;
  printf("\033[1;36mchild (%d) waiting %d seconds\033[0m\n", getpid(), num);
  for (; num > 0; num--)
  {
    printf("\033[1;36mchild %d\033[0m\n", num);
    sleep(1);
  }

  printf("\033[1;36mchild finished\033[0m\n");
  return 99;
}

int main()
{
  int pid = fork();
  if (pid == -1)
  {
    perror("fork\n");
    return 1;
  }
  else if (pid == 0)
  {
    return child();
  }
  else
  {
    parent(pid);
  }

  return 0;
}
