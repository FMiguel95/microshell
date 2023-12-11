#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <wait.h>
#include <unistd.h>

int erro(char *str)
{
	while (*str)
	{
		write(STDERR_FILENO, str, 1);
		str++;
	}
	return 1;
}

void check_fatal_error(int ret)
{
	if (ret < 0)
	{
		erro("error: fatal\n");
		exit(1);
	}
}

int fork1()
{
	int pid = fork();
	check_fatal_error(pid);
	return pid;
}

void run_command(char **args, char **env)
{
	int pid = fork1();
	if (*args)
	{
		if (!pid)
		{
			if (execve(args[0], args, env) < 0)
			{
				erro("error: cannot execute ");
				erro(args[0]);
				erro("\n");
				exit(1);
			}
			exit(1);
		}
		waitpid(pid, NULL, 0);
	}
	exit(0);
}

int has_pipe_next(char **av)
{
	while (*av && strcmp(*av, "|"))
		av++;
	return (*av && !strcmp(*av, "|"));
}

void run_pipes(char **av, char **env)
{
	if (has_pipe_next(av))
	{
		int p[2];
		check_fatal_error(pipe(p));
		
		if (!fork1())
		{
			// set | to NULL
			char **args = av;
			while (*av && strcmp(*av, "|"))
				av++;
			*av = NULL;
			check_fatal_error(dup2(p[1], STDOUT_FILENO));
			check_fatal_error(close(p[0]));
			check_fatal_error(close(p[1]));
			run_command(args, env);
			exit(0);
		}
		check_fatal_error(dup2(p[0], STDIN_FILENO));
		check_fatal_error(close(p[0]));
		check_fatal_error(close(p[1]));

		while (*av && strcmp(*av, "|"))
				av++;
		if (*av)
			av++;
		run_pipes(av, env);
	}
	else
	{
		run_command(av, env);
	}
}

void run_semicolon(char **av, char **env)
{
	char **args = av;

	//set ; to NULL
	while (*av && strcmp(*av, ";"))
		av++;
	*av = NULL;

	run_pipes(args, env);
}

void cd(char **av)
{
	if (av[1] && (strcmp(av[1], "|") && strcmp(av[1], ";")))
	{
		if (chdir(av[1]) < 0)
		{
			erro("error: cd: cannot change directory to ");
			erro(av[1]);
			erro("\n");
		}
	}
	else
	{
		erro("error: cd: bad arguments\n");
	}
}

int main(int ac, char **av, char **env)
{
	(void)ac;
	av++;
	while (*av)
	{
		if (!strcmp(*av, "cd"))
		{
			cd(av);
		}
		else if (strcmp(*av, ";"))
		{
			int pid = fork1();
			if (!pid)
			{
				run_semicolon(av, env);
				exit(0);
			}
			waitpid(pid, NULL, 0);
		}

		// skip to next cmd
		while (*av && strcmp(*av, ";"))
			av++;
		if (*av && !strcmp(*av, ";"))
			av++;
	}
	return 0;
}
