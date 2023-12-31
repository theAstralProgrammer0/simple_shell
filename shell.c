#include "shell.h"

int main(__attribute__((unused))int argc, char **argv);

/**
 * tokenizer - make tokens from string
 * @line: string to tokenise
 * @delim: string delimiter
 *
 * Return: an array of tokens
 */
char **tokenizer(char *line, const char *delim)
{
	char *dup_line = NULL, *token = NULL, **tokens = NULL;
	size_t i, tok_count = 0;

	if (!line || !(*line))
		return (NULL);
	dup_line = strdup(line);
	token = strtok(dup_line, delim);
	while (token)
	{
		++tok_count;
		token = strtok(NULL, delim);
	}
	free(dup_line);
	if (tok_count > 0)
	{
		tokens = malloc(sizeof(char *) * (tok_count + 1));
		if (tokens == NULL)
		{
			fprintf(stderr, "malloc failed\n");
			exit(EXIT_FAILURE);
		}
		tokens[tok_count] = NULL;
		token = strtok(line, delim);
		for (i = 0; token; i++)
		{
			tokens[i] = strdup(token);
			token = strtok(NULL, delim);
		}
	}
	tok_count = 0;
	return (tokens);
}

/**
 * freeTokens - free tokens array
 * @tokens: tokens to free
 *
 * Return: nothing
 */
void freeTokens(char **tokens)
{
	int i;

	if (!tokens)
		return;
	for (i = 0; tokens[i]; i++)
	{
		free(tokens[i]);
		tokens[i] = NULL;
	}
	free(tokens);
	tokens = NULL;
}

/**
 * main - run a shell instance
 * @argc: argument count
 * @argv: argument vector
 *
 * Return: dependent on syscalls status
 */
int main(__attribute__((unused))int argc, char **argv)
{
	ssize_t bytesR = 0, cmd_count = 0;
	size_t n;
	char *line = NULL, **tokens = NULL;
	int lastExitCode = 0;
	alias_t *aliasList = initAliasList(NULL);
	alias_node *alias = NULL;

	while (1)
	{
		if (isatty(STDIN_FILENO))
			printf("$ ");
		fflush(stdout);
		++cmd_count;

		bytesR = getline(&line, &n, stdin);
		if (bytesR == -1) /* errors and EOF */
		{
			free(line);
			if (isatty(STDIN_FILENO))
				printf("\n");
			freeAliasList(aliasList);
			exit(lastExitCode);
		}
		line[bytesR - 1] = '\0';
		tokens = tokenizer(line, " ");
		if (!tokens)
			continue;
		if (strcmp(tokens[0], "alias") == 0)
			handleAlias(aliasList, tokens + 1);
		else if (strcmp(tokens[0], "setenv") == 0)
			handle_setenv(tokens);
		else if (strcmp(tokens[0], "unsetenv") == 0)
			handle_unsetenv(tokens);
		else
		{
			alias = findAlias(aliasList, tokens[0]);
			if (alias)
			{
				free(tokens[0]);
				tokens[0] = strdup(alias->value);
			}
			if (strcmp(tokens[0], "exit") == 0)
			{
				if (tokens[1])
				{
					if (isdigit(*tokens[1]) &&
							atoi(tokens[1]))
						lastExitCode = atoi(tokens[1]);
					else
					{
						fprintf(stderr,
						"%s: %ld: %s: Illegal number: %s\n",
						argv[0],
						cmd_count,
						tokens[0],
						tokens[1]);
						lastExitCode = 2;
					}
				}
				freeTokens(tokens);
				free(line);
				freeAliasList(aliasList);
				exit(lastExitCode);
			}
			if (strcmp(tokens[0], "env") == 0 || strcmp(tokens[0], "printenv") == 0)
			{
				handle_env();
				freeTokens(tokens);
				continue;
			}
			lastExitCode = executeCommands(tokens, argv, cmd_count);
		}
		freeTokens(tokens);
	}
	free(line);
	line = NULL;
	return (0);
}

/**
 * executeCommands - execute command
 * @tokens: an array of tokens
 * @argv: argument vector
 * @cmd_count: command count for each session
 *
 * Return: command status
 */
int executeCommands(char **tokens, char **argv, size_t cmd_count)
{
	pid_t pid;
	int status, exitCode;

	if (!tokens || !tokens[0])
		return (0);

	if (access(tokens[0], X_OK) == -1)
	{
		get_path(&tokens[0]);
		if (tokens && access(tokens[0], X_OK) == -1)
		{
			fprintf(stderr, "%s: %ld: %s: not found\n",
					argv[0], cmd_count, tokens[0]);
			return (127);
		}
	}
	pid = fork();
	if (pid == -1)
	{
		perror("fork");
		return (1);
	}

	if (pid == 0)
	{
		if (execve(tokens[0], tokens, environ) == -1)
		{
			perror("execve");
			exit(127);
		}
	}
	else
		waitpid(pid, &status, 0);
	exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 0;
	return (exitCode);
}

/**
 * get_path - get command path
 * @pathname: command to execute
 *
 * Return: nothing
 */
void get_path(char **pathname)
{
	char *token = NULL, *path = NULL, *fullpath = NULL, *dup_path;

	if (!pathname || !(*pathname))
		return;

	path = getenv("PATH");
	if (path == NULL)
		return;

	dup_path = strdup(path);
	token = strtok(dup_path, ":");
	while (token)
	{
		fullpath = malloc(sizeof(char) * (strlen(token)
					+ strlen(*pathname) + 2));
		sprintf(fullpath, "%s/%s", token, *pathname);

		if (access(fullpath, X_OK) == 0)
		{
			free(*pathname);
			*pathname = strdup(fullpath);
			free(fullpath);
			break;
		}
		free(fullpath);
		token = strtok(NULL, ":");
	}
	free(dup_path);
}
