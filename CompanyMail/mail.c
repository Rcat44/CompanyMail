// Secure Centralized Asynchronous Communications
// Reese Myers (rsmyers)
// CPSC 6240 - Fall 2024
// 11/16/2024

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024

const char* mailDir = "/CompanyMail/mailboxes/";

const char* outbox = "/outbox";
const char* sent = "/sent";
const char* logName = "/log.txt";
const char* drafts = "/drafts";

const char* inbox = "/inbox";
const char* unread = "/unread";
const char* readStr = "/read";

const char* draftFilename = "/draft.txt";
const char* draftAttachmentFilename = "/draft.attach";
const char* draftHeadername = "/draft_hdr.txt";
const char* destinationsFilename = "/destinations.txt";
const char* lockName = "/lock.lck";

// Structs for file names/paths needed for each user
typedef struct customPaths {
	char* userPath;

    char* outboxPath;

    char* sentPath;

	char* sentLog;

    char* draftPath;
    
    char* inboxPath;

	char* unreadLog;

    char* unreadPath;

    char* readLog;

	char* readPath;

	char* unreadLock;
} paths;

// Generates all necessary paths to populate a paths struct.
// Paths are customized based on username
void generatePaths (paths* currPaths, char* username) {
	currPaths->userPath = malloc(strlen(mailDir) + strlen(username) + 1);
    sprintf(currPaths->userPath, "%s%s", mailDir, username);

    currPaths->outboxPath = malloc(strlen(currPaths->userPath) + strlen(outbox) + 1);
    sprintf(currPaths->outboxPath, "%s%s", currPaths->userPath, outbox);

    currPaths->sentPath = malloc(strlen(currPaths->outboxPath) + strlen(sent) + 1);
    sprintf(currPaths->sentPath, "%s%s", currPaths->outboxPath, sent);

	currPaths->sentLog = malloc(strlen(currPaths->sentPath) + strlen(logName) + 1);
    sprintf(currPaths->sentLog, "%s%s", currPaths->sentPath, logName);

    currPaths->draftPath = malloc(strlen(currPaths->outboxPath) + strlen(drafts) + 1);
    sprintf(currPaths->draftPath, "%s%s", currPaths->outboxPath, drafts);
	
    currPaths->inboxPath = malloc(strlen(currPaths->userPath) + strlen(inbox) + 1);
    sprintf(currPaths->inboxPath, "%s%s", currPaths->userPath, inbox);

    currPaths->unreadPath = malloc(strlen(currPaths->inboxPath) + strlen(unread) + 1);
    sprintf(currPaths->unreadPath, "%s%s", currPaths->inboxPath, unread);

	currPaths->unreadLog = malloc(strlen(currPaths->unreadPath) + strlen(logName) + 1);
    sprintf(currPaths->unreadLog, "%s%s", currPaths->unreadPath, logName);

    currPaths->readPath = malloc(strlen(currPaths->inboxPath) + strlen(readStr) + 1);
    sprintf(currPaths->readPath, "%s%s", currPaths->inboxPath, readStr);

	currPaths->readLog = malloc(strlen(currPaths->readPath) + strlen(logName) + 1);
    sprintf(currPaths->readLog, "%s%s", currPaths->readPath, logName);

	currPaths->unreadLock = malloc(strlen(currPaths->unreadPath) + strlen(lockName) + 1);
    sprintf(currPaths->unreadLock, "%s%s", currPaths->unreadPath, lockName);
}

// Frees all the memory of a paths struct
void freePaths(paths* currPaths) {
	free(currPaths->userPath);
	free(currPaths->outboxPath);
	free(currPaths->sentPath);
	free(currPaths->sentLog);
	free(currPaths->draftPath);
	free(currPaths->inboxPath);
	free(currPaths->unreadLog);
	free(currPaths->unreadPath);
	free(currPaths->readPath);
	free(currPaths->readLog);
	free(currPaths->unreadLock);

	currPaths->userPath = NULL;
	currPaths->outboxPath = NULL;
	currPaths->sentPath = NULL;
	currPaths->sentLog = NULL;
	currPaths->draftPath = NULL;
	currPaths->inboxPath = NULL;
	currPaths->unreadLog = NULL;
	currPaths->unreadPath = NULL;
	currPaths->readPath = NULL;
	currPaths->readLog = NULL;
	currPaths->unreadLock = NULL;

}

// Copies a file from a source to a destination
// Root permissions will be dropped when creating the destination file if
// dropPerms is true
int copyFile(const char *src_filename, const char *dest_filename, bool dropPerms) {
	// Opens source file in binary read mode
    FILE *src_file = fopen(src_filename, "rb");
    if (src_file == NULL) {
        perror("Error opening source file");
        return -1;
    }

	unsigned int euid = geteuid();
	unsigned int ruid = getuid();

	// drops root perms when writing file
	if (dropPerms) {
		setuid(ruid);
	}

    // Opens destination file in binary write mode
	FILE *dest_file = fopen(dest_filename, "wb");
    if (dest_file == NULL) {
        perror("Error opening destination file");
        fclose(src_file);
        return -1;
    }

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dest_file);
    }


    fclose(src_file);
    fclose(dest_file);

	setuid(euid);

    return 0;  // Success
}


// Appends the contents of a source file to a destination file
int appendFile(const char *src_filename, const char *dest_filename) {
    // Open the source file in binary read mode
    FILE *src_file = fopen(src_filename, "rb");
    if (src_file == NULL) {
        perror("Error opening source file");
        return -1;
    }

    // Open the destination file in binary append mode ("ab")
    FILE *dest_file = fopen(dest_filename, "ab");
    if (dest_file == NULL) {
        perror("Error opening destination file");
        fclose(src_file);
        return -1;
    }

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
            perror("Error writing to destination file");
            fclose(src_file);
            fclose(dest_file);
            return -1;
        }
    }

    fclose(src_file);
    fclose(dest_file);

    return 0;  // Success
}


// Prompts user with provided prompt. Asks for 'y' or 'n'
// Returns true if 'y' else if 'n' false
bool yesNoPromptFunc(char* prompt) {
	char inputChar;
	bool invalidInput = true;
	char discard;

	do {
		printf("%s (Y/N)?: ", prompt);
		scanf(" %c", &inputChar);
		while ((discard = getchar()) != '\n' && discard != EOF);
		inputChar = tolower(inputChar);

		if (inputChar == 'y' || inputChar == 'n') {
			invalidInput = false;
		}
	} while (invalidInput);

	return inputChar == 'y';
}

// Returns a string representing the time that can be used as a valid file name
char* getTimeString() {
	time_t raw_time;
    struct tm *time_info;
    char buffer[80];

    // Get the current time
    time(&raw_time);

    // Convert the time to local time
    time_info = localtime(&raw_time);

    // Format the time with underscores instead of spaces
    strftime(buffer, sizeof(buffer), "%Y_%m_%d_%H_%M_%S", time_info);

	char* timeStr = malloc(sizeof(buffer));

	strcpy(timeStr, buffer);

    return timeStr;
}

// Function to view all unread mail
void viewMail(char* username, paths* userPaths) {

	system("clear");
	
	char buffer[MAX_LINE_LENGTH];
	char tokenBuffer[MAX_LINE_LENGTH];
	bool unreadMail = false;

	// Check if unread log exists
	struct stat bufferStat;
	if (stat(userPaths->unreadLog, &bufferStat) != 0) {
   		printf("No Unread Mail!\n");
		return;
	}

	char* unreadLogCopy = malloc(strlen(userPaths->unreadLog) + strlen("_copy") + 1);
	sprintf(unreadLogCopy, "%s%s", userPaths->unreadLog, "_copy");

	char* stillUnread = malloc(strlen(userPaths->unreadLog) + strlen("_stillUnread") + 1);
	sprintf(stillUnread, "%s%s", userPaths->unreadLog, "_stillUnread"); 

	// Acquire Lock
	FILE* unreadLockFile = fopen(userPaths->unreadLock, "r");
	int lockFD = fileno(unreadLockFile);
	flock(lockFD, LOCK_EX);

	// Copy the log
	copyFile(userPaths->unreadLog, unreadLogCopy, false);
	FILE* unreadLogFile = fopen(userPaths->unreadLog, "w");
	fclose(unreadLogFile);

	// Release Lock
	flock(lockFD, LOCK_UN);

	FILE* unreadLogCopyFile = fopen(unreadLogCopy, "r");
	FILE* stillUnreadFile = fopen(stillUnread, "a");
	
	// Each part of log copy
	while (fscanf(unreadLogCopyFile, "%s", buffer) != EOF) {
		unreadMail = true;
		strcpy(tokenBuffer, buffer);
		char* usernameReceive = strtok(tokenBuffer, "_");
		char* dateTime = strtok(NULL, "\n");

		unsigned int count = 0;

		// Format the date of the message
		for (int i = 0; i < strlen(dateTime); i++) {
			if (dateTime[i] == '_') {
				if (count < 2) {
					dateTime[i] = '/';
				}
				else if (count == 2) {
					dateTime[i] = ' ';
				}
				else {
					dateTime[i] = ':';
				}
				count++;
			}
		}

		printf("Message received from %s at %s\n", usernameReceive, dateTime);

		// Does user want to read the message
		if(yesNoPromptFunc("Would you like to read this message")) {
			bool attachment;

			// Prepare to move to read folder
			char* currentMessageLocation = malloc(strlen(userPaths->unreadPath) + strlen("/") + strlen(buffer) + 1);
			sprintf(currentMessageLocation, "%s/%s", userPaths->unreadPath, buffer);

			char* futureMessageLocation = malloc(strlen(userPaths->readPath) + strlen("/") + strlen(buffer) + 1);
			sprintf(futureMessageLocation, "%s/%s", userPaths->readPath, buffer);

			// Add to the read log
			FILE* readLogFile = fopen(userPaths->readLog, "a");

			fprintf(readLogFile, "%s\n", buffer);

			fclose(readLogFile);

			char* currentAttachName = malloc(strlen(currentMessageLocation) + strlen("_attachment") + 1);
			sprintf(currentAttachName, "%s%s", currentMessageLocation, "_attachment");

			char* futureAttachName = malloc(strlen(futureMessageLocation) + strlen("_attachment") + 1);
			sprintf(futureAttachName, "%s%s", futureMessageLocation, "_attachment");


			FILE* readMessage = fopen(currentMessageLocation, "r");
			char attachBuffer[101];

			// Check for attachment, storing name in attach buffer or "NONE"
			do {
				fscanf(readMessage, "%s", attachBuffer);
			} while(strcmp(attachBuffer, "Attachment:") != 0);
			fscanf(readMessage, "%s", attachBuffer);

			attachment = strcmp(attachBuffer, "NONE") != 0;

			fclose(readMessage);

			// Move file link to read folder
			link(currentMessageLocation, futureMessageLocation);
			remove(currentMessageLocation);

			// Move attachment link if necessary
			if (attachment) {
				link(currentAttachName, futureAttachName);
				remove(currentAttachName);
			}

			pid_t childID = fork();

			char* vimArgs[] = {"vim", "-M", futureMessageLocation, NULL};

			// Child opens message
			if (!childID) {
				execvp("vim", vimArgs);
			}
			else {
				wait(NULL);

				// User can download an attachment to their home directory
				if (attachment) {
					if (yesNoPromptFunc("Would you like to download the file attached to this message")) {
						char* attachmentFilePath = malloc(strlen("/home/") + strlen(username) + strlen("/") + sizeof(attachBuffer));
						sprintf(attachmentFilePath, "/home/%s/%s", username, attachBuffer);

						printf("The file will be downloaded to: %s\n", attachmentFilePath);

						if (yesNoPromptFunc("Download the file")) {
							copyFile(futureAttachName, attachmentFilePath, true);
						}
						free(attachmentFilePath);
					}

				}
				
			}
			free(currentMessageLocation);
			free(futureMessageLocation);
			free(currentAttachName);
			free(futureAttachName);
		}
		else {
			fprintf(stillUnreadFile, "%s\n", buffer);
		}
	}
	fclose(unreadLogCopyFile);
	remove(unreadLogCopy);

	fclose(stillUnreadFile);

	// Acquire the lock again
	flock(lockFD, LOCK_EX);

	// Add any new arrivals to the still unread file
	appendFile(userPaths->unreadLog, stillUnread);
	// Copy the still unread file to the log
	copyFile(stillUnread, userPaths->unreadLog, false);

	// Remove still unread
	remove(stillUnread);

	// Release the lock
	flock(lockFD, LOCK_UN);
	
	fclose(unreadLockFile);

	system("clear");

	if (!unreadMail) {
		printf("No Unread Mail!\n");
	}

	free(unreadLogCopy);
	free(stillUnread);

}

// Function to view all read mail
void viewOldMail(char* username, paths* userPaths) {

	system("clear");
	

	char buffer[MAX_LINE_LENGTH];
	char tokenBuffer[MAX_LINE_LENGTH];
	bool readMail = false;

	// Check if read log exists
	struct stat bufferStat;
	if (stat(userPaths->readLog, &bufferStat) != 0) {
   		printf("No Read Mail!\n");
		return;
	}

	char* readLogCopy = malloc(strlen(userPaths->readLog) + strlen("_copy") + 1);
	sprintf(readLogCopy, "%s%s", userPaths->readLog, "_copy");


	copyFile(userPaths->readLog, readLogCopy, false);
	FILE* readLogFile = fopen(userPaths->readLog, "w");


	FILE* readLogCopyFile = fopen(readLogCopy, "r");
	
	// Read each entry in log copy
	while (fscanf(readLogCopyFile, "%s", buffer) != EOF) {
		readMail = true;
		strcpy(tokenBuffer, buffer);
		char* usernameReceive = strtok(tokenBuffer, "_");
		char* dateTime = strtok(NULL, "\n");

		unsigned int count = 0;

		// Format date of message
		for (int i = 0; i < strlen(dateTime); i++) {
			if (dateTime[i] == '_') {
				if (count < 2) {
					dateTime[i] = '/';
				}
				else if (count == 2) {
					dateTime[i] = ' ';
				}
				else {
					dateTime[i] = ':';
				}
				count++;
			}
		}

		bool attachment;

		char* currentMessageLocation = malloc(strlen(userPaths->readPath) + strlen("/") + strlen(buffer) + 1);
		sprintf(currentMessageLocation, "%s/%s", userPaths->readPath, buffer);


		char* currentAttachName = malloc(strlen(currentMessageLocation) + strlen("_attachment") + 1);
		sprintf(currentAttachName, "%s%s", currentMessageLocation, "_attachment");


		FILE* readMessage = fopen(currentMessageLocation, "r");
		char attachBuffer[101];

		// Check for attachment
		do {
			fscanf(readMessage, "%s", attachBuffer);
		} while(strcmp(attachBuffer, "Attachment:") != 0);
		fscanf(readMessage, "%s", attachBuffer);

		attachment = strcmp(attachBuffer, "NONE") != 0;

		fclose(readMessage);

		printf("Message received from %s at %s\n", usernameReceive, dateTime);

		// Does user want to read the message
		if(yesNoPromptFunc("Would you like to read this message")) {
			
			pid_t childID = fork();

			char* vimArgs[] = {"vim", "-M", currentMessageLocation, NULL};


			if (!childID) {
				execvp("vim", vimArgs);
			}
			else {
				wait(NULL);
				// User can download the attachment if desired
				if (attachment) {
					if (yesNoPromptFunc("Would you like to download the file attached to this message")) {
						char* attachmentFilePath = malloc(strlen("/home/") + strlen(username) + strlen("/") + sizeof(attachBuffer));
						sprintf(attachmentFilePath, "/home/%s/%s", username, attachBuffer);

						printf("The file will be downloaded to: %s\n", attachmentFilePath);

						if (yesNoPromptFunc("Download the file")) {
							copyFile(currentAttachName, attachmentFilePath, true);
						}
						free(attachmentFilePath);
					}

				}
				
			}
		}
		// Message and attachment can be deleted
		if(yesNoPromptFunc("Would you like to delete the message")) {
			if (attachment) {
				remove(currentAttachName);
			}
			remove(currentMessageLocation);

		}
		// Add log entry back to log if not deleted
		else {
			fprintf(readLogFile, "%s\n", buffer);
		}
		free(currentAttachName);
		free(currentMessageLocation);
	}
	fclose(readLogCopyFile);
	remove(readLogCopy);
	fclose(readLogFile);

	free(readLogCopy);


	system("clear");


	if (!readMail) {
		printf("No Read Mail!\n");
	}

}

// Function to view all sent mail
void viewSentMail(char* username, paths* userPaths) {

	system("clear");
	

	char buffer[MAX_LINE_LENGTH];
	bool sentMail = false;

	// Check if sent log exists
	struct stat bufferStat;
	if (stat(userPaths->sentLog, &bufferStat) != 0) {
   		printf("No Sent Mail!\n");
		return;
	}

	char* sentLogCopy = malloc(strlen(userPaths->sentLog) + strlen("_copy") + 1);
	sprintf(sentLogCopy, "%s%s", userPaths->sentLog, "_copy");


	copyFile(userPaths->sentLog, sentLogCopy, false);
	FILE* sentLogFile = fopen(userPaths->sentLog, "w");


	FILE* sentLogCopyFile = fopen(sentLogCopy, "r");
	
	// Read each entry of sent log copy
	while (fscanf(sentLogCopyFile, "%s", buffer) != EOF) {
		sentMail = true;
		char dateTime[MAX_LINE_LENGTH];
		strcpy(dateTime, buffer);

		unsigned int count = 0;
		
		// Extract date in proper format
		for (int i = 0; i < strlen(dateTime); i++) {
			if (dateTime[i] == '_') {
				if (count < 2) {
					dateTime[i] = '/';
				}
				else if (count == 2) {
					dateTime[i] = ' ';
				}
				else {
					dateTime[i] = ':';
				}
				count++;
			}
		}

		bool attachment;

		char* currentMessageLocation = malloc(strlen(userPaths->sentPath) + strlen("/") + strlen(buffer) + 1);
		sprintf(currentMessageLocation, "%s/%s", userPaths->sentPath, buffer);


		char* currentAttachName = malloc(strlen(currentMessageLocation) + strlen("_attachment") + 1);
		sprintf(currentAttachName, "%s%s", currentMessageLocation, "_attachment");

		char* currentDestinations = malloc(strlen(currentMessageLocation) + strlen("_destinations.txt") + 1);
		sprintf(currentDestinations, "%s%s", currentMessageLocation, "_destinations.txt");


		FILE* sentMessage = fopen(currentMessageLocation, "r");
		char attachBuffer[101];

		// Check for attachment
		do {
			fscanf(sentMessage, "%s", attachBuffer);
		} while(strcmp(attachBuffer, "Attachment:") != 0);
		fscanf(sentMessage, "%s", attachBuffer);

		attachment = strcmp(attachBuffer, "NONE") != 0;

		fclose(sentMessage);

		unsigned int destCount = 0;

		FILE* desinationFile = fopen(currentDestinations, "r");

		char destinationsBuffer[MAX_LINE_LENGTH];

		printf("Message sent at %s to ", dateTime);

		// Print who the message was sent to
		while (fscanf(desinationFile, "%s", destinationsBuffer) != EOF) {
			if (destCount == 0) {
				printf("%s", destinationsBuffer);
			}
			else {
				printf(", %s", destinationsBuffer);
			}
			destCount++;
		}
		printf("\n");

		fclose(desinationFile);

		// User can read the message
		if(yesNoPromptFunc("Would you like to read this message")) {
			
			pid_t childID = fork();

			char* vimArgs[] = {"vim", "-M", currentMessageLocation, NULL};


			if (!childID) {
				execvp("vim", vimArgs);
			}
			else {
				wait(NULL);
				// User can download an attachment
				if (attachment) {
					if (yesNoPromptFunc("Would you like to download the file attached to this message")) {
						char* attachmentFilePath = malloc(strlen("/home/") + strlen(username) + strlen("/") + sizeof(attachBuffer));
						sprintf(attachmentFilePath, "/home/%s/%s", username, attachBuffer);

						printf("The file will be downloaded to: %s\n", attachmentFilePath);

						if (yesNoPromptFunc("Download the file")) {
							copyFile(currentAttachName, attachmentFilePath, true);
						}
						free(attachmentFilePath);
					}

				}
				
			}
		}
		// Deletes message, destinations list, and attachment
		if(yesNoPromptFunc("Would you like to delete the message")) {
			if (attachment) {
				remove(currentAttachName);
			}
			remove(currentMessageLocation);
			remove(currentDestinations);

		}
		// Adds entry back to sent log if not deleted
		else {
			fprintf(sentLogFile, "%s\n", buffer);
		}
		free(currentAttachName);
		free(currentMessageLocation);
		free(currentDestinations);
	}
	fclose(sentLogCopyFile);
	remove(sentLogCopy);
	fclose(sentLogFile);

	free(sentLogCopy);

	system("clear");


	if (!sentMail) {
		printf("No Sent Mail!\n");
	}

}

// Function to send a message
void composeMail(char* username, paths* userPaths) {
	char* userDraftFilePath = malloc(strlen(userPaths->draftPath) + strlen(draftFilename) + 1);
	sprintf(userDraftFilePath, "%s%s", userPaths->draftPath, draftFilename);

	char* userDraftAttachmentFilePath = malloc(strlen(userPaths->draftPath) + strlen(draftAttachmentFilename) + 1);
	sprintf(userDraftAttachmentFilePath, "%s%s", userPaths->draftPath, draftAttachmentFilename);

	// User will initially compose a message within their own home directory
	char* userPersonalDraftFilePath = malloc(strlen("/home/") + strlen(username) + strlen(draftFilename) + 1);
	sprintf(userPersonalDraftFilePath, "%s%s%s", "/home/", username, draftFilename);

	char* userDestinations = malloc(strlen(userPaths->draftPath) + strlen(destinationsFilename) + 1);
	sprintf(userDestinations, "%s%s", userPaths->draftPath, destinationsFilename);

	FILE* clearDestinations = fopen(userDestinations, "w");
	fclose(clearDestinations);

	pid_t childID = fork();

	char* vimArgs[] = {"vim", userPersonalDraftFilePath, NULL};

	// Root permissions are dropped in child process before creating personal draft
	if (!childID) {
		setuid(getuid());

		FILE* personalDraft;
		personalDraft = fopen(userPersonalDraftFilePath, "w");
		fclose(personalDraft);

		execvp("vim", vimArgs);
	}
	else {
		wait(NULL);

		char subject[100];
		
		bool promptAgainSubject = true;
		bool promptAgainYesNo = true;
		char yesNo;
		char discard;

		system("clear");
		// User enters a subject line
		do {
			printf("Please enter a subject line for your message (100 chars max):\n\n");
			fgets(subject, sizeof(subject), stdin);
			printf("\nYou entered: %s\n", subject);
			do {
				printf("Is that subject line correct (Y/N)?: ");
				scanf("%c", &yesNo);
				while ((discard = getchar()) != '\n' && discard != EOF);

				yesNo = tolower(yesNo);

				if (yesNo == 'y') {
					promptAgainYesNo = false;
					promptAgainSubject = false;
				}
				else if (yesNo =='n') {
					promptAgainYesNo = false;
				}
			} while (promptAgainYesNo);
		} while (promptAgainSubject);



		FILE* personalDraft = fopen(userPersonalDraftFilePath, "r");
		FILE* actualDraft = fopen(userDraftFilePath, "w");

		char buffer[MAX_LINE_LENGTH];

		fprintf(actualDraft, "From: %s\n", username);
		fprintf(actualDraft, "Subject: %s", subject);

		// User may want to provide an attachment
		bool attachment = yesNoPromptFunc("Do you want to add an attachment");

		// User must specify valid path to attachment file or cancel attaching process.
		if(attachment) {
			bool invalidPath = true;
			do {
				char attachBuffer[256];
				printf("Please specify the relative path to the file: /home/%s/",username);
				scanf("%s", attachBuffer);
				char* attachmentFilePath = malloc(strlen("/home/") + strlen(username) + strlen("/") + sizeof(attachBuffer));
				sprintf(attachmentFilePath, "/home/%s/%s", username, attachBuffer);

				if (access(attachmentFilePath, F_OK) == 0) {
        			printf("File '%s' exists.\n", attachmentFilePath);
					invalidPath = false;

					FILE* personalAttachment = fopen(attachmentFilePath, "r");
					FILE* draftFolderAttach = fopen(userDraftAttachmentFilePath, "w");

					while (fgets(buffer, sizeof(buffer), personalAttachment) != NULL) {
						fputs(buffer, draftFolderAttach);
					}

					fclose(personalAttachment);
					fclose(draftFolderAttach);
    			} 
				else {
        			printf("File does not exist\n");
					attachment = yesNoPromptFunc("Do you still want to attach a file");
    			}
				free(attachmentFilePath);
				attachmentFilePath = NULL;

			} while (invalidPath && attachment);

		}

		// User can specify the name to send the file as
		if (attachment) {
			char attachmentDescription[101];

			do {
				printf("Please specify a name for the attachment file: ");
				scanf("%100s", attachmentDescription);

				char discard;
				while ((discard = getchar()) != '\n' && discard != EOF);

				printf("\nThe selected name is %s\n", attachmentDescription);

			} while (!yesNoPromptFunc("Is that the desired name"));
			fprintf(actualDraft, "Attachment: %s\n\n", attachmentDescription);

		}
		else {
			fprintf(actualDraft, "Attachment: NONE\n\n");
		}

		while (fgets(buffer, sizeof(buffer), personalDraft) != NULL) {
			fputs(buffer, actualDraft);
		}

		fclose(personalDraft);
		fclose(actualDraft);
		remove(userPersonalDraftFilePath);

		promptAgainYesNo = true;

		// Does user want to send the message?
		do {
			printf("Would you like to send the message now (Y/N)?: ");
			scanf(" %c", &yesNo);
			while ((discard = getchar()) != '\n' && discard != EOF);

			yesNo = tolower(yesNo);

			if (yesNo == 'y' || yesNo == 'n') {
				promptAgainYesNo = false;
			}
		} while (promptAgainYesNo);

		promptAgainYesNo = true;

		// Does user want to delete the draft?
		if (yesNo == 'n') {
			do {
				printf("\nDo you want to delete the draft (Y/N)?: ");
				scanf("%c", &yesNo);
				while ((discard = getchar()) != '\n' && discard != EOF);

				yesNo = tolower(yesNo);

				if (yesNo == 'y' || yesNo == 'n') {
					promptAgainYesNo = false;
				}
			} while (promptAgainYesNo);

			if (yesNo == 'y') {
				remove(userDraftFilePath);
			}
		}
		// Send the message
		else {
			char destination[33];
			char line[MAX_LINE_LENGTH];
			bool matchFound = false;
			unsigned int numDestinations = 0;

			const char* usrsFilePath = "/CompanyMail/Config/users";

			system("clear");

			FILE* usrFile;


			usrFile = fopen(usrsFilePath, "r");

			// must specify a valid account to send the message to
			do {
				matchFound = false;
				fseek(usrFile, 0, SEEK_SET);
				printf("Which user would you like to send the message to?: ");

				scanf("%32s", destination);

				// Performs destination username validation
				while(fgets(line, sizeof(line), usrFile) != NULL && !matchFound) {
					char *compareUsername = strtok(line, ":");

					if (!strcmp(destination, compareUsername)) {
						matchFound = true;
					}
						
				}
				// Checks if user already addressed message to specified valid account
				if (matchFound && numDestinations > 0) {
					char compareDest[33];
					bool alreadyAdded = false;
					
					FILE* destinationsFile;

					destinationsFile = fopen(userDestinations, "r");

					while (fscanf(destinationsFile, "%s", compareDest) != EOF && !alreadyAdded) {
    					if (!strcmp(destination, compareDest)) {
        					alreadyAdded = true;
							matchFound = false;
        					printf("Message already addressed to %s.\n", destination);
    					}
					}



					fprintf(destinationsFile, "%s\n", destination);
					fclose(destinationsFile);
				}

				if (!matchFound) {
					printf("Invalid Destination\n");
				}
				// Destination is valid, so destination username adds to destination list
				else {
					numDestinations++;
					FILE* destinationsFile;
					destinationsFile = fopen(userDestinations, "a");
					printf("The destination: %s\n\n", destination);
					fprintf(destinationsFile, "%s\n", destination);
					fclose(destinationsFile);

				}
				// User may add another destination.
			} while (yesNoPromptFunc("Would you like to add another destination"));

			// Sends message if user specified at least one valid destination
			if (numDestinations > 0) {
				char* timeStr = getTimeString();

				FILE* sentLog = fopen(userPaths->sentLog, "a");

				// Logs sending
				fprintf(sentLog, "%s\n", timeStr);
				fclose(sentLog);


				// Filenames are generated for both the sender's sent folder and the unread folders of the destinations
				char* sentName = malloc(strlen(userPaths->sentPath) + strlen("/") + strlen(timeStr) + 1);
				sprintf(sentName, "%s/%s", userPaths->sentPath, timeStr);

				char* sentDestinations = malloc(strlen(userPaths->sentPath) + strlen("/") + strlen(timeStr) + strlen("_destinations.txt") + 1);
				sprintf(sentDestinations, "%s/%s_destinations.txt", userPaths->sentPath, timeStr);

				// moves draft from draft to sent folder
				link(userDraftFilePath, sentName);
				link(userDestinations, sentDestinations);

				char* destLogMessageName = malloc(strlen(username) + strlen("_") + strlen(timeStr) + 1);
				sprintf(destLogMessageName, "%s_%s", username, timeStr);

				char* destFileMessageName = malloc(strlen(username) + strlen("_") + strlen(timeStr) + 1);
				sprintf(destFileMessageName, "%s_%s", username, timeStr);

				char* attachmentName;

				// attachment moved if necessary
				if (attachment) {
					attachmentName = malloc(strlen(destFileMessageName) + strlen("_") + strlen("attachment") + 1);
					sprintf(attachmentName, "%s_%s", destFileMessageName, "attachment");

					char* senderAttachmentPath = malloc(strlen(sentName) + strlen("_") + strlen("attachment") + 1);
					sprintf(senderAttachmentPath, "%s_%s", sentName, "attachment");

					link(userDraftAttachmentFilePath, senderAttachmentPath);
					
					free(senderAttachmentPath);
					senderAttachmentPath = NULL;
				}
				else {
					attachmentName = NULL;
				}

				FILE* destinationsFile = fopen(userDestinations, "r");

				// Sends to each user specified in destinations file
				for (int i = 0; i < numDestinations; i++) {
					char destUsername[33];

					fscanf(destinationsFile, "%s", destUsername);
					paths curDestPaths;
					generatePaths(&curDestPaths, destUsername);

					

					char* destFilePath = malloc(strlen(curDestPaths.unreadPath) + strlen("/") + strlen(destFileMessageName));
					sprintf(destFilePath, "%s/%s", curDestPaths.unreadPath, destFileMessageName);

					FILE* curDestLock = fopen(curDestPaths.unreadLock, "r");

					int lockFD = fileno(curDestLock);

					// Lock aquired so entry in destination's unread log can be safely added
					flock(lockFD, LOCK_EX);

					FILE* curDestLog = fopen(curDestPaths.unreadLog, "a");

					// Entry added
					fprintf(curDestLog, "%s\n", destLogMessageName);

					fclose(curDestLog);

					// Link created in destination unread directory
					link(userDraftFilePath, destFilePath);

					// Attachment link created if necessary
					if (attachment) {
						char * destAttachName = malloc(strlen(curDestPaths.unreadPath) + strlen("/") + strlen(attachmentName) + 1);
						sprintf(destAttachName, "%s/%s", curDestPaths.unreadPath, attachmentName);
						link(userDraftAttachmentFilePath, destAttachName);
						free(destAttachName);
					}

					// Lock released
					flock(lockFD, LOCK_UN);

					fclose(curDestLock);
					
					free(destFilePath);
					destFilePath = NULL;
					freePaths(&curDestPaths);

				}
				// Clears out user's draft folder
				remove(userDraftFilePath);
				remove(userDestinations);
				if (attachment) {
					remove(userDraftAttachmentFilePath);
					free(attachmentName);
				}
				system("clear");
				printf("Mesage Sent\n");
				sleep(1);

				free(sentName);
				free(sentDestinations);
				free(destLogMessageName);
				free(destFileMessageName);
			}
		}
	}
	system("clear");

	free(userDraftFilePath);
	free(userPersonalDraftFilePath);
	free(userDraftAttachmentFilePath);
	free(userDestinations);

	userDraftFilePath = NULL;
	userPersonalDraftFilePath = NULL;
	userDraftAttachmentFilePath = NULL;
	userDestinations = NULL;
}

// Displays menu of choices for regular users
// Returns a char representing a valid selection
char displayMenu() {
	bool needSelection = true;

	char selection, discard;

	do {
		printf("Please Make a Selection.\n\n");
		printf("Company Mail\n\n");
		printf("View Unread Mesages: V\n");
		printf("View Read Mesages: R\n");
		printf("View Sent Mesages: S\n");
		printf("Compose a Message: C\n");
		printf("Quit: Q\n");
		printf("Your Selection: ");
		scanf(" %c", &selection);
		while ((discard = getchar()) != '\n' && discard != EOF);

		selection = tolower(selection);


		if(selection == 'v' || selection == 'c' || selection == 'q' || selection == 'r' || selection == 's') {
			needSelection = false;
		}
		else {
			system("clear");
			printf("Invalid Input\n\n");
		}

	} while (needSelection);

	return selection;
}

// Displays menu of choices for users running program with sudo
// Returns a char representing a valid selection
char displayAdminMenu(void) {
	bool needSelection = true;

	char selection, discard;

	do {
		printf("Please Make a Selection.\n\n");
		printf("Company Mail Admin Menu\n\n");
		printf("Update Users In Company Mail System: U\n");
		printf("Run Setup Utility: S\n");
		printf("Quit: Q\n");
		printf("Your Selection: ");
		scanf(" %c", &selection);
		while ((discard = getchar()) != '\n' && discard != EOF);

		selection = tolower(selection);


		if(selection == 'u' || selection == 's' || selection == 'q') {
			needSelection = false;
		}
		else {
			system("clear");
			printf("Invalid Input\n\n");
		}

	} while (needSelection);

	system("clear");

	return selection;

}

// If root or sudoer is running program, this function gains control of the program
// Admin menu is displayed. Setup or update_user utilities may be executed.
// Otherwise, user can quit
void runAdminMenu(void) {
	system("clear");
	char selection;

	do {
		selection = displayAdminMenu();
		
		switch (selection) {
			case 'u':
				execl("/CompanyMail/Setup/update_users", "/CompanyMail/Setup/update_users", NULL);
				break;
			case 's':
				execl("/CompanyMail/Setup/setup", "/CompanyMail/Setup/setup", NULL);
				break;
		}

	} while (selection != 'q');
}


int main(void) {
	system("clear");

	FILE* users;
	FILE* admins;

	const char* usersFilename = "/CompanyMail/Config/users";
	const char* adminFilename = "/CompanyMail/Config/admins";

	char ruidStr[11];

	sprintf(ruidStr, "%d", getuid());

	// Checks if RUID is root and runs admin menu if so
	if (getuid() == 0) {
		runAdminMenu();
		return 0;
	}


	char line[MAX_LINE_LENGTH];

	users = fopen(usersFilename, "r");

	bool accountExists = false;

	char savedUsername[33];

	// Checks if user that is running program is in the Company Mail System.
	while (fgets(line, sizeof(line), users) != NULL && !accountExists) {

        char *username = strtok(line, ":");

        char *uid_str = strtok(NULL, "\n");

        if(!strcmp(ruidStr,uid_str)) {
            accountExists = true;
			strcpy(savedUsername, username);
			printf("Welcome %s.\n\n", username);
        }
    }
	fclose(users);

	// Program exits if user has not be added to Company Mail System.
	if(!accountExists) {
		puts("You do not have a Company Mail account.\nAsk an admin to add you to the system.\n");
		exit(1);
	}



	bool isAdmin = false;
	admins = fopen(adminFilename, "r");

	char compareUsername[33];
	int successfulIO;

	// Checks if the user who executed the program is the admin.
	do {
		successfulIO = fscanf(admins, "%s", compareUsername);
		if (!strcmp(savedUsername, compareUsername)) {
			isAdmin = true;
			printf("You are an admin\n");
		}
	} while (successfulIO != EOF && !isAdmin);

	fclose(admins);

	char selection;


	paths currentUserPaths;

	// Generates all custom paths for the user who executed the program
	generatePaths(&currentUserPaths, savedUsername);
	
	// Loop provides user with a menu of choices.
	// Loop iterates until user quits the program.
	do {
		selection = displayMenu();

		switch (selection) {
			case 'c':
				composeMail(savedUsername, &currentUserPaths);
				break;
			case 'v':
				viewMail(savedUsername, &currentUserPaths);
				break;
			case 'r':
				viewOldMail(savedUsername, &currentUserPaths);
				break;
			case 's':
				viewSentMail(savedUsername, &currentUserPaths);
				break;
		}



	} while (selection != 'q');

	freePaths(&currentUserPaths);
	system("clear");

}
