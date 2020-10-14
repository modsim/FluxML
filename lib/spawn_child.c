#ifdef P_WIN32
#warning "spawn_child wird auf der Win32-Plattform nicht unterstuetzt"
#else

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>

/** hier muß eine gültige Shell angegeben werden */
#ifndef SHELL_CMD
#define SHELL_CMD "/bin/sh"
#endif

/** als Alternative zu pipe(2) kann auch socketpair(2) verwendet werden
#define HAVE_SOCKETPAIR 
#ifdef HAVE_SOCKETPAIR
#define GET_FD_PAIR(fd)	socketpair(AF_UNIX, SOCK_STREAM, 0, fd)
#else
#define GET_FD_PAIR(fd)	pipe(fd)
#endif
*/

#ifdef __STRICT_ANSI__
/** Prototyp für fdopen in ANSI-Umgebung */
FILE * fdopen(int, char const *);
#endif

//#ifdef P_LINUX
#if 1
extern char ** environ;
static char *** p_environ = &environ;
#else
# include <crt_externs.h>
#define p_environ _NSGetEnviron()
#endif

pid_t spawn_child_fd(
	char const * cmd,
	int *pipe_in,
	int *pipe_out,
	int *pipe_err
	)
{
	pid_t pid;
	int in_fds[2];
	int out_fds[2];
	int err_fds[2];
	int save_errno;

	if (pipe_in && pipe(in_fds) == -1)
		return -1;
	if (pipe_out && pipe(out_fds) == -1)
	{
		save_errno = errno;
		if (pipe_in)
		{
			close(in_fds[0]);
			close(in_fds[1]);
		}
		errno = save_errno;
		return -1;
	}
	if (pipe_err && pipe(err_fds) == -1)
	{
		save_errno = errno;
		if (pipe_in)
		{
			close(in_fds[0]);
			close(in_fds[1]);
		}
		if (pipe_out)
		{
			close(out_fds[0]);
			close(out_fds[1]);
		}
		errno = save_errno;
		return -1;
	}

	pid = fork();

	if (pid == -1)
	{
		/* fork() fehlgeschlagen */
		save_errno = errno;
		if (pipe_in)
		{
			close(in_fds[0]);
			close(in_fds[1]);
		}
		if (pipe_out)
		{
			close(out_fds[0]);
			close(out_fds[1]);
		}
		if (pipe_err)
		{
			close(err_fds[0]);
			close(err_fds[1]);
		}
		errno = save_errno;
		return -1;
	}

	if (pid == 0) /* Kind Prozess */
	{
		char * argv[4];
		
		if (pipe_out)
		{
			/* Lese-Ende schliessen */
			close(out_fds[0]);
			/* Binde Schreibe-Ende an stdout */
			dup2(out_fds[1], STDOUT_FILENO);
			/* Schließe altes Schreibe-Ende */
			close(out_fds[1]);
		}
		if (pipe_in)
		{
			close(in_fds[1]);
			dup2(in_fds[0], STDIN_FILENO);
			close(in_fds[0]);
		}
		if (pipe_err)
		{
			close(err_fds[0]);
			dup2(err_fds[1], STDERR_FILENO);
			close(err_fds[1]);
		}
		
		/* "HP-UX SIGCHLD fix" */
		signal(SIGCHLD, SIG_IGN);

		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = (char *)cmd;
		argv[3] = NULL;

		execve(SHELL_CMD, argv, *p_environ);
		_exit(EXIT_FAILURE);
	}

	/* Vater-Prozess */
	if (pipe_out)
	{
		/* nicht benötigtes Schreibe-Ende schließen */
		close(out_fds[1]);
		*pipe_out = out_fds[0];
	}
	if (pipe_in)
	{
		close(in_fds[0]);
		*pipe_in = in_fds[1];
	}
	if (pipe_err)
	{
		close(err_fds[1]);
		*pipe_err = err_fds[0];
	}
	return pid;
}

pid_t spawn_child_stream(
	char const * cmd,
	FILE **pipe_in,
	FILE **pipe_out,
	FILE **pipe_err
	)
{
	int fd_in, fd_out, fd_err;
	pid_t pid;

	pid = spawn_child_fd(
		cmd,
		pipe_in ? &fd_in : NULL,
		pipe_out ? &fd_out : NULL,
		pipe_err ? &fd_err : NULL
		);

	if (pid == -1)
		return -1;

	/*
	 * auf den von spawn_child_fd gelieferten file Descriptoren input-,
	 * output- und error-Streams oeffnen ...
	 */
	if (pipe_in)
	{
		*pipe_in = fdopen(fd_in, "a");
		if (!(*pipe_in))
			close(fd_in);
	}
	if (pipe_out)
	{
		*pipe_out = fdopen(fd_out, "r");
		if (!(*pipe_out))
			close(fd_out);
	}
	if (pipe_err)
	{
		*pipe_err = fdopen(fd_err, "r");
		if (!(*pipe_err))
			close(fd_err);
	}
	return pid;
}

#endif

#if 0
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int main()
{
	int i;
	pid_t pid;
	FILE *chin, *chout, *cherr;
	char command[] = "ls -l";
	char buf[256];
	size_t bytes;
	int status;

	pid = spawn_child_stream(command, &chin, &chout, /*NULL*/&cherr);

	if (pid == -1)
	{
		fprintf(stderr, "spawning of [%s] failed: %s\n",
			command, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("child's pid is %i\n", pid);

	/* buffering des chin-Streams abschalten */
	setbuf(chin, NULL);

	do
	{
		if (!feof(chout) && (bytes = fread(buf,1,sizeof(buf),chout)) != 0)
			fwrite(buf,1,bytes,stdout);
		if (!feof(cherr) && (bytes = fread(buf,1,sizeof(buf),cherr)) != 0)
			fwrite(buf,1,bytes,stderr);
	}
	while (!feof(chout) || !feof(cherr));
/*
	for (i=0; i<10; i++)
	{
		buf[0] = '\0';
		fputs("test\n", chin);
		fgets(buf, 255, chout);
		printf("answer: [%s]", buf);
		usleep(50000);
	}
*/
	/* input-Stream des Kind-Prozesses schliessen (schlaue Kind-Prozesse
	 * beenden sich); Vermeiden von Zombies: waitpid!
	 */
	fclose(chin);
	fclose(chout);
	fclose(cherr);

	waitpid(pid, &status, 0);

	if (WIFEXITED(status))
		printf("child exited (exit status %i)\n",
			WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		printf("child killed by signal %i\n",
			WTERMSIG(status));
	else if (WCOREDUMP(status))
		printf("child aborted with core dump\n");
	else if (WIFSTOPPED(status))
		printf("child is debugged (ptrace) / stopped with signal %i\n",
			WSTOPSIG(status));
	return 0;
}
#endif

