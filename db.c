/*
 * db.c
 * Functions for managing the data, abstracted
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "db.h"

void complete(char *name, char *message);
void uncomplete(char *name);

void addTask(char *name);
void removeTask(char *name);

int getday();
char *basePath();
char *taskPath(char *name);
char *logPath(char *name);
void assertSaveDir();

//Read and write a task
int writeTask(Task *t);
Task readTask(char *n);

//Read and write the logs of an existing task
int writeLog(Task *t);
int readLog(Task *t);


int getday() {
	return (int) time(NULL)/(60*60*24);
}

char *basePath() {
	char *path;
	char *save_dir = "/progress/";
	path = getenv("XDG_CONFIG_HOME");
	if (path == NULL) path = getenv("HOME");
	if (path == NULL) path = ".";
	int len = strlen(path) + strlen(save_dir) + 1;
	char *result = malloc(len*sizeof(char));
	strncpy(result, path, len);
	strncat(result, save_dir, len);
	return result;
}

char *taskPath(char *name) {
	assertSaveDir();
	char *save_path = basePath();
	int len = strlen(name) + strlen(save_path) + 1;
	char *path = malloc(len*sizeof(char));
	strncpy(path, save_path, len);
	strncat(path, name, len);
	free(save_path);
	return path;
}

char *logPath(char *name) {
	char *path = taskPath(name);
	int len = strlen(path) + 5; //for "_log"
	path = realloc(path, len*sizeof(char));
	strncat(path, "_log", len);
	return path;
}

void assertSaveDir() {
	struct stat st = {0};
	char *path = basePath();
	if (stat(path, &st) == -1) {
		mkdir(path, 0700);
	}
	free(path);
}

int writeTask(Task *t) {
	char *path = taskPath(t->name);
	FILE *f = fopen(path, "w");
	free(path);
	if (f == NULL) { return 0; }

	fwrite(t, sizeof(*t), 1, f);
	writeLog(t);
	fclose(f);
}

Task readTask(char *n) {
	char *path = taskPath(n);
	FILE *f = fopen(path, "r");
	free(path);
	Task new;

	fread(new.name, sizeof(char), 32, f);
	fread(&new.streak, sizeof(int), 1, f);
	fread(&new.logc, sizeof(int), 1, f);
	readLog(&new);

	fclose(f);
	return new;
}

int readLog(Task *t) {
	t->logs = malloc(t->logc*sizeof(Log));

	char *path = logPath(t->name);
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) { perror("Unable to read log"); return 0; }

	for (int i = 0; i < t->logc; i++) {
		fread(&t->logs[i].day, sizeof(int), 1, f);
		fread(t->logs[i].message, sizeof(char), 140, f);
	}

	fclose(f);
	return 1; //success
}

int writeLog(Task *t) {
	char *path = logPath(t->name);
	FILE *f = fopen(path, "w");
	free(path);
	if (f == NULL) { perror("Unable to write log"); return 0; }

	for (int i = 0; i < t->logc; i++) {
		fwrite(&t->logs[i].day, sizeof(int), 1, f);
		fwrite(t->logs[i].message, sizeof(char), 140, f);
	}

	fclose(f);
	return 1; //success
}

void printTask(Task *t) {
	printf("%s:\t %d day streak\n", t->name, t->streak);
}

void printLog(Task *t, int n) {
	int max = (n > t->logc || n == 0) ? t->logc : n;
	int today = getday();
	for (int i = 0; i < max; i++) {
		if (t->logs[i].message[0] != '\0') {
			printf(" - %dd ago:\t%s\n", today-t->logs[i].day, t->logs[i].message);
		}
	}
}

void complete(char *name, char *message) {
	char *path = taskPath(name);
	Task t = readTask(path);
	free(path);
	t.streak++;
	t.logc++;
	t.logs = realloc(t.logs, sizeof(Log)*t.logc);
	t.logs[t.logc-1].day = getday();
	strncpy(t.logs[t.logc-1].message, message, 140);
	writeTask(&t);
}

void uncomplete(char *name) {
	char *path = taskPath(name);
	Task t = readTask(path);
	free(path);
	t.streak--;
	t.logc--;
	t.logs = realloc(t.logs, sizeof(Log)*t.logc);
	writeTask(&t);
}

void addTask(char *name) {
	Task t;
	strncpy(t.name, name, 32);
	t.streak = 0;
	t.logc = 0;
	t.logs = NULL;
	writeTask(&t);
}
