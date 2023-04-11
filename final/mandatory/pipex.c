/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipex.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ppimchan <ppimchan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/10 16:45:10 by ppimchan          #+#    #+#             */
/*   Updated: 2023/04/11 15:36:54 by ppimchan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/pipex.h"

static char	*find_path(char **envp)
{
	while (ft_strncmp("PATH", *envp, 4))
		envp++;
	return (*envp + 5);
}

static char	*get_execute_path(char **paths, char *cmd)
{
	char	*tmp;
	char	*command;

	while (*paths)
	{
		tmp = ft_strjoin(*paths, "/");
		command = ft_strjoin(tmp, cmd);
		free(tmp);
		if (access(command, 0) == 0)
			return (command);
		free(command);
		paths++;
	}
	return (NULL);
}

static void	first_child_process(t_pipe t, char **argv, char **envp)
{
	int	r;
	
	r = 0;
	t.infile = open(argv[1],O_RDONLY);
	if(t.infile < 0)
		perror_and_exit(argv[1],EXIT_SUCCESS);
	close(t.pipe_fd[0]);
	dup2(t.infile,STDIN_FILENO);
	dup2(t.pipe_fd[1],STDOUT_FILENO);
	t.cmd_args = ft_split(argv[2], ' ');
	t.execute_path = get_execute_path(t.env_path_lists ,t.cmd_args[0]);
	if(!t.execute_path)
	{
		free_exec_args(&t);
		msg_error(ERR_CMD);
		exit(127);
	}
	r = execve(t.execute_path, t.cmd_args  , envp);
	free_exec_args(&t);
	close(t.pipe_fd[1]); // need to close ?
	if(r == -1) 
		perror_and_exit("execve",1);
	exit(EXIT_SUCCESS);
}

static void	second_child_process(t_pipe t,int argc, char **argv, char **envp)
{
	int r;

	r = 0;
	t.outfile  = open(argv[argc - 1], O_TRUNC | O_CREAT | O_RDWR, 0000644);
	if(t.outfile < 0) 
		perror_and_exit(argv[argc - 1],EXIT_FAILURE);
	dup2(t.pipe_fd[0],STDIN_FILENO);
	close(t.pipe_fd[1]);
	dup2(t.outfile,STDOUT_FILENO);
	t.cmd_args = ft_split(argv[3], ' ');
	t.execute_path = get_execute_path(t.env_path_lists ,t.cmd_args[0]);
	if(!t.execute_path)
	{
		free_exec_args(&t);
		msg_error(ERR_CMD);
		exit(127);
	}
	r = execve(t.execute_path, t.cmd_args , envp);
	free_exec_args(&t);
	close(t.pipe_fd[0]);
	if(r == -1) 
		perror_and_exit("execve",1);
	exit(EXIT_SUCCESS);
}

void	close_pipes(t_pipe *t)
{
	close(t->pipe_fd[0]);
	close(t->pipe_fd[1]);
}

int	main(int argc, char **argv, char **envp)
{
	t_pipe	t;
	
	if(argc != 5)
		perror_and_exit(ERR_ARGS,EXIT_FAILURE);
	t.env_path = find_path(envp);
	t.env_path_lists = ft_split(t.env_path,':');
	if(pipe(t.pipe_fd) < 0)
		perror_and_exit(ERR_PIPE, EXIT_FAILURE);
	t.pid_first_child = fork();
	if(t.pid_first_child < 0)
		perror_and_exit(ERR_FORK, EXIT_FAILURE);
	if (t.pid_first_child == 0) 
		first_child_process(t,argv,envp);
	t.pid_second_child = fork();
	if(t.pid_first_child < 0)
		perror_and_exit(ERR_FORK, EXIT_FAILURE);
	if(t.pid_second_child == 0)
		second_child_process(t,argc,argv,envp);
	close_pipes(&t);
	waitpid(t.pid_first_child, &t.status,0);
	waitpid(t.pid_second_child, &t.status,0);
	parent_free(&t);
	if (WEXITSTATUS(t.status) != 0) 
		exit(WEXITSTATUS(t.status));
	exit(EXIT_SUCCESS);
}