// Secure Centralized Asynchronous Communications
// Reese Myers (rsmyers)
// CPSC 6240 - Fall 2024
// 11/16/2024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define PATH_LIMIT 200
#define NUM_USER_DIRS 7

const char* mailDir = "/CompanyMail/mailboxes/";

const char* outbox = "/outbox";
const char* sent = "/sent";
const char* drafts = "/drafts";

const char* inbox = "/inbox";
const char* unread = "/unread";
const char* readStr = "/read";

const char* lockName = "/lock.lck";


// Function sets up the inbox and outbox for each user w/ all necessary files
void setupUserDir(char* username) {
    char* userPath = malloc(strlen(mailDir) + strlen(username) + 1);
    sprintf(userPath, "%s%s", mailDir, username);

    char* outboxPath = malloc(strlen(userPath) + strlen(outbox) + 1);
    sprintf(outboxPath, "%s%s", userPath, outbox);

    char* sentPath = malloc(strlen(outboxPath) + strlen(sent) + 1);
    sprintf(sentPath, "%s%s", outboxPath, sent);

    char* draftPath = malloc(strlen(outboxPath) + strlen(drafts) + 1);
    sprintf(draftPath, "%s%s", outboxPath, drafts);
    
    char* inboxPath = malloc(strlen(userPath) + strlen(inbox) + 1);
    sprintf(inboxPath, "%s%s", userPath, inbox);

    char* unreadPath = malloc(strlen(inboxPath) + strlen(unread) + 1);
    sprintf(unreadPath, "%s%s", inboxPath, unread);

    char* readPath = malloc(strlen(inboxPath) + strlen(readStr) + 1);
    sprintf(readPath, "%s%s", inboxPath, readStr);

    char* dirsToCreate[] = {userPath, outboxPath, sentPath, draftPath, inboxPath, unreadPath, readPath};

    // Checks for each directory and creates if necessary
    for (int i = 0; i < NUM_USER_DIRS; i++) {
        if (!access(dirsToCreate[i], F_OK)) {
		    puts("directory exists");
	    }
        else {
		    puts("DNE, creating...");
            printf("Creating %s\n", dirsToCreate[i]);
            usleep(1000);

		    int status = mkdir(dirsToCreate[i], 0600);

		    if (status) {
			    printf("Error creating Directories");
			    exit(-1);
		    }
	    }
    }

    // Lock files will be placed in unread directories
    char* unreadLockPath = malloc(strlen(unreadPath) + strlen(lockName) + 1);
    sprintf(unreadLockPath, "%s%s", unreadPath, lockName);
    
    FILE* lockFile = fopen(unreadLockPath, "w");
    fclose(lockFile);

    free(unreadLockPath);

    for (int i = 0; i < NUM_USER_DIRS; i++) {
        free(dirsToCreate[i]);
    }
}

int main(void) {
    FILE* fileRead;
    FILE* fileWrite;
    char line[MAX_LINE_LENGTH];

    const char* filenameRead = "/etc/passwd";
    const char* filenameWrite = "/CompanyMail/Config/users";

    fileRead = fopen(filenameRead, "r");

    if (fileRead == NULL) {
        printf("Error opening file to Read");
        return EXIT_FAILURE;
    }

    fileWrite = fopen(filenameWrite, "w");

    if (fileWrite == NULL) {
        printf("Error opening file to Write");
        return EXIT_FAILURE;
    }

    // Reads all actual users from /etc/passwd and adds to users file w/ UID
    while (fgets(line, sizeof(line), fileRead) != NULL) {

        char *username = strtok(line, ":");
        strtok(NULL, ":");
        char *uid_str = strtok(NULL, ":");

        if( username != NULL && uid_str != NULL) {
            int uid = atoi(uid_str);

            if (uid >= 1000 && uid <= 2000) {
                setupUserDir(username);
                fprintf(fileWrite, "%s:%s\n", username, uid_str);
            }
        }
    }
    fclose(fileRead);
    fclose(fileWrite);

}