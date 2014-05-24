#define _GNU_SOURCE  
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sched.h>

#define CACHE_CONFLICT 			0
#define CACHE_NO_CONFLICT		1


typedef int data_type;

data_type **matrix_a;
data_type **matrix_b;
data_type **matrix_c;
data_type **matrix_d;

int cache_type  = CACHE_CONFLICT;
int matrix_size = 1024;
int pthread_max = 1;
int cpu_online  = 1;
int dump = 0;

struct thread_data 
{
	pthread_t thread_id;
	int index;
	unsigned long start_time;
	unsigned long end_time;
};

void unix_error(char *msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(0);
}

void posix_error(int code, char *msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(code));
	exit(0);
}

void usage_error(char *name)
{
	printf("usage: %s \n", name);
	printf("\t-m 0(cache_conflict) | 1(no_cache_conflict)\n");
	printf("\t-t thread_count\n");
	printf("\t-s matrix_size\n");
	printf("\t-d(dumping matrixs)\n");
	exit(0);
}

int get_int(char *arg)
{
	long ret;
	char *endptr;

	if (arg == NULL || *arg == '\0')
	{
		unix_error("parameter should not be null");
	}

	errno = 0;
	ret = strtol(arg, &endptr, 10);
	if (errno != 0 || *endptr != '\0')
	{
		unix_error("parameter with nonnumeric characters");
	}
	return ret;
}

void create_matrix(data_type ***matrix)
{

	int i;
	(*matrix) = (data_type **)malloc(matrix_size * sizeof(data_type *));
	if ((*matrix) == NULL)
		goto error;
	for (i = 0; i < matrix_size; i++)
	{
		(*matrix)[i] = (data_type *)malloc(matrix_size * sizeof(data_type));
		if ((*matrix)[i] == NULL)
			goto error;
	}
	return;
error:
	unix_error("can't alloc memory");
}


void random_matrix(data_type **matrix)
{
	int i, j;
	for(i = 0; i < matrix_size; i++)
	{
		for(j = 0; j < matrix_size; j++)
		{
			matrix[i][j] = rand();
		}
	}
}

void nonmal_matrix_multipy(data_type **matrix_a, data_type **matrix_b, data_type **matrix_c)
{
	int i, j, k;
	data_type acc = 0;
	for(i = 0; i < matrix_size; i++)
	{
		for(j = 0; j < matrix_size; j++)
		{
			for(k = 0; k < matrix_size; k++)
			{
				acc = acc + matrix_a[i][k] * matrix_b[k][j];
			}
			matrix_c[i][j] = acc;
			acc = 0;
		}
	}
}

void cache_conflict_matrix_multipy(int index)
{
	int i, j, k;
	data_type acc = 0;
	for (i = 0; i < matrix_size; i++)
	{
		for (j = index; j < matrix_size; j+=pthread_max)
		{
			for (k = 0; k < matrix_size; k++)
			{
				acc = acc + matrix_a[i][k] * matrix_b[k][j];
			}
			matrix_c[i][j] = acc;
			acc = 0;
		}
	}
	
}

void cache_no_conflict_matrix_multipy(int index)
{
	int i, j, k;
	int start = (matrix_size / pthread_max) * index;
	int end = start + (matrix_size / pthread_max);
	data_type acc = 0;
	for (i = start; i < end; i++)
	{
		for (j = 0; j < matrix_size; j++)
		{
			for (k = 0; k < matrix_size; k++)
			{
				acc = acc + matrix_a[i][k] * matrix_b[k][j];
			}
			matrix_c[i][j] = acc;
			acc = 0;
		}
	}
}

int matrix_equal(data_type **matrix_a, data_type **matrix_b)
{
	int i, j;
	for(i = 0; i < matrix_size; i++)
	{
		for(j = 0; j < matrix_size; j++)
		{
			if (matrix_a[i][j] != matrix_b[i][j])
			{
				return 0;
			}
		}
	}
	return 1;
}

void dump_matrix(const char *msg, data_type **matrix)
{
	int i, j;
	printf("%s = \n", msg);
	for (i = 0; i < matrix_size; i++)
	{
		printf("[");
		for (j = 0; j < matrix_size; j++)
		{
			printf("%d", matrix[i][j]);
			if (j < matrix_size - 1)
			{
				printf(", \t");
			}
		}
		printf("]\n");
	}
}

void free_matrix(data_type **matrix)
{
	int i = 0;
	for (i = 0; i < matrix_size; i++)
	{
		free(matrix[i]);
	}
	free(matrix);
}

void set_thread_affinity(int cpu_number)
{
	int ret = 0;
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu_number%cpu_online, &mask);
	if ((ret = pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)) != 0)
	{
		posix_error(ret, "set thread affinity failed");
	}
}


unsigned long get_current_time()
{
	struct timeval current;
	gettimeofday(&current, NULL);
	return current.tv_sec * 1000000 + current.tv_usec;
}


void *thread_func(void *data)
{
	struct thread_data *thread = (struct thread_data *)data;
	int index = thread->index;
	
	set_thread_affinity(index);
	thread->start_time = get_current_time();
	if (cache_type == CACHE_CONFLICT)
	{
		cache_conflict_matrix_multipy(index);
	}else
	{
		cache_no_conflict_matrix_multipy(index);
	}
	thread->end_time = get_current_time();
	return NULL;
}

void program_parameter(char *name)
{

	printf("%s run with	%s,	%d threads, %d square matrix.\n", 
		name, cache_type == CACHE_CONFLICT ? "cache conflict" : "no cache conflict", pthread_max, matrix_size);

}

void init_program_parameter(int argc, char **argv)
{
	char ch;
	while ((ch = getopt(argc, argv, "m:t:s:d")) != -1)
	{
		switch(ch)
		{
			case 'm':
				cache_type = get_int(optarg);
				break;
			case 't':
				pthread_max = get_int(optarg);
				break;
			case 's':
				matrix_size = get_int(optarg);
				break;
			case 'd':
				dump = 1;
			break;

		}
	}
	if (matrix_size % pthread_max != 0)
	{
		unix_error("matrix size should be multiple of thread count");
	}
	if (cache_type != CACHE_CONFLICT && cache_type != CACHE_NO_CONFLICT)
	{
		unix_error("-m should be one of 0 or 1, for cache conflict or no cache conflict");
	}
}


void statistics(struct thread_data *data)
{
	int i;
	unsigned long max_start = 0, max_end = 0;
	for (i = 0; i < pthread_max; i++)
	{
		if (data[i].start_time > max_start)
		{
			max_start = data[i].start_time;
		}
		if (data[i].end_time > max_end)
		{
			max_end = data[i].end_time;
		}
	}
	printf("total used %ld us.\n", max_end - max_start);
}


int main(int argc, char *argv[])
{
	struct thread_data *threads;
	struct thread_data *thread;
	int i, ret, ch;

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0)
		{
			usage_error(argv[0]);
		}
		init_program_parameter(argc, argv);
	}

	program_parameter(argv[0]);

	create_matrix(&matrix_a);
	create_matrix(&matrix_b);
	create_matrix(&matrix_c);
	create_matrix(&matrix_d);
	random_matrix(matrix_a);
	random_matrix(matrix_b);

	nonmal_matrix_multipy(matrix_a, matrix_b, matrix_d);

	threads = (struct thread_data *)malloc(pthread_max * sizeof(struct thread_data));
	if (threads == NULL)
	{
		unix_error("malloc threads failed");
	}

	cpu_online = sysconf(_SC_NPROCESSORS_CONF);

	for(i = 0; i < pthread_max; i++)
	{
		thread = threads + i;
		thread->index = i;
		if ((ret = pthread_create(&thread->thread_id, NULL, thread_func, thread)) != 0)
		{
			posix_error(ret, "pthread_create failed");
		}
	}

	for(i = 0; i < pthread_max; i++)
	{
		thread = threads + i;
		if ((ret = pthread_join(thread->thread_id, NULL)) != 0)
		{
			posix_error(ret, "pthread_join failed");
		}
	}

	if (matrix_equal(matrix_c, matrix_d) == 0)
	{
		unix_error("runtime error");
	}
	if (dump)
	{
		dump_matrix("matrix A", matrix_a);
		dump_matrix("matrix B", matrix_b);
		dump_matrix("matrix C", matrix_c);
		dump_matrix("matrix D", matrix_d);
	}
	statistics(threads);

	free(matrix_a);
	free(matrix_b);
	free(matrix_c);
	free(matrix_d);
	free(threads);
	return 0;
}
