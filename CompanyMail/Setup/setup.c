// Secure Centralized Asynchronous Communications
// Reese Myers (rsmyers)
// CPSC 6240 - Fall 2024
// 11/16/2024

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    system("clear");

    printf("Welcome to the Setup Utility for Company Mail\n");
    sleep(1);

    const char* configDirectory = "/CompanyMail/Config";
	const char* mailboxDir = "/CompanyMail/mailboxes";

    // Check if config directory already exists
	if (!access(configDirectory, F_OK)) {
		puts("directory exists");
	}
	else {
		puts("DNE, creating...");

		int status = mkdir(configDirectory, 0600);

		if (status) {
			printf("Error creating Directories");
			exit(-1);
		}

	}
    // Check if mailbox direcotry exists
	if (!access(mailboxDir, F_OK)) {
		puts("directory exists");
	}
	else {
		puts("DNE, creating...");

		int status = mkdir(mailboxDir, 0600);

		if (status) {
			printf("Error creating Directories");
			exit(-1);
		}

		
	}
    
    printf("\nDetermining users on System\nPlease Wait...\n");

    pid_t childID = fork();


    // Child process runs update users
	if (!childID) {
		execl("/CompanyMail/Setup/update_users", "/CompanyMail/Setup/update_users", NULL);
	}
	else {
		wait(NULL);
		sleep(1);
		printf("Done!\n");
	}




    bool promptAgain = true;
    char username[33];
    char compareUsername[33];

    FILE* userFile;
    FILE* adminFile;

    const char* usersFilename = "/CompanyMail/Config/users";
    const char* adminFilename = "/CompanyMail/Config/admins";

    userFile = fopen(usersFilename, "r");

    // Choose admin account user
    do {
        printf("\nPlease specify the username of the user who will have an administrator account (32 chars max): ");
        

        scanf("%32s", username);

        char c;

        while ((c = getchar()) != '\n' && c != EOF);

        bool confirmAgain = true;

        do {
            printf("\nYour chosen administrator's username is %s. Is that correct? (Y OR N): ", username);

            char confirmChar;

            scanf("%c", &confirmChar);
            confirmChar = tolower(confirmChar);
            while ((c = getchar()) != '\n' && c != EOF);

            if(confirmChar == 'y') {
                bool matchFound = false;
                int  successfulIO;
                char discard;
                
                // compares admin selection to users file to find match
                do {
                    successfulIO = fscanf(userFile, "%[^:]", compareUsername);
                    while ((discard = fgetc(userFile)) != '\n' && discard != EOF);

                    if (!strcmp(username, compareUsername)) {
                        matchFound = true;
                    }

                } while (!matchFound && discard != EOF);

                if (matchFound) {
                    promptAgain = false;
                    confirmAgain = false;
                }
                else {
                    fseek(userFile, SEEK_SET, 0);
                    printf("No user exists with username %s\n", username);
                    printf("\nHere is a listing of possible administrators and their corresponding UIDs:\n");

                    do {
                        successfulIO = fscanf(userFile, "%s", compareUsername);
                        if (successfulIO != EOF) {
                            printf("%s\n", compareUsername);
                        }
                        

                    } while (successfulIO != EOF);
                    fseek(userFile, SEEK_SET, 0);

                    confirmAgain = false;
                }
            }
            else if (confirmChar == 'n') {
                confirmAgain = false;
            }
            else {
                printf("\nInvalid Input\n");
            }

        } while (confirmAgain);

    } while (promptAgain);

    fclose(userFile);

    // saves the admin selection
    adminFile = fopen(adminFilename, "w");
    fprintf(adminFile, "%s\n", username);
    system("clear");
    
}