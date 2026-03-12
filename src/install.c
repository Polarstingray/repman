

#include "lib/verify.h"
#include "lib/util.h"
#include <stdio.h>
#include <stdlib.h>

#define PACKAGE_URL "https://github.com/Polarstingray/packages/releases/download/test-v1.2.10/test_v1.2.10_ubuntu_amd64.tar.gz"

// gcc src/verify.c src/util.c src/install.c src/index.c -lcurl -o ./build/install.o && ./build/install.o 
// && tree ~/.local/share/repman



int main() {
    repman_download(PACKAGE_URL, "test_v1.2.10_ubuntu_amd64.tar.gz");
    return 0;
}
