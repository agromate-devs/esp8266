#include <stdlib.h>
#include <stdio.h>
#include "../keystore/keystore.h"

#define UUID_LEN 37

// From https://stackoverflow.com/questions/51053568/generating-a-random-uuid-in-c
char* generate_uuid(void) {
    char v[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    //3fb17ebc-bc38-4939-bc8b-74f2443281d4
    //8 dash 4 dash 4 dash 4 dash 12
    static char buf[UUID_LEN] = {0};

    //gen random for all spaces because lazy
    for(int i = 0; i < 36; ++i) {
        buf[i] = v[rand()%16];
    }

    //put dashes in place
    buf[8] = '-';
    buf[13] = '-';
    buf[18] = '-';
    buf[23] = '-';

    //needs end byte
    buf[36] = '\0';

    return buf;
}

int uuid_exists() {
    char *uuid = read_key("uuid", UUID_LEN);
    return (uuid!= NULL);
}
