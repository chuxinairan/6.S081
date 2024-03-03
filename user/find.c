#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;    // buf is always exist, even current function return
    return buf;
}

void find(char *path, char *target)
{
    char *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_FILE:
        if (strcmp(fmtname(path), target) == 0)
        {
            printf("%s\n", path);
        }
        break;

    case T_DIR:

        if (strlen(path) + 1 + DIRSIZ + 1 > 512)
        {
            printf("find: path too long\n");
            break;
        }
        p = path + strlen(path);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;
            if (strcmp(".", de.name) == 0 || strcmp("..", de.name) == 0)
            {
                continue;
            }
            char *t = p;
            memmove(t, de.name, strlen(de.name));
            t[strlen(de.name)] = 0;
            find(path, target);
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(2, "Usage: find [path] [filename1] [filename2] ...\n");
        exit(1);
    }

    char path[512];
    strcpy(path, argv[1]);
    for (int i = 2; i < argc; i++)
    {
        find(path, argv[i]);
    }

    exit(0);
}