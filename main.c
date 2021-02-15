#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define CMD 0
#define PIPE 1

typedef struct s_cmd t_cmd;

void exe(t_cmd *cmd, char **env);

struct s_cmd
{
	int t;
	t_cmd *l;
	t_cmd *r;
	char **c;
};

void free_cmd(t_cmd *cmd)
{
	if (cmd->t == CMD)
	{
		free(cmd->c);
	}
	else{
		free_cmd(cmd->l);
		free_cmd(cmd->r);
	}
	free(cmd);
}

void str_error(char *s1, char *s2)
{
	if (!s1)
	{
		write(2, "error: fatal\n", 13);
		exit(1);
	}
	write(2, s1, strlen(s1));
	if (s2)
		write(2, s2, strlen(s2));
	write(2, "\n", 1);
}

void ft_cd(char **av)
{
	if (!av[1] || av[2])
		str_error("error: cd: bad arguments", NULL);
	else if (chdir(av[1]) == -1)
		str_error("error: cd: cannot change directory to ", av[1]);
}

void exe_pipe(t_cmd *cmd, char **env)
{
	int fd[2];
	pid_t pid = -1;
	
	if (pipe(fd) < 0 || (pid = fork()) < 0)
		str_error(NULL, NULL);
	if (pid == 0)
	{
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		exe(cmd->l, env);
		exit(0);
	}
	dup2(fd[0], 0);
	close(fd[1]);
	close(fd[0]);
	exe(cmd->r, env);
	waitpid(pid, NULL, 0);
}

void exe_cmd(char **av, char **env)
{
	pid_t pid;
	int ret;
	
	if (*av)
	{
		if (!strcmp(*av, "cd"))
		{
			ft_cd(av);
			return ;
		}
		if ((pid = fork()) < 0 )
			str_error(NULL, NULL);
		else if (pid == 0)
		{
			ret = execve(*av, av, env);
			str_error("error: cannot execute ", *av);
			exit(ret);
		}
		waitpid(pid, NULL, 0);
	}
}

void exe(t_cmd *cmd, char **env)
{
	if (cmd->t == CMD)
		exe_cmd(cmd->c, env);
	else
		exe_pipe(cmd, env);
}

t_cmd *cmd_new(int t, t_cmd *l, t_cmd *r)
{
	t_cmd *cmd;
	
	if (!(cmd = malloc(sizeof(t_cmd))))
		str_error(NULL, NULL);
	cmd->t = t;
	cmd->l = l;
	cmd->r = r;
	cmd->c = NULL;
	
	return cmd;
}

t_cmd *cmd_leaf(char ***pav)
{
	t_cmd *cmd = cmd_new(CMD, NULL, NULL);
	int j = 0;
	char **av = *pav;
	
	while (av[j] && strcmp(av[j], "|") && strcmp(av[j], ";"))
		j++;
	if (!(cmd->c = malloc(sizeof(char *) * ( j + 1))))
		str_error(NULL, NULL);
	for (int i = 0; i < j; i++)
		cmd->c[i] = av[i];
	cmd->c[j] = NULL;
	*pav = &av[j];
	
	return cmd;
}

t_cmd *cmd_create(char ***pav)
{
	t_cmd *cmd;
	
	cmd = cmd_leaf(pav);
	while (**pav && !strcmp(**pav, "|"))
	{
		(*pav)++;
		if (**pav && strcmp(**pav, "|"))
			cmd = cmd_new(PIPE, cmd, cmd_leaf(pav));
	}
	return cmd;
}

int main(int ac, char **av, char **env)
{
	av = &av[1];
	t_cmd *cmd;
	
	if (ac == 1)
		return 1;
	
	while(*av)
	{
		if (!strcmp(*av, ";"))
		{
			av++;
			continue ;
		}
		cmd = cmd_create(&av);
		exe(cmd, env);
		free_cmd(cmd);
	}
	return 0;
}