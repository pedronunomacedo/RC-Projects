#define MAX_BUF_SIZE 200
#define SERVER_PORT 21

#define HOST_REGEX_WHITH_USER_PASS "%*[^/]//%*[^@]@%[^/]"
#define PATH_REGEX_WHITH_USER_PASS "%*[^/]//%*[^/]%s"
#define USER_REGEX "%*[^/]//%[^:]"
#define PASSWORD_REGEX "%*[^/]//%*[^:]:%[^@]"

#define HOST_REGEX "%*[^/]//%[^/]"
#define PATH_REGEX "%*[^/]//%*[^/]%s"