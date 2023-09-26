#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

int writer(char* filename, char* text)
{

    FILE *file = fopen(filename,"w+");
    size_t tlength;
    size_t wlength;

    openlog(NULL,0,LOG_USER);

    if (file == NULL) {
        int err = errno;
        syslog(LOG_ERR,"Error opening file %s [%d]: %s\n", filename, err, strerror(err));
        return 1;
    }

    tlength = strlen(text);
    wlength=fwrite(text, 1, tlength, file);
    if (wlength != tlength) {
        int err = errno;
        syslog(LOG_ERR,"Error writing to file %s [%d]: %s", filename, err, strerror(err));
        return 1;
    }

    fclose(file);

    syslog(LOG_INFO,"Writing '%s' to %s", text, filename);

    return 0;
}

int main(int argc, char* argv[])
{
    // open user log
    openlog(NULL,0,LOG_USER);

    // check that we have two parameters in addition to command (argc=3)
    if (argc != 3) {
        syslog(LOG_ERR,"writer needs two parameters");
        return 1;
    }

    // call writer with arguments 1 and 2 and return status
    return writer(argv[1],argv[2]);
}