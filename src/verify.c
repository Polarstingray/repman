

#include "lib/verify.h"
#include "lib/util.h"
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


int repman_verify_sha256(const char *filepath, const char *sha256_path) {
    pid_t childpid;
    int fd[2];

    if ((pipe(fd) == -1)) {
        perror("Failed to create pipe");
        return -1;
    }
    if ((childpid = fork()) == -1) {
        perror("Failed to fork");
        return -1;
    }
    if (childpid == 0) {        /* child process */
        close(fd[0]);
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout");
            exit(EXIT_FAILURE);
        }
        close(fd[1]);

        execlp("sha256sum", "sha256sum", filepath, NULL);
        perror("Failed to execute sha256sum");
        exit(EXIT_FAILURE);
    } 
    /* parent process */
    close(fd[1]);
    char buffer[512];
    int count = read(fd[0], buffer, sizeof(buffer) - 1);
    int status;
    waitpid(childpid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "sha256sum failed with status %d\n", WEXITSTATUS(status));
        return -1;
    }
    close(fd[0]);
    if (count == -1) {
        perror("Failed to read from child process");
        return -1;
    }
    buffer[count] = '\0';  // Null-terminate the output

    // Extract the checksum from the output
    char *token = strtok(buffer, " ");
    if (token == NULL) {
        fprintf(stderr, "Failed to parse sha256sum output\n");
        return -1;
    }
    char *actual_checksum = token;
    token = strtok(repman_read_file(sha256_path, NULL), " ");
    if (token == NULL) {
        fprintf(stderr, "Failed to parse expected checksum\n");
        return -1;
    }
    // Remove any trailing newline from the actual checksum
    actual_checksum[strcspn(actual_checksum, "\r\n")] = '\0';
    if (strlen(actual_checksum) != 64) {
        fprintf(stderr, "Invalid actual checksum length: %s\n", actual_checksum);
        return -1;
    }
    char *expected_checksum = token;
    if (expected_checksum == NULL) {
        fprintf(stderr, "Failed to read expected checksum from file\n");
        return -1;
    }
    // Remove any trailing newline from the expected checksum
    expected_checksum[strcspn(expected_checksum, "\r\n")] = '\0';
    if (strlen(expected_checksum) != 64) {
        fprintf(stderr, "Invalid expected checksum length: %s\n", expected_checksum);
        free(expected_checksum);
        return -1;
    }

    if (strcmp(actual_checksum, expected_checksum) != 0) {
        fprintf(stderr, "Checksum mismatch: expected %s, got %s\n", expected_checksum, actual_checksum);
        free(expected_checksum);
        return -1;
    }
    printf("Checksum verified successfully: %s\n", actual_checksum);
    return 0;
}



int repman_verify_minisig(const char *filepath, const char *minisig_path) {
    pid_t childpid;
    int fd[2];
    if ((pipe(fd) == -1)) {
        perror("Failed to create pipe");
        return -1;
    }
    if ((childpid = fork()) == -1) {
        perror("Failed to fork");
        return -1;
    }

    if (childpid == 0) {       /* child process */
        close(fd[0]);
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout");
            exit(EXIT_FAILURE);
        }
        close(fd[1]);
        
        // minisign -Vm myfile.txt -p ./minisign.pub
        execlp("minisign", "minisign", "-Vm", filepath, "-p", "ci.pub", NULL);
        perror("Failed to execute minisign");
        exit(EXIT_FAILURE);
    }
    /* parent process */
    close(fd[1]);
    int status;
    waitpid(childpid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "minisign failed with status %d\n", WEXITSTATUS(status));
        return -1;
    }
    close(fd[0]);

    fprintf(stdout, "Minisign signature verified successfully for file: %s\n", filepath);
    return 0;
}


int main() {

    download("https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json", "index.json");
    download("https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.sha256", "index.json.sha256");
    download("https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.minisig", "index.json.minisig");
    download("https://raw.githubusercontent.com/Polarstingray/repman-ci/refs/heads/main/ci.pub", "ci.pub");
    repman_verify_sha256("index.json", "index.json.sha256");
    repman_verify_minisig("index.json", "index.json.minisig");
    return 0;
}


// gcc src/verify.c src/util.c src/index.c -lcurl -o ./build/verify.o
// ./build/verify.o